#include "UI/Combat/SkillDetailOverlayPresenter.h"

#include "Actor/TileMap/TileLayer.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"
#include "UI/Combat/SkillTacticalDiagramWidget.h"
#include "UI/DetailOverlayInputShield.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "CombatLayoutHUD"

/*
 * 지역 헬퍼는 Detail 접두를 붙인다. 익명 이름 그대로 두면 유니티 빌드가
 * CombatLayoutHUDWidget.cpp의 같은 이름 헬퍼와 한 덩어리로 묶일 때 중복
 * 정의로 깨진다 -- 파일 수가 바뀌는 날 갑자기 (MercenaryHireDetail과 같은 사유).
 */
namespace
{
	/** @brief 있으면 보이고 없으면 접는다. 배치안마다 요소를 빼도 되게 하는 핵심. */
	void DetailSetShown(UWidget* Widget, const bool bShown)
	{
		if (Widget != nullptr)
		{
			Widget->SetVisibility(bShown
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
		}
	}

	void DetailSetTextIfPresent(UTextBlock* Text, const FText& Value)
	{
		if (Text != nullptr)
		{
			Text->SetText(Value);
		}
	}

	/** @brief 공용 상세 WBP의 authored Canvas 슬롯을 상세 종류별로 안전하게 옮긴다. */
	void SetCanvasRect(UWidget* Widget, const FVector2D Position,
		const FVector2D Size)
	{
		if (Widget == nullptr)
		{
			return;
		}
		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
		}
	}

	/**
	 * @brief 초상을 칸에 맞춰 윗부분만 잘라 그린다.
	 *
	 * @details
	 * 초상은 972x1619 로 길쭉한데 칸은 정사각에 가깝다. 그냥 넣으면 가로로
	 * 눌려 얼굴이 찌그러진다. 여백을 두면 칸이 작아지고, 정사각본을 따로
	 * 만들면 자산이 배로 는다. 그래서 **UV 로 윗부분만** 쓴다 -- HUD 본체의
	 * 같은 이름 헬퍼와 동일한 규약이다.
	 */
	void DetailSetPortraitCropped(UImage* Image, UTexture2D* Texture, const float Aspect = 1.f)
	{
		if (Image == nullptr || Texture == nullptr)
		{
			return;
		}
		const float Width = static_cast<float>(Texture->GetSizeX());
		const float Height = static_cast<float>(Texture->GetSizeY());
		if (Width <= 0.f || Height <= 0.f || Aspect <= 0.f)
		{
			Image->SetBrushFromTexture(Texture, false);
			return;
		}

		FSlateBrush Brush = Image->GetBrush();
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Image;

		// 칸보다 세로로 길면 위에서 잘라 쓰고, 가로로 길면 가운데를 쓴다.
		const float WantHeight = Width / Aspect;
		if (WantHeight < Height)
		{
			Brush.ImageSize = FVector2D(Width, WantHeight);
			Brush.SetUVRegion(FBox2f(FVector2f(0.f, 0.f),
				FVector2f(1.f, WantHeight / Height)));
		}
		else
		{
			const float WantWidth = Height * Aspect;
			const float Margin = (1.f - WantWidth / Width) * 0.5f;
			Brush.ImageSize = FVector2D(WantWidth, Height);
			Brush.SetUVRegion(FBox2f(FVector2f(Margin, 0.f),
				FVector2f(1.f - Margin, 1.f)));
		}
		Image->SetBrush(Brush);
	}

}

namespace CombatDetailGrid
{
	// 빈 칸 / 조준 범위 / 타격 범위 / 시전자. 판에 칠하는 색과 같은 계열로 둔다.
	const FLinearColor Empty(0.10f, 0.085f, 0.065f, 0.55f);
	const FLinearColor Select(0.28f, 0.60f, 0.95f, 0.95f);
	const FLinearColor Hit(0.90f, 0.32f, 0.30f, 0.95f);
	const FLinearColor Caster(0.98f, 0.80f, 0.35f, 0.95f);

	/** @brief 시전자(가운데)에서 (Row,Column)까지 이 형태가 닿는가. */
	bool SelectCovers(ECombatSkillSelectShapeUI Shape, int32 Range, int32 dRow, int32 dColumn)
	{
		const int32 Manhattan = FMath::Abs(dRow) + FMath::Abs(dColumn);
		const int32 Chebyshev = FMath::Max(FMath::Abs(dRow), FMath::Abs(dColumn));
		switch (Shape)
		{
		case ECombatSkillSelectShapeUI::Single:   return dRow == 0 && dColumn == 0;
		case ECombatSkillSelectShapeUI::Cross:    return (dRow == 0 || dColumn == 0) && Manhattan <= Range;
		case ECombatSkillSelectShapeUI::Square:   return Chebyshev <= Range;
		case ECombatSkillSelectShapeUI::Diagonal: return Chebyshev <= Range
			&& (dRow == 0 || dColumn == 0 || FMath::Abs(dRow) == FMath::Abs(dColumn));
		case ECombatSkillSelectShapeUI::Line:     return (dRow == 0 || dColumn == 0) && Manhattan <= Range;
		default:                                  return false;
		}
	}

	bool HitCovers(ECombatSkillHitShapeUI Shape, int32 Range, int32 dRow, int32 dColumn)
	{
		const int32 Manhattan = FMath::Abs(dRow) + FMath::Abs(dColumn);
		const int32 Chebyshev = FMath::Max(FMath::Abs(dRow), FMath::Abs(dColumn));
		switch (Shape)
		{
		case ECombatSkillHitShapeUI::Single: return Manhattan == 0;
		case ECombatSkillHitShapeUI::Cross:  return (dRow == 0 || dColumn == 0) && Manhattan <= Range;
		// 8방향: 직교 축 또는 정확한 대각선. TileMapModel::GetEffectTiles의
		// Star(직교+대각 8방향)와 같은 모양 -- 전술판(SkillTacticalDiagram)과 동일 판정.
		case ECombatSkillHitShapeUI::Star:   return Chebyshev <= Range
			&& (dRow == 0 || dColumn == 0 || FMath::Abs(dRow) == FMath::Abs(dColumn));
		case ECombatSkillHitShapeUI::Circle: return Chebyshev <= Range;
		default:                             return false;
		}
	}
}

namespace SkillVisualLayout
{
	// 실제 검은 칠판(X 344~1574 / Y 254~817)을 좌우 두 열로 나눈다. 왼쪽은
	// 큰 스킬 아이콘과 4행 수치, 얇은 범위 버튼을 세로로 고정하고 오른쪽은
	// 긴 설명/전술 WBP가 같은 넓은 영역을 교대로 사용한다.
	const FVector2D StatIconPositions[] = {
		FVector2D(386.f, 526.f), FVector2D(386.f, 568.f),
		FVector2D(386.f, 610.f), FVector2D(386.f, 652.f) };
	const FVector2D StatTextPositions[] = {
		FVector2D(430.f, 525.f), FVector2D(430.f, 567.f),
		FVector2D(430.f, 609.f), FVector2D(430.f, 651.f) };
	const FVector2D StatIconSize(34.f, 34.f);
	const FVector2D StatTextSize(232.f, 36.f);
	// 40px 설계 높이는 1600x590(약 0.55배)에서 실제 22px까지 줄어
	// 라벨과 터치 영역 모두 지나치게 얇았다. 두 버튼을 하단 프레임 안에서
	// 56px로 키우고 간격을 다시 잡는다.
	const FVector2D SelectButtonPosition(368.f, 690.f);
	const FVector2D EffectButtonPosition(368.f, 752.f);
	const FVector2D RangeButtonSize(316.f, 56.f);
	const FVector2D DescriptionPosition(724.f, 278.f);
	const FVector2D DescriptionSize(778.f, 494.f);
	const FVector2D DividerPosition(694.f, 274.f);
	const FVector2D DividerSize(2.f, 510.f);
	// 설명문 아래, 아이콘 오른쪽 정보면의 정중앙. 5행을 모두 써도 하단 프레임과
	// 닫기 버튼을 침범하지 않는 크기다.
	const FVector2D GridOrigin(910.f, 530.f);
	const FVector2D CellSize(58.f, 42.f);
	const FVector2D CellStep(56.f, 44.f);
	constexpr int32 CasterRow = 2;
	constexpr int32 CasterColumn = 3;

	FSlateBrush MakeRangeButtonBrush(UTexture2D* Texture,
		const FLinearColor& Tint = FLinearColor::White)
	{
		FSlateBrush Brush;
		if (Texture != nullptr)
		{
			Brush.SetResourceObject(Texture);
			const FIntPoint ImportedSize = Texture->GetImportedSize();
			Brush.ImageSize = FVector2D(ImportedSize.X, ImportedSize.Y);
			Brush.DrawAs = ESlateBrushDrawType::Image;
		}
		else
		{
			Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
			Brush.OutlineSettings = FSlateBrushOutlineSettings(3.f,
				FSlateColor(FLinearColor(.69f, .43f, .16f, 1.f)), 1.25f);
		}
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}

	FButtonStyle MakeRangeButtonStyle(UTexture2D* NormalTexture,
		UTexture2D* SelectedTexture, const bool bSelected)
	{
		FSlateBrush Normal = MakeRangeButtonBrush(
			bSelected && SelectedTexture != nullptr ? SelectedTexture : NormalTexture);
		FSlateBrush Hovered = MakeRangeButtonBrush(
			SelectedTexture != nullptr ? SelectedTexture : NormalTexture,
			FLinearColor(1.04f, 1.04f, 1.04f, 1.f));
		FSlateBrush Pressed = Hovered;
		Pressed.TintColor = FSlateColor(FLinearColor(.80f, .88f, .90f, 1.f));
		FSlateBrush Disabled = MakeRangeButtonBrush(NormalTexture,
			FLinearColor(.48f, .48f, .48f, .72f));

		FButtonStyle Style;
		Style.SetNormal(Normal);
		Style.SetHovered(Hovered);
		Style.SetPressed(Pressed);
		Style.SetDisabled(Disabled);
		Style.SetNormalPadding(FMargin(0.f));
		// 버튼 PNG의 우측 화살표와 무관하게 라벨은 버튼 전체 폭의 정확한
		// 중앙에 둔다. 눌림 상태에서도 콘텐츠를 밀지 않는다.
		Style.SetPressedPadding(FMargin(0.f));
		return Style;
	}
}

USkillDetailOverlayPresenter::USkillDetailOverlayPresenter()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> SkillContentFinder(
		TEXT("/Game/UI/CombatDetail/WBP_SkillDetailContent"));
	if (SkillContentFinder.Succeeded())
	{
		mSkillDetailContentWidgetClass = SkillContentFinder.Class;
	}

	/*
	 * HUD 생성자와 같은 그림을 기본값으로 든다. 호스트가 SetVisualAssets로
	 * 덮으면 그쪽이 이긴다. 하드 참조라 전투 HUD 없이도 쿡에서 안 빠진다.
	 */
#define RD_LOAD_TEX(Field, Path) \
	{ \
		static ConstructorHelpers::FObjectFinder<UTexture2D> Finder(TEXT(Path)); \
		if (Finder.Succeeded()) { Field = Finder.Object; } \
	}
	RD_LOAD_TEX(mSkillVisualRingTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_StatChip_Ring.T_KitA_StatChip_Ring");
	RD_LOAD_TEX(mSkillVisualCellNormalTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Disabled.T_KitA_Cell_Disabled");
	RD_LOAD_TEX(mSkillVisualCellSelectedTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Selected.T_KitA_Cell_Selected");
	RD_LOAD_TEX(mSkillVisualAPIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/HUD04/KK_HUD04_zone_cost_badge.KK_HUD04_zone_cost_badge");
	RD_LOAD_TEX(mSkillVisualDamageIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/CombatDetail/SkillTactical/T_SkillStat_Damage_Simple_v2.T_SkillStat_Damage_Simple_v2");
	RD_LOAD_TEX(mSkillVisualCooldownIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/HUD04/KK_HUD04_zone_cooldown_badge.KK_HUD04_zone_cooldown_badge");
	RD_LOAD_TEX(mSkillVisualCriticalIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/CombatDetail/SkillTactical/T_SkillStat_Critical_Simple_v2.T_SkillStat_Critical_Simple_v2");
	RD_LOAD_TEX(mSkillVisualCasterIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsIcon_MercenaryGlyph.T_MB_OptionsIcon_MercenaryGlyph");
	RD_LOAD_TEX(mSkillVisualTargetIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsIcon_MonsterGlyph.T_MB_OptionsIcon_MonsterGlyph");
	RD_LOAD_TEX(mSkillRangeButtonTexture, "/Game/SVN/OutSideAsset/AICreation/UI/CombatDetail/SkillTactical/T_SkillRangeButton_Normal_v1.T_SkillRangeButton_Normal_v1");
	RD_LOAD_TEX(mSkillRangeButtonSelectedTexture, "/Game/SVN/OutSideAsset/AICreation/UI/CombatDetail/SkillTactical/T_SkillRangeButton_Selected_v1.T_SkillRangeButton_Selected_v1");
#undef RD_LOAD_TEX
}

void USkillDetailOverlayPresenter::Initialize(UWorld* World,
	TSubclassOf<UUserWidget> OverlayClass,
	TSubclassOf<USkillTacticalDiagramWidget> DiagramClass,
	const int32 ViewportZOrder)
{
	mWorld = World;
	mDetailOverlayWidgetClass = OverlayClass;
	mSkillTacticalDiagramWidgetClass = DiagramClass;
	mViewportZOrder = ViewportZOrder;
}

void USkillDetailOverlayPresenter::SetOverlayWidgetClass(
	TSubclassOf<UUserWidget> OverlayClass)
{
	if (mDetailOverlayWidget == nullptr)
	{
		mDetailOverlayWidgetClass = OverlayClass;
	}
}

void USkillDetailOverlayPresenter::SetReadableDetailFont(UFont* Font)
{
	mReadableDetailFont = Font;
}

void USkillDetailOverlayPresenter::SetVisualAssets(
	const FSkillDetailOverlayVisualAssets& Assets)
{
	/* 호스트가 안 채운 칸은 생성자 기본값을 보존한다. */
#define RD_TAKE_TEX(Field, Value) if ((Value) != nullptr) { Field = (Value); }
	RD_TAKE_TEX(mSkillVisualRingTexture, Assets.mRingTexture);
	RD_TAKE_TEX(mSkillVisualCellNormalTexture, Assets.mCellNormalTexture);
	RD_TAKE_TEX(mSkillVisualCellSelectedTexture, Assets.mCellSelectedTexture);
	RD_TAKE_TEX(mSkillVisualAPIconTexture, Assets.mAPIconTexture);
	RD_TAKE_TEX(mSkillVisualDamageIconTexture, Assets.mDamageIconTexture);
	RD_TAKE_TEX(mSkillVisualCooldownIconTexture, Assets.mCooldownIconTexture);
	RD_TAKE_TEX(mSkillVisualCriticalIconTexture, Assets.mCriticalIconTexture);
	RD_TAKE_TEX(mSkillVisualCasterIconTexture, Assets.mCasterIconTexture);
	RD_TAKE_TEX(mSkillVisualTargetIconTexture, Assets.mTargetIconTexture);
	RD_TAKE_TEX(mSkillRangeButtonTexture, Assets.mRangeButtonTexture);
	RD_TAKE_TEX(mSkillRangeButtonSelectedTexture, Assets.mRangeButtonSelectedTexture);
#undef RD_TAKE_TEX
}

/**
 * @brief 상세 패널 위젯을 처음 한 번 만들어 화면에 얹는다.
 *
 * @details
 * WBP_CombatDetailOverlay 는 배치와 그림(판·틀·글꼴)을 소유하고, 내용 글자는
 * 여기서 이름으로 찾은 위젯에 채운다 -- HUD 본체와 같은 규약이라 없는 위젯은
 * 조용히 건너뛴다.
 *
 * 겹은 HitTestInvisible 로 얹는다. 눌림을 받으면 화면 전체를 덮는 한 장이
 * 되어 판 탭이 HUD 까지 안 내려온다 -- 닫는 탭을 받을 사람이 사라진다.
 */
bool USkillDetailOverlayPresenter::EnsureOverlayWidget(
	APlayerController* OwningPlayer)
{
	if (mDetailOverlayWidget != nullptr)
	{
		return true;
	}
	if (mDetailOverlayWidgetClass == nullptr)
	{
		return false;
	}
	APlayerController* ResolvedOwningPlayer = OwningPlayer;
	if (ResolvedOwningPlayer == nullptr)
	{
		if (UWorld* World = mWorld.Get())
		{
			ResolvedOwningPlayer = World->GetFirstPlayerController();
		}
	}
	if (ResolvedOwningPlayer != nullptr)
	{
		mDetailOverlayWidget =
			CreateWidget<UUserWidget>(ResolvedOwningPlayer,
				mDetailOverlayWidgetClass);
	}
	else if (UWorld* World = mWorld.Get())
	{
		// 에디터 자동화처럼 로컬 플레이어가 없는 월드에서도 상세 계약은
		// 검증할 수 있어야 한다. 실제 플레이에서는 늘 위의 소유 플레이어를 쓴다.
		mDetailOverlayWidget =
			CreateWidget<UUserWidget>(World, mDetailOverlayWidgetClass);
	}
	if (mDetailOverlayWidget == nullptr)
	{
		return false;
	}
	// 몬스터 도감(55)에서 스킬을 오래 눌러도 상세는 반드시 그 위에 온다.
	mDetailOverlayWidget->AddToViewport(mViewportZOrder);
	mDetailOverlayWidget->SetVisibility(ESlateVisibility::Collapsed);

	mDetailIconImage = Cast<UImage>(mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailIconImage")));
	mDetailTitleText = Cast<UTextBlock>(mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailTitleText")));
	mDetailSubtitleText = Cast<UTextBlock>(mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailSubtitleText")));
	mDetailBodyText = Cast<UTextBlock>(mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailBodyText")));
	if (mDetailTitleText != nullptr && mDetailSubtitleText != nullptr
		&& mDetailBodyText != nullptr)
	{
		mDefaultDetailTitleFont = mDetailTitleText->GetFont();
		mDefaultDetailSubtitleFont = mDetailSubtitleText->GetFont();
		mDefaultDetailBodyFont = mDetailBodyText->GetFont();
		mDefaultDetailFontsCached = true;
	}
	BindDetailExtras();
	BuildSkillVisualPreview();

	/*
	 * 판·틀·글자는 눌림을 **삼키지 않게** 해 둔다.
	 *
	 * 스킬 칸이 생기면서 이 겹은 눌림을 받아야 하는데, 그러면 장식까지 눌림을
	 * 먹어서 패널 위를 톡 쳐도 닫히지 않는다. 눌림을 받을 것은 스킬 칸뿐이다.
	 */
	static const TCHAR* const DecorativeNames[] = {
		TEXT("DetailScrimBg"), TEXT("DetailScrimImage"),
		TEXT("DetailFrameImage"), TEXT("DetailIconImage"),
		TEXT("DetailTitleText"), TEXT("DetailSubtitleText"), TEXT("DetailBodyText") };
	for (const TCHAR* Name : DecorativeNames)
	{
		if (UWidget* Decoration = mDetailOverlayWidget->GetWidgetFromName(Name))
		{
			Decoration->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	if (UWidget* PanelRoot = mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailPanelRoot")))
	{
		// 자기는 안 받고 자식(스킬 칸)만 받는다.
		PanelRoot->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// 화면 전체 받이는 상세 뒤의 전투판 입력만 삼킨다. 상세의 빈 곳을 눌러도
	// 닫히지 않으며, 아래의 명시적인 "닫기" 버튼만 종료 경로로 사용한다.
	RDDetailOverlay::EnsureModalInputShield(mDetailOverlayWidget);
	// 확정 시안의 "닫기" 단추도 같은 곳으로 배선한다.
	if (UButton* CloseButton = Cast<UButton>(
		mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailCloseButton"))))
	{
		CloseButton->OnClicked.AddUniqueDynamic(
			this, &USkillDetailOverlayPresenter::HandleCloseClicked);
	}

	return true;
}

void USkillDetailOverlayPresenter::Teardown()
{
	if (mDetailOverlayWidget != nullptr)
	{
		mDetailOverlayWidget->RemoveFromParent();
		mDetailOverlayWidget = nullptr;
	}
	mSkillTacticalDiagramWidget = nullptr;
	mSkillContentWidget = nullptr;
	mSkillVisualPreview = nullptr;
	mSkillIconImage = nullptr;
	mSkillContentSwitcher = nullptr;
	mSkillSelectRangePlate = nullptr;
	mSkillEffectRangePlate = nullptr;
	mSkillWorldPreviewImage = nullptr;
	mArtifactDescriptionRoot = nullptr;
	mArtifactDescriptionScrollBox = nullptr;
	mArtifactDescriptionText = nullptr;
	mDetailDesignCanvas = nullptr;
	mDetailScaleDefaultsCached = false;
	mLastResponsiveViewportSize = FVector2D::ZeroVector;
	mLastResponsiveViewportScale = 0.f;
	mResponsiveDetailActive = false;
	mResponsiveDetailIsSkill = false;
	mDefaultDetailFontsCached = false;
}

void USkillDetailOverlayPresenter::HandleCloseClicked()
{
	// 전투 HUD 처럼 닫은 뒤 뒷정리(위협 범위 걷기)가 필요한 호출자는 여기에
	// 바인딩해 스스로 닫는다. 아무도 안 들으면 프레젠터가 직접 접는다.
	if (mOnCloseClicked.IsBound() == true)
	{
		mOnCloseClicked.Broadcast();
		return;
	}
	Dismiss();
}

void USkillDetailOverlayPresenter::Dismiss()
{
	if (IsShowing() == false)
	{
		return;
	}
	mDetailOverlayWidget->SetVisibility(ESlateVisibility::Collapsed);
	HideWorldPreviewImage();
}

bool USkillDetailOverlayPresenter::IsShowing() const
{
	return mDetailOverlayWidget != nullptr
		&& mDetailOverlayWidget->GetVisibility() != ESlateVisibility::Collapsed;
}

void USkillDetailOverlayPresenter::ApplyResponsiveSkillPanelScale(
	const bool bSkillDetail)
{
	mResponsiveDetailActive = true;
	mResponsiveDetailIsSkill = bSkillDetail;
	if (mDetailOverlayWidget == nullptr)
	{
		return;
	}

	if (mDetailDesignCanvas == nullptr)
	{
		mDetailDesignCanvas = Cast<UCanvasPanel>(
			mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailResponsiveCanvas")));
	}

	if (mDetailDesignCanvas == nullptr)
	{
		return;
	}
	if (mDetailScaleDefaultsCached == false)
	{
		mDefaultDetailDesignScale =
			mDetailDesignCanvas->GetRenderTransform().Scale;
		mDefaultDetailDesignTranslation =
			mDetailDesignCanvas->GetRenderTransform().Translation;
		mDefaultDetailDesignPivot = mDetailDesignCanvas->GetRenderTransformPivot();
		mDetailScaleDefaultsCached = true;
	}

	// 실제 반응형 배율은 WBP의 DetailResponsiveScale(ScaleToFit)이 담당한다.
	// 여기서 루트 Canvas를 다시 RenderScale 하면 DPI 배율과 중복되어 폴드/태블릿에서
	// 판이 커지고 닫기 버튼이 화면 밖으로 잘린다. 디자인 Canvas는 항상 원본 transform이다.
	mDetailDesignCanvas->SetRenderTransformPivot(mDefaultDetailDesignPivot);
	mDetailDesignCanvas->SetRenderTranslation(mDefaultDetailDesignTranslation);
	mDetailDesignCanvas->SetRenderScale(mDefaultDetailDesignScale);

	const FVector2D ViewportSize =
		UWidgetLayoutLibrary::GetViewportSize(mDetailOverlayWidget);
	if (mDetailOverlayWidget->GetOwningPlayer() == nullptr
		|| ViewportSize.X <= 0.f || ViewportSize.Y <= 0.f)
	{
		// WidgetRenderer 자동화처럼 소유 플레이어/GameViewport가 없는 경로에서는
		// 위치 보정을 계산할 수 없다. authored 1920 위치와 배율을 그대로 쓴다.
		return;
	}

	const float ViewportScale = FMath::Max(KINDA_SMALL_NUMBER,
		UWidgetLayoutLibrary::GetViewportScale(mDetailOverlayWidget));
	const float ResponsiveScale = bSkillDetail
		? CalculateResponsiveSkillPanelScale(ViewportSize, ViewportScale)
		: CalculateResponsiveDetailPanelScale(ViewportSize, ViewportScale);
	const FVector2D LogicalViewport = ViewportSize / ViewportScale;
	const FVector2D DesignSize(1920.f, 1080.f);
	const FVector2D PresentedDesignSize = DesignSize * ResponsiveScale;
	mLastResponsiveViewportSize = ViewportSize;
	mLastResponsiveViewportScale = ViewportScale;
	mDetailOverlayWidget->ForceLayoutPrepass();
	UE_LOG(LogTemp, Display,
		TEXT("RD_DETAIL_RESPONSIVE viewport=%.0fx%.0f dpi=%.3f logical=%.0fx%.0f fit=%.3f letterbox=%.0f,%.0f skill=%d"),
		ViewportSize.X, ViewportSize.Y, ViewportScale,
		LogicalViewport.X, LogicalViewport.Y, ResponsiveScale,
		(LogicalViewport.X - PresentedDesignSize.X) * .5f,
		(LogicalViewport.Y - PresentedDesignSize.Y) * .5f,
		bSkillDetail ? 1 : 0);
}

float USkillDetailOverlayPresenter::CalculateResponsiveDetailPanelScale(
	const FVector2D ViewportSize, const float ViewportScale)
{
	if (ViewportSize.X <= 0.f || ViewportSize.Y <= 0.f
		|| ViewportScale <= KINDA_SMALL_NUMBER)
	{
		return 1.f;
	}
	const FVector2D LogicalViewport = ViewportSize / ViewportScale;
	const float FitScale = FMath::Min(LogicalViewport.X / 1920.f,
		LogicalViewport.Y / 1080.f);
	// 4K에서 원화를 무한히 키우지는 않되 폴드의 2176 폭은 자연스럽게 채운다.
	return FMath::Clamp(FitScale, .5f, 1.25f);
}

float USkillDetailOverlayPresenter::CalculateResponsiveSkillPanelScale(
	const FVector2D ViewportSize, const float ViewportScale)
{
	// 스킬도 닫기 단추와 설명 스크롤까지 전부 보여야 한다. 판 내부만 기준으로
	// 다시 키우면 폴드에서 1.5배가 되어 하단 조작부가 화면 밖으로 밀린다.
	return CalculateResponsiveDetailPanelScale(ViewportSize, ViewportScale);
}

void USkillDetailOverlayPresenter::RefreshResponsiveLayout()
{
	if (mResponsiveDetailActive == false || IsShowing() == false
		|| mDetailOverlayWidget == nullptr
		|| mDetailOverlayWidget->GetOwningPlayer() == nullptr)
	{
		return;
	}
	const FVector2D ViewportSize =
		UWidgetLayoutLibrary::GetViewportSize(mDetailOverlayWidget);
	const float ViewportScale = FMath::Max(KINDA_SMALL_NUMBER,
		UWidgetLayoutLibrary::GetViewportScale(mDetailOverlayWidget));
	if (ViewportSize.Equals(mLastResponsiveViewportSize, .5f)
		&& FMath::IsNearlyEqual(ViewportScale, mLastResponsiveViewportScale, .001f))
	{
		return;
	}
	ApplyResponsiveSkillPanelScale(mResponsiveDetailIsSkill);
}

void USkillDetailOverlayPresenter::ApplyReadableDetailTypography(const bool bReadable)
{
	if (mDefaultDetailFontsCached == false)
	{
		return;
	}
	if (bReadable && mReadableDetailFont == nullptr)
	{
		mReadableDetailFont = LoadObject<UFont>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/Fonts/GowunBatang/F_GowunBatang.F_GowunBatang"));
	}
	auto ApplyPrimary = [this, bReadable](UTextBlock* Text,
		const FSlateFontInfo& DefaultFont, const bool bBold)
	{
		if (Text == nullptr)
		{
			return;
		}
		FSlateFontInfo Font = DefaultFont;
		if (bReadable && mReadableDetailFont != nullptr)
		{
			Font.FontObject = mReadableDetailFont;
			Font.TypefaceFontName = bBold ? TEXT("Bold") : TEXT("Regular");
		}
		Text->SetFont(Font);
	};
	auto ApplyDynamic = [this, bReadable](UTextBlock* Text, const bool bBold)
	{
		if (Text == nullptr)
		{
			return;
		}
		FSlateFontInfo Font = Text->GetFont();
		if (bReadable && mReadableDetailFont != nullptr)
		{
			Font.FontObject = mReadableDetailFont;
			Font.TypefaceFontName = bBold ? TEXT("Bold") : TEXT("Regular");
		}
		else
		{
			Font.FontObject = mDefaultDetailBodyFont.FontObject;
			Font.TypefaceFontName = mDefaultDetailBodyFont.TypefaceFontName;
		}
		Text->SetFont(Font);
	};

	ApplyPrimary(mDetailTitleText, mDefaultDetailTitleFont, true);
	ApplyPrimary(mDetailSubtitleText, mDefaultDetailSubtitleFont, true);
	ApplyPrimary(mDetailBodyText, mDefaultDetailBodyFont, false);
	ApplyDynamic(mSkillDescriptionText, false);
	ApplyDynamic(mArtifactDescriptionText, false);
	for (UTextBlock* StatText : mSkillVisualStatTexts)
	{
		ApplyDynamic(StatText, true);
	}
	ApplyDynamic(mSkillSelectRangeText, true);
	ApplyDynamic(mSkillEffectRangeText, true);
}

/**
 * @brief 상세창의 수치 칩과 범위 칸을 이름으로 찾아 둔다.
 *
 * @details 없으면 그냥 건너뛴다. 옛 WBP 처럼 칩·칸이 없는 판도 그대로 돌아가야
 * 한다 -- 상세창은 화면 담당이 갈아 끼우는 자산이라 이쪽이 강제하지 않는다.
 */
void USkillDetailOverlayPresenter::BindDetailExtras()
{
	mDetailChipLabels.Reset();
	mDetailChipValues.Reset();
	for (int32 ChipSlot = 0; ChipSlot < DetailChipCount; ++ChipSlot)
	{
		mDetailChipLabels.Add(Cast<UTextBlock>(mDetailOverlayWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("DetailChip%dLabel"), ChipSlot)))));
		mDetailChipValues.Add(Cast<UTextBlock>(mDetailOverlayWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("DetailChip%dValue"), ChipSlot)))));
	}

	mDetailSelectCells.Reset();
	mDetailHitCells.Reset();
	for (int32 Row = 0; Row < DetailGridExtent; ++Row)
	{
		for (int32 Column = 0; Column < DetailGridExtent; ++Column)
		{
			mDetailSelectCells.Add(Cast<UImage>(mDetailOverlayWidget->GetWidgetFromName(
				FName(*FString::Printf(TEXT("DetailSelectCell_R%dC%d"), Row, Column)))));
			mDetailHitCells.Add(Cast<UImage>(mDetailOverlayWidget->GetWidgetFromName(
				FName(*FString::Printf(TEXT("DetailHitCell_R%dC%d"), Row, Column)))));
		}
	}

	// 0822 확정: 조준/효과 차단 글줄 설명은 걷는다. WBP 에 라벨이 구워져
	// 있으므로 이름으로 찾아 접는다(계약 이름은 남긴다).
	for (const TCHAR* BlockerName :
		{ TEXT("DetailAimBlockerText"), TEXT("DetailEffectBlockerText") })
	{
		if (UWidget* Blocker = mDetailOverlayWidget->GetWidgetFromName(BlockerName))
		{
			Blocker->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	mDetailSelectCaptionText = Cast<UTextBlock>(
		mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailSelectCaptionText")));
	mDetailHitCaptionText = Cast<UTextBlock>(
		mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailHitCaptionText")));

	mDetailStatBlock = mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailStatBlock"));
	mDetailTargetBlock = mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailTargetBlock"));
	mDetailSkillBlock = mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailSkillBlock"));
	mDetailExtraBlock = mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailExtraBlock"));
	mDetailExtraHeading = Cast<UTextBlock>(
		mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailExtraHeading")));
	mDetailExtraText = Cast<UTextBlock>(
		mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailExtraText")));
	mDetailIdentityColumn = mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailIdentityColumn"));
	mDetailStatColumn = mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailStatColumn"));
	mDetailRightColumn = mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailRightColumn"));
	mDetailWideColumn = mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailWideColumn"));
	mDetailDivider0 = mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailDivider_0"));
	mDetailDivider1 = mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailDivider_1"));
}

/**
 * @brief 오른쪽 열의 세 덩어리 중 하나만 켠다.
 * @param Wanted 켤 덩어리. nullptr 이면 셋 다 끈다
 */
void USkillDetailOverlayPresenter::ShowDetailRightBlock(const UWidget* Wanted)
{
	UWidget* const Blocks[] = { mDetailTargetBlock, mDetailSkillBlock, mDetailExtraBlock };
	for (UWidget* Block : Blocks)
	{
		if (Block != nullptr)
		{
			DetailSetShown(Block, Block == Wanted);
		}
	}
}

/**
 * @brief 상세 종류에 맞춰 열 자체를 재배치한다.
 *
 * @details 루트만 화면 전체에 앵커링하고 자식을 1920 좌표에 고정하면 20:9에서
 * 오른쪽이 비거나 늘어난다. 열을 화면 비율 앵커로 두면 폭이 달라져도 비율을
 * 유지한다. 아티팩트는 값만 숨기는 것이 아니라 가운데 열과 두 번째 기둥을
 * 접고, 정체성/효과 열을 34:66으로 다시 펼친다.
 */
void USkillDetailOverlayPresenter::ApplyDetailColumnLayout(const bool bArtifactTwoColumn)
{
	// 유닛/상태/아티팩트가 스킬 뒤에 같은 WBP를 재사용할 때 스킬 전용 DPI
	// 보정을 물려받지 않게 공용 배치 진입점에서 authored 배율로 먼저 돌린다.
	ApplyResponsiveSkillPanelScale(false);
	// 스킬 전용 런타임 도식은 다른 상세 화면으로 넘어갈 때 반드시 먼저 걷는다.
	SetSkillVisualPreviewShown(false);
	// 아티팩트 전용 효과 스크롤도 같은 규칙 -- 다음 상세가 이어받지 않게 걷는다.
	DetailSetShown(mArtifactDescriptionRoot, false);
	/*
	 * 자리는 **판이 정한다.** 여기서는 켜고 끄기만 한다.
	 *
	 * 전에는 이 함수가 열마다 앵커 비율을 제 손으로 적어 두고 다시 앉혔다
	 * (0.034 / 0.227 / 0.278 …). 그래서 배치 빌더에서 열 비율을 고치면
	 * 화면은 안 따라오고, 두 값이 어긋난 채로 남았다. 자리를 재는 곳이
	 * 둘이면 어느 쪽이 맞는지 늘 따지게 된다.
	 *
	 * 이제 판이 세 열과 "가운데+오른쪽을 이은 넓은 열" 을 모두 만들어 두고,
	 * 여기서는 화면에 맞는 쪽을 편다.
	 */
	DetailSetShown(mDetailStatColumn, bArtifactTwoColumn == false);
	DetailSetShown(mDetailRightColumn, bArtifactTwoColumn == false);
	DetailSetShown(mDetailWideColumn, bArtifactTwoColumn);

	// 확정 시안(0806)의 기본 모습으로 되돌린다. 아티팩트만 이 뒤에서
	// 받침판을 걷고 보석 줄을 켠다 -- 한 번 켠 것이 다음 화면에 남지 않게.
	if (mDetailOverlayWidget != nullptr)
	{
		// 아티팩트/스킬이 각각 바꾼 아이콘 크기와 메타 위치를 다음 상세가
		// 이어받지 않도록 authored WBP 기본 좌표부터 복원한다.
		SetCanvasRect(mDetailOverlayWidget->GetWidgetFromName(
			TEXT("DetailIconFrame")), FVector2D(317.f, 225.f),
			FVector2D(300.f, 300.f));
		SetCanvasRect(mDetailIconImage, FVector2D(365.f, 273.f),
			FVector2D(204.f, 204.f));
		DetailSetShown(mDetailOverlayWidget->GetWidgetFromName(
			TEXT("DetailIconFrame")), true);
		DetailSetShown(mDetailIconImage, true);
		SetCanvasRect(mDetailOverlayWidget->GetWidgetFromName(
			TEXT("DetailSubtitleText_Center")), FVector2D(611.f, 231.f),
			FVector2D(910.f, 106.f));
		// 스킬/아티팩트가 접어 둔 메타 정보 판과 밝은 글자색을 다른 상세가
		// 이어받지 않도록 공용 기본 상태부터 복구한다.
		DetailSetShown(mDetailOverlayWidget->GetWidgetFromName(
			TEXT("DetailIdentityPlate")), true);
		DetailSetShown(mDetailOverlayWidget->GetWidgetFromName(
			TEXT("DetailStatsPlate")), true);
		if (mDetailSubtitleText != nullptr)
		{
			DetailSetShown(mDetailSubtitleText, true);
			mDetailSubtitleText->SetColorAndOpacity(FSlateColor(
				FLinearColor(0.16f, 0.08f, 0.035f, 1.f)));
		}
		// 아티팩트가 구분선 아래로 넓혀 쓴 설명 칸을 다른 상세가 이어받지 않게
		// 공용 기본 위치로 먼저 되돌린다.
		if (mDetailBodyText != nullptr)
		{
			if (UWidget* BodyMount = mDetailBodyText->GetParent())
			{
				if (UCanvasPanelSlot* BodySlot = Cast<UCanvasPanelSlot>(BodyMount->Slot))
				{
					BodySlot->SetPosition(FVector2D(631.f, 347.f));
					BodySlot->SetSize(FVector2D(844.f, 158.f));
				}
			}
			mDetailBodyText->SetLineHeightPercentage(1.f);
		}
		if (UWidget* FreePlate = mDetailOverlayWidget->GetWidgetFromName(
			TEXT("DetailFreePlate")))
		{
			FreePlate->SetVisibility(bArtifactTwoColumn
				? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}
		DetailSetShown(mDetailBodyText, true);
		for (int32 Index = 0; Index < 5; ++Index)
		{
			DetailSetShown(mDetailOverlayWidget->GetWidgetFromName(
				FName(*FString::Printf(TEXT("DetailRarityGem_%d"), Index))), false);
		}
	}
	// 기둥 하나는 정체성 열과의 사이, 다른 하나는 가운데와 오른쪽 사이다.
	// 두 열을 이어 붙이면 그 사이 기둥은 판 한가운데를 가로지른다.
	DetailSetShown(mDetailDivider0, true);
	DetailSetShown(mDetailDivider1, bArtifactTwoColumn == false);
}

void USkillDetailOverlayPresenter::SetDetailChip(int32 ChipSlot, const FText& Label, const FText& Value)
{
	if (mDetailChipLabels.IsValidIndex(ChipSlot) == true)
	{
		DetailSetTextIfPresent(mDetailChipLabels[ChipSlot], Label);
	}
	if (mDetailChipValues.IsValidIndex(ChipSlot) == true)
	{
		DetailSetTextIfPresent(mDetailChipValues[ChipSlot], Value);
	}
}

void USkillDetailOverlayPresenter::ClearDetailChips()
{
	for (int32 ChipSlot = 0; ChipSlot < DetailChipCount; ++ChipSlot)
	{
		SetDetailChip(ChipSlot, FText::GetEmpty(), FText::FromString(TEXT("-")));
	}
}

FString USkillDetailOverlayPresenter::GetChipValueString(const int32 ChipIndex) const
{
	return mDetailChipValues.IsValidIndex(ChipIndex)
		&& mDetailChipValues[ChipIndex] != nullptr
		? mDetailChipValues[ChipIndex]->GetText().ToString() : FString();
}

void USkillDetailOverlayPresenter::PaintSelectGrid(const FSkillTargetingUI& Targeting)
{
	using namespace CombatDetailGrid;
	const int32 Center = DetailGridExtent / 2;
	const int32 Range = FMath::RoundToInt(Targeting.mSelectRange);
	for (int32 Row = 0; Row < DetailGridExtent; ++Row)
	{
		for (int32 Column = 0; Column < DetailGridExtent; ++Column)
		{
			UImage* Cell = mDetailSelectCells.IsValidIndex(Row * DetailGridExtent + Column)
				? mDetailSelectCells[Row * DetailGridExtent + Column].Get() : nullptr;
			if (Cell == nullptr)
			{
				continue;
			}
			const int32 dRow = Row - Center;
			const int32 dColumn = Column - Center;
			const bool bCaster = (dRow == 0 && dColumn == 0);
			const bool bCovered = SelectCovers(Targeting.mSelectShape, Range, dRow, dColumn);
			Cell->SetColorAndOpacity(bCaster ? Caster : (bCovered ? Select : Empty));
		}
	}
}

void USkillDetailOverlayPresenter::PaintHitGrid(const FSkillTargetingUI& Targeting)
{
	using namespace CombatDetailGrid;
	const int32 Center = DetailGridExtent / 2;
	const int32 Range = FMath::RoundToInt(Targeting.mHitRange);
	for (int32 Row = 0; Row < DetailGridExtent; ++Row)
	{
		for (int32 Column = 0; Column < DetailGridExtent; ++Column)
		{
			UImage* Cell = mDetailHitCells.IsValidIndex(Row * DetailGridExtent + Column)
				? mDetailHitCells[Row * DetailGridExtent + Column].Get() : nullptr;
			if (Cell == nullptr)
			{
				continue;
			}
			const int32 dRow = Row - Center;
			const int32 dColumn = Column - Center;
			const bool bCaster = (dRow == 0 && dColumn == 0);
			const bool bCovered = HitCovers(Targeting.mHitShape, Range, dRow, dColumn);
			Cell->SetColorAndOpacity(bCaster ? Caster : (bCovered ? Hit : Empty));
		}
	}
}

void USkillDetailOverlayPresenter::ClearDetailGrids()
{
	using namespace CombatDetailGrid;
	for (TObjectPtr<UImage>& Cell : mDetailSelectCells)
	{
		if (Cell != nullptr)
		{
			Cell->SetColorAndOpacity(Empty);
		}
	}
	for (TObjectPtr<UImage>& Cell : mDetailHitCells)
	{
		if (Cell != nullptr)
		{
			Cell->SetColorAndOpacity(Empty);
		}
	}
	DetailSetTextIfPresent(mDetailSelectCaptionText, FText::GetEmpty());
	DetailSetTextIfPresent(mDetailHitCaptionText, FText::GetEmpty());
}

bool USkillDetailOverlayPresenter::BindAuthoredSkillContent()
{
	if (mSkillContentWidget != nullptr)
	{
		return mSkillVisualPreview != nullptr;
	}
	if (mDetailOverlayWidget == nullptr)
	{
		return false;
	}
	if (mSkillDetailContentWidgetClass == nullptr)
	{
		mSkillDetailContentWidgetClass = LoadClass<UUserWidget>(nullptr,
			TEXT("/Game/UI/CombatDetail/WBP_SkillDetailContent."
				"WBP_SkillDetailContent_C"));
	}
	if (mSkillDetailContentWidgetClass == nullptr)
	{
		return false;
	}

	if (APlayerController* Owner = mDetailOverlayWidget->GetOwningPlayer())
	{
		mSkillContentWidget = CreateWidget<UUserWidget>(Owner,
			mSkillDetailContentWidgetClass);
	}
	else if (UWorld* World = mWorld.Get())
	{
		mSkillContentWidget = CreateWidget<UUserWidget>(World,
			mSkillDetailContentWidgetClass);
	}
	if (mSkillContentWidget == nullptr)
	{
		return false;
	}

	UCanvasPanel* Host = Cast<UCanvasPanel>(
		mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailResponsiveCanvas")));
	if (Host == nullptr)
	{
		Host = Cast<UCanvasPanel>(
			mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailPanelRoot")));
	}
	if (Host == nullptr)
	{
		mSkillContentWidget = nullptr;
		return false;
	}
	if (UCanvasPanelSlot* Slot = Host->AddChildToCanvas(mSkillContentWidget))
	{
		// 이 한 사각형만 공용 프레임 좌표에 둔다. 내부 배치는 전부
		// WBP_SkillDetailContent의 1230x563 디자이너가 소유한다.
		Slot->SetAnchors(FAnchors(0.f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetPosition(FVector2D(344.f, 254.f));
		Slot->SetSize(FVector2D(1230.f, 563.f));
		Slot->SetZOrder(40);
	}

	mSkillVisualPreview = Cast<UCanvasPanel>(
		mSkillContentWidget->GetWidgetFromName(TEXT("SkillDetailContentRoot")));
	mSkillIconImage = Cast<UImage>(
		mSkillContentWidget->GetWidgetFromName(TEXT("SkillIconImage")));
	mSkillDescriptionScrollBox = Cast<UScrollBox>(
		mSkillContentWidget->GetWidgetFromName(TEXT("SkillDescriptionScroll")));
	mSkillDescriptionText = Cast<UTextBlock>(
		mSkillContentWidget->GetWidgetFromName(TEXT("SkillDescriptionText")));
	mSkillContentSwitcher = Cast<UWidgetSwitcher>(
		mSkillContentWidget->GetWidgetFromName(TEXT("SkillContentSwitcher")));
	mSkillSelectRangeButton = Cast<UButton>(
		mSkillContentWidget->GetWidgetFromName(TEXT("SkillSelectRangeButton")));
	mSkillEffectRangeButton = Cast<UButton>(
		mSkillContentWidget->GetWidgetFromName(TEXT("SkillEffectRangeButton")));
	mSkillSelectRangeText = Cast<UTextBlock>(
		mSkillContentWidget->GetWidgetFromName(TEXT("SkillSelectRangeText")));
	mSkillEffectRangeText = Cast<UTextBlock>(
		mSkillContentWidget->GetWidgetFromName(TEXT("SkillEffectRangeText")));
	mSkillSelectRangePlate = Cast<UImage>(
		mSkillContentWidget->GetWidgetFromName(TEXT("SkillSelectRangePlate")));
	mSkillEffectRangePlate = Cast<UImage>(
		mSkillContentWidget->GetWidgetFromName(TEXT("SkillEffectRangePlate")));
	mSkillWorldPreviewImage = Cast<UImage>(
		mSkillContentWidget->GetWidgetFromName(TEXT("SkillWorldPreview")));

	auto ResolveHudTexture = [this](const FName SourceWidgetName,
		UTexture2D* Fallback) -> UTexture2D*
	{
		if (mStatTextureResolver.IsBound())
		{
			if (UTexture2D* Texture = mStatTextureResolver.Execute(
				SourceWidgetName, Fallback))
			{
				return Texture;
			}
		}
		return Fallback;
	};
	UTexture2D* StatTextures[SkillPreviewStatCount] = {
		ResolveHudTexture(TEXT("CommandCostBadge_0"), mSkillVisualAPIconTexture),
		mSkillVisualDamageIconTexture,
		ResolveHudTexture(TEXT("CommandCooldownBadge_0"),
			mSkillVisualCooldownIconTexture),
		mSkillVisualCriticalIconTexture };
	mSkillVisualStatIcons.Reset();
	mSkillVisualStatTexts.Reset();
	for (int32 Index = 0; Index < SkillPreviewStatCount; ++Index)
	{
		UImage* Icon = Cast<UImage>(mSkillContentWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("SkillStatIcon_%d"), Index))));
		if (Icon != nullptr && StatTextures[Index] != nullptr)
		{
			Icon->SetBrushFromTexture(StatTextures[Index], false);
		}
		mSkillVisualStatIcons.Add(Icon);
		mSkillVisualStatTexts.Add(Cast<UTextBlock>(
			mSkillContentWidget->GetWidgetFromName(FName(*FString::Printf(
				TEXT("SkillStatText_%d"), Index)))));
	}

	if (mSkillSelectRangeButton != nullptr)
	{
		mSkillSelectRangeButton->OnClicked.AddUniqueDynamic(this,
			&USkillDetailOverlayPresenter::HandleSkillSelectRangeButtonClicked);
	}
	if (mSkillEffectRangeButton != nullptr)
	{
		mSkillEffectRangeButton->OnClicked.AddUniqueDynamic(this,
			&USkillDetailOverlayPresenter::HandleSkillEffectRangeButtonClicked);
	}

	if (mSkillTacticalDiagramWidgetClass != nullptr)
	{
		if (APlayerController* Owner = mDetailOverlayWidget->GetOwningPlayer())
		{
			mSkillTacticalDiagramWidget =
				CreateWidget<USkillTacticalDiagramWidget>(Owner,
					mSkillTacticalDiagramWidgetClass);
		}
		else if (UWorld* World = mWorld.Get())
		{
			mSkillTacticalDiagramWidget =
				CreateWidget<USkillTacticalDiagramWidget>(World,
					mSkillTacticalDiagramWidgetClass);
		}
		UOverlay* TacticalHost = Cast<UOverlay>(
			mSkillContentWidget->GetWidgetFromName(TEXT("SkillTacticalHost")));
		if (mSkillTacticalDiagramWidget != nullptr && TacticalHost != nullptr)
		{
			TacticalHost->AddChildToOverlay(mSkillTacticalDiagramWidget);
			if (UOverlaySlot* Slot = Cast<UOverlaySlot>(
				mSkillTacticalDiagramWidget->Slot))
			{
				Slot->SetPadding(FMargin(0.f));
				Slot->SetHorizontalAlignment(HAlign_Fill);
				Slot->SetVerticalAlignment(VAlign_Fill);
			}
			mSkillTacticalDiagramWidget->OnPreviewVisibilityChanged().AddUObject(
				this, &USkillDetailOverlayPresenter::
					HandleSkillTacticalPreviewVisibilityChanged);
			mSkillTacticalDiagramWidget->SetVisibility(
				ESlateVisibility::SelfHitTestInvisible);
			for (const TCHAR* WidgetName : {
				TEXT("TacticalDiagramBackdrop"), TEXT("TacticalLegendRule"),
				TEXT("TacticalSelectLegendButton"),
				TEXT("TacticalEffectLegendButton") })
			{
				DetailSetShown(
					mSkillTacticalDiagramWidget->GetWidgetFromName(WidgetName), false);
			}
		}
	}

	if (mSkillContentSwitcher != nullptr)
	{
		mSkillContentSwitcher->SetActiveWidgetIndex(0);
	}
	RefreshSkillRangeButtonStyles();
	DetailSetShown(mSkillVisualPreview, false);
	UE_LOG(LogTemp, Display,
		TEXT("RD_SKILL_DETAIL_CONTENT bound asset=%s host=%s authored=1"),
		*mSkillContentWidget->GetClass()->GetPathName(), *Host->GetName());
	return mSkillVisualPreview != nullptr;
}

/** @brief 이미지 시안의 수치 메달/통합 전술 보드를 실제 상세 WBP 인스턴스에 짓는다. */
void USkillDetailOverlayPresenter::BuildSkillVisualPreview()
{
	if (BindAuthoredSkillContent())
	{
		return;
	}
	if (mSkillVisualPreview != nullptr || mDetailOverlayWidget == nullptr
		|| mDetailOverlayWidget->WidgetTree == nullptr)
	{
		return;
	}
	UWidgetTree* Tree = mDetailOverlayWidget->WidgetTree;
	/*
	 * 런타임 내용도 프레임과 **같은 디자인 캔버스**에 앉혀야 한다.
	 *
	 * DetailPanelRoot는 뷰포트 전체 크기다. 16:9에서는 우연히 1920x1080이라
	 * 아래의 디자인 좌표가 맞았지만, 폴드의 1296x1080에서는 프레임만 안쪽
	 * ScaleBox에서 축소되고 런타임 내용은 바깥 좌표를 그대로 써 서로 어긋났다.
	 * 프레임의 실제 부모 캔버스를 찾으면 두 묶음이 같은 ScaleBox 변환을 받는다.
	 */
	UCanvasPanel* Host = nullptr;
	UWidget* Frame = mDetailOverlayWidget->GetWidgetFromName(
		TEXT("DetailFrameImage"));
	if (Frame != nullptr)
	{
		for (UPanelWidget* Parent = Frame->GetParent(); Parent != nullptr;
			Parent = Parent->GetParent())
		{
			if (UCanvasPanel* FrameCanvas = Cast<UCanvasPanel>(Parent))
			{
				Host = FrameCanvas;
				break;
			}
		}
	}
	if (Host == nullptr)
	{
		Host = Cast<UCanvasPanel>(
			mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailPanelRoot")));
	}
	if (Host == nullptr)
	{
		Host = Cast<UCanvasPanel>(Tree->RootWidget);
	}
	if (Host == nullptr)
	{
		return;
	}
	UE_LOG(LogTemp, Display,
		TEXT("RD_SKILL_DETAIL_HOST host=%s frameParent=%s"),
		*Host->GetName(),
		Frame != nullptr && Frame->GetParent() != nullptr
			? *Frame->GetParent()->GetName() : TEXT("(none)"));

	/*
	 * 폴드처럼 화면이 16:9가 아니어도 이 묶음의 좌표계는 언제나 1920x1080이다.
	 * 외곽 WBP가 사용하는 것과 같은 ScaleToFit 계약을 런타임 자식에도 명시한다.
	 * ScaleBox 없이 Canvas만 화면에 Fill하면 X는 1296, Y는 1080인 서로 다른
	 * 축척 위에 1920 기준 좌표를 찍게 되어 제목 위로 설명이 올라가고 아이콘과
	 * 수치행이 포개졌다.
	 */
	UScaleBox* DesignScale = Tree->ConstructWidget<UScaleBox>(
		UScaleBox::StaticClass(), TEXT("RuntimeSkillDesignScale"));
	if (UCanvasPanelSlot* ScaleSlot = Host->AddChildToCanvas(DesignScale))
	{
		ScaleSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		ScaleSlot->SetOffsets(FMargin(0.f));
		ScaleSlot->SetAlignment(FVector2D::ZeroVector);
		ScaleSlot->SetZOrder(40);
	}
	DesignScale->SetStretch(EStretch::ScaleToFit);
	DesignScale->SetStretchDirection(EStretchDirection::Both);
	DesignScale->SetClipping(EWidgetClipping::ClipToBounds);

	USizeBox* DesignSize = Tree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("RuntimeSkillDesignSize"));
	DesignSize->SetWidthOverride(1920.f);
	DesignSize->SetHeightOverride(1080.f);
	DesignScale->AddChild(DesignSize);
	if (UScaleBoxSlot* DesignSlot = Cast<UScaleBoxSlot>(DesignSize->Slot))
	{
		DesignSlot->SetHorizontalAlignment(HAlign_Center);
		DesignSlot->SetVerticalAlignment(VAlign_Center);
	}

	mSkillVisualPreview = Tree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("RuntimeSkillVisualPreview"));
	DesignSize->SetContent(mSkillVisualPreview);
	mSkillVisualPreview->SetVisibility(ESlateVisibility::Collapsed);

	auto AddImage = [this, Tree](const FName Name, UTexture2D* Texture,
		const FVector2D Position, const FVector2D Size, const int32 Z,
		const FLinearColor Color = FLinearColor::White) -> UImage*
	{
		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		if (UCanvasPanelSlot* Slot = mSkillVisualPreview->AddChildToCanvas(Image))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
			Slot->SetZOrder(Z);
		}
		if (Texture != nullptr)
		{
			Image->SetBrushFromTexture(Texture, false);
		}
		Image->SetColorAndOpacity(Color);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Image;
	};
	auto AddText = [this, Tree](const FName Name, const FVector2D Position,
		const FVector2D Size, const int32 FontSize, const FLinearColor Color,
		const ETextJustify::Type Justification = ETextJustify::Left) -> UTextBlock*
	{
		UTextBlock* TextBlock = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), Name);
		if (UCanvasPanelSlot* Slot = mSkillVisualPreview->AddChildToCanvas(TextBlock))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
			Slot->SetZOrder(8);
		}
		FSlateFontInfo Font = mDetailBodyText != nullptr
			? mDetailBodyText->GetFont() : FSlateFontInfo();
		Font.Size = FontSize;
		TextBlock->SetFont(Font);
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetJustification(Justification);
		TextBlock->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
		TextBlock->SetShadowOffset(FVector2D(1.5f, 1.5f));
		TextBlock->SetVisibility(ESlateVisibility::HitTestInvisible);
		return TextBlock;
	};
	auto AddRule = [this, Tree](const FName Name, const FVector2D Position,
		const FVector2D Size, const FLinearColor Color, const int32 Z) -> UBorder*
	{
		UBorder* Rule = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		if (UCanvasPanelSlot* Slot = mSkillVisualPreview->AddChildToCanvas(Rule))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
			Slot->SetZOrder(Z);
		}
		Rule->SetBrushColor(Color);
		Rule->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Rule;
	};

	// 시안의 세로 황동 구분선을 실제 칠판 안쪽에만 둔다. 외곽 WBP의 하단
	// 장식선은 건드리지 않으므로 다른 상세 화면과 구조를 공유할 수 있다.
	AddRule(TEXT("RuntimeSkillColumnDivider"), SkillVisualLayout::DividerPosition,
		SkillVisualLayout::DividerSize, FLinearColor(0.72f, 0.46f, 0.18f, 0.72f), 5);

	// 설명은 스킬마다 길이가 달라진다. 공용 DetailBodyText의 고정 높이를
	// 억지로 늘리지 않고, 스킬 상태에서만 오른쪽을 실제 ScrollBox로 쓴다.
	mSkillDescriptionScrollBox = Tree->ConstructWidget<UScrollBox>(
		UScrollBox::StaticClass(), TEXT("RuntimeSkillDescriptionScroll"));
	if (UCanvasPanelSlot* ScrollSlot = mSkillVisualPreview->AddChildToCanvas(
		mSkillDescriptionScrollBox))
	{
		ScrollSlot->SetPosition(SkillVisualLayout::DescriptionPosition);
		ScrollSlot->SetSize(SkillVisualLayout::DescriptionSize);
		ScrollSlot->SetZOrder(7);
	}
	mSkillDescriptionScrollBox->SetScrollBarVisibility(ESlateVisibility::Visible);
	mSkillDescriptionScrollBox->SetScrollbarThickness(FVector2D(5.f, 5.f));
	mSkillDescriptionScrollBox->SetAlwaysShowScrollbar(false);
	mSkillDescriptionScrollBox->SetAlwaysShowScrollbarTrack(false);
	mSkillDescriptionScrollBox->SetAnimateWheelScrolling(true);
	mSkillDescriptionScrollBox->SetAllowOverscroll(true);
	mSkillDescriptionScrollBox->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
	// SelfHitTestInvisible이면 스크롤바는 보이지만 ScrollBox 본체가 손가락의
	// 드래그를 받지 못한다. 모바일 터치 스크롤을 위해 반드시 Visible이다.
	mSkillDescriptionScrollBox->SetVisibility(ESlateVisibility::Visible);

	mSkillDescriptionText = Tree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("RuntimeSkillDescriptionText"));
	FSlateFontInfo DescriptionFont = mDetailBodyText != nullptr
		? mDetailBodyText->GetFont() : FSlateFontInfo();
	DescriptionFont.Size = 29;
	mSkillDescriptionText->SetFont(DescriptionFont);
	mSkillDescriptionText->SetColorAndOpacity(FSlateColor(
		FLinearColor(0.94f, 0.88f, 0.76f, 1.f)));
	mSkillDescriptionText->SetAutoWrapText(true);
	mSkillDescriptionText->SetWrapTextAt(SkillVisualLayout::DescriptionSize.X - 42.f);
	mSkillDescriptionText->SetJustification(ETextJustify::Left);
	mSkillDescriptionText->SetLineHeightPercentage(1.32f);
	mSkillDescriptionText->SetShadowColorAndOpacity(
		FLinearColor(0.f, 0.f, 0.f, 0.84f));
	mSkillDescriptionText->SetShadowOffset(FVector2D(1.5f, 1.5f));
	mSkillDescriptionText->SetVisibility(ESlateVisibility::HitTestInvisible);
	mSkillDescriptionScrollBox->AddChild(mSkillDescriptionText);
	if (UScrollBoxSlot* DescriptionSlot = Cast<UScrollBoxSlot>(
		mSkillDescriptionText->Slot))
	{
		DescriptionSlot->SetPadding(FMargin(8.f, 4.f, 18.f, 18.f));
		DescriptionSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	// AP/쿨타임은 현재 전투 HUD WBP의 실제 Image 브러시를 우선 쓴다.
	// 상세 전용으로 비슷한 아이콘을 추측하지 않아 HUD 교체 시에도 같은 규격을
	// 유지한다. 피해와 치명타는 34px 모바일 표시를 위해 굵은 단색 실루엣으로
	// 새로 만든 상세 전용 에셋을 쓴다. 호스트 화면의 브러시 조회는 델리게이트로
	// 받는다 -- 프레젠터는 UIModel/게임모드를 보지 않는다.
	auto ResolveHudTexture = [this](const FName SourceWidgetName,
		UTexture2D* Fallback) -> UTexture2D*
	{
		if (mStatTextureResolver.IsBound() == true)
		{
			if (UTexture2D* Texture = mStatTextureResolver.Execute(
				SourceWidgetName, Fallback))
			{
				return Texture;
			}
		}
		return Fallback;
	};
	UTexture2D* StatTextures[SkillPreviewStatCount] = {
		ResolveHudTexture(TEXT("CommandCostBadge_0"), mSkillVisualAPIconTexture),
		mSkillVisualDamageIconTexture,
		ResolveHudTexture(TEXT("CommandCooldownBadge_0"),
			mSkillVisualCooldownIconTexture),
		mSkillVisualCriticalIconTexture };

	mSkillVisualStatIcons.Reset();
	mSkillVisualStatTexts.Reset();
	for (int32 Index = 0; Index < SkillPreviewStatCount; ++Index)
	{
		mSkillVisualStatIcons.Add(AddImage(
			FName(*FString::Printf(TEXT("RuntimeSkillStatIcon_%d"), Index)),
			StatTextures[Index], SkillVisualLayout::StatIconPositions[Index],
			SkillVisualLayout::StatIconSize, 8));
		UTextBlock* StatText = AddText(
			FName(*FString::Printf(TEXT("RuntimeSkillStatText_%d"), Index)),
			SkillVisualLayout::StatTextPositions[Index],
			SkillVisualLayout::StatTextSize, 24,
			FLinearColor(0.94f, 0.88f, 0.76f, 1.f), ETextJustify::Left);
		StatText->SetText(FText::FromString(TEXT("-")));
		mSkillVisualStatTexts.Add(StatText);
		AddRule(FName(*FString::Printf(TEXT("RuntimeSkillStatRule_%d"), Index)),
			FVector2D(382.f, SkillVisualLayout::StatTextPositions[Index].Y + 38.f),
			FVector2D(286.f, 1.f), FLinearColor(0.58f, 0.37f, 0.15f, 0.66f), 6);
	}

	auto AddRangeButton = [this, Tree](const FName ButtonName,
		const FName TextName, const FVector2D Position,
		TObjectPtr<UTextBlock>& OutText) -> UButton*
	{
		UButton* Button = Tree->ConstructWidget<UButton>(
			UButton::StaticClass(), ButtonName);
		if (UCanvasPanelSlot* Slot = mSkillVisualPreview->AddChildToCanvas(Button))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(SkillVisualLayout::RangeButtonSize);
			Slot->SetZOrder(12);
		}
		// 별도 생성한 얇은 투명 PNG를 실제 버튼 브러시로 쓴다. 원본의 가로형
		// 비율을 유지한 316x56으로 키워 초광폭 모바일에서도 읽을 높이를 확보한다.
		Button->SetStyle(SkillVisualLayout::MakeRangeButtonStyle(
			mSkillRangeButtonTexture, mSkillRangeButtonSelectedTexture, false));
		Button->SetTouchMethod(EButtonTouchMethod::PreciseTap);
		Button->SetClickMethod(EButtonClickMethod::PreciseClick);
		Button->SetVisibility(ESlateVisibility::Visible);

		UScaleBox* AutoFit = Tree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), FName(*FString::Printf(TEXT("%s_AutoFit"),
				*TextName.ToString())));
		AutoFit->SetStretch(EStretch::ScaleToFitX);
		AutoFit->SetStretchDirection(EStretchDirection::DownOnly);
		AutoFit->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		AutoFit->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Button->SetContent(AutoFit);
		if (UButtonSlot* AutoFitSlot = Cast<UButtonSlot>(AutoFit->Slot))
		{
			AutoFitSlot->SetHorizontalAlignment(HAlign_Fill);
			AutoFitSlot->SetVerticalAlignment(VAlign_Fill);
			AutoFitSlot->SetPadding(FMargin(0.f));
		}

		OutText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
		FSlateFontInfo ButtonFont = mDetailBodyText != nullptr
			? mDetailBodyText->GetFont() : FSlateFontInfo();
		ButtonFont.Size = 24;
		OutText->SetFont(ButtonFont);
		OutText->SetColorAndOpacity(FSlateColor(
			FLinearColor(0.96f, 0.91f, 0.80f, 1.f)));
		OutText->SetJustification(ETextJustify::Center);
		OutText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .9f));
		OutText->SetShadowOffset(FVector2D(1.5f, 1.5f));
		OutText->SetVisibility(ESlateVisibility::HitTestInvisible);
		AutoFit->SetContent(OutText);
		if (UScaleBoxSlot* TextSlot = Cast<UScaleBoxSlot>(OutText->Slot))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Center);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
		return Button;
	};
	mSkillSelectRangeButton = AddRangeButton(TEXT("RuntimeSkillSelectRangeButton"),
		TEXT("RuntimeSkillSelectRangeText"), SkillVisualLayout::SelectButtonPosition,
		mSkillSelectRangeText);
	mSkillEffectRangeButton = AddRangeButton(TEXT("RuntimeSkillEffectRangeButton"),
		TEXT("RuntimeSkillEffectRangeText"), SkillVisualLayout::EffectButtonPosition,
		mSkillEffectRangeText);
	mSkillSelectRangeButton->OnClicked.AddUniqueDynamic(this,
		&USkillDetailOverlayPresenter::HandleSkillSelectRangeButtonClicked);
	mSkillEffectRangeButton->OnClicked.AddUniqueDynamic(this,
		&USkillDetailOverlayPresenter::HandleSkillEffectRangeButtonClicked);
	RefreshSkillRangeButtonStyles();

	// 선택/효과 중심을 감싸는 큰 링. 셀보다 먼저 그려 은은한 하광으로 쓴다.
	mSkillVisualCasterHalo = AddImage(TEXT("RuntimeSkillCasterHalo"),
		mSkillVisualRingTexture, FVector2D::ZeroVector, FVector2D(82.f, 82.f), 0,
		FLinearColor(0.12f, 0.62f, 1.f, 0.82f));
	mSkillVisualEffectHalo = AddImage(TEXT("RuntimeSkillEffectHalo"),
		mSkillVisualRingTexture, FVector2D::ZeroVector, FVector2D(238.f, 238.f), 0,
		FLinearColor(1.f, 0.18f, 0.06f, 0.46f));

	mSkillVisualCells.Reset();
	for (int32 Row = 0; Row < SkillPreviewRows; ++Row)
	{
		for (int32 Column = 0; Column < SkillPreviewColumns; ++Column)
		{
			const FVector2D Position = SkillVisualLayout::GridOrigin + FVector2D(
				SkillVisualLayout::CellStep.X * Column,
				SkillVisualLayout::CellStep.Y * Row);
			UImage* Cell = AddImage(FName(*FString::Printf(
				TEXT("RuntimeSkillRangeCell_R%dC%d"), Row, Column)),
				mSkillVisualCellNormalTexture, Position,
				SkillVisualLayout::CellSize, 2,
				FLinearColor(0.30f, 0.28f, 0.24f, 0.52f));
			mSkillVisualCells.Add(Cell);
		}
	}

	mSkillVisualConnectorPips.Reset();
	for (int32 Index = 0; Index < 6; ++Index)
	{
		mSkillVisualConnectorPips.Add(AddImage(FName(*FString::Printf(
			TEXT("RuntimeSkillConnectorPip_%d"), Index)),
			mSkillVisualAPIconTexture, FVector2D::ZeroVector, FVector2D(14.f, 14.f), 4,
			FLinearColor(1.f, 0.82f, 0.28f, 1.f)));
	}
	mSkillVisualCasterMarker = AddImage(TEXT("RuntimeSkillCasterMarker"),
		mSkillVisualCasterIconTexture, FVector2D::ZeroVector, FVector2D(42.f, 42.f), 6,
		FLinearColor(0.55f, 0.86f, 1.f, 1.f));
	mSkillVisualTargetMarker = AddImage(TEXT("RuntimeSkillTargetMarker"),
		mSkillVisualTargetIconTexture, FVector2D::ZeroVector, FVector2D(42.f, 42.f), 6,
		FLinearColor(1.f, 0.72f, 0.25f, 1.f));
	mSkillVisualSelectLegend = AddText(TEXT("RuntimeSkillSelectLegend"),
		FVector2D(760.f, 770.f), FVector2D(330.f, 44.f), 25,
		FLinearColor(0.34f, 0.72f, 1.f, 1.f), ETextJustify::Center);
	mSkillVisualEffectLegend = AddText(TEXT("RuntimeSkillEffectLegend"),
		FVector2D(1110.f, 770.f), FVector2D(330.f, 44.f), 25,
		FLinearColor(1.f, 0.39f, 0.22f, 1.f), ETextJustify::Center);

	// 첨부 시안형 2.5D 범위판은 별도 WBP가 소유한다. 전투 월드를 다시
	// 렌더하지 않고, 실제 스킬 DTO로 이 WBP의 셀/마커만 갱신한다.
	if (mSkillTacticalDiagramWidgetClass != nullptr)
	{
		if (APlayerController* DiagramOwner = mDetailOverlayWidget->GetOwningPlayer())
		{
			mSkillTacticalDiagramWidget =
				CreateWidget<USkillTacticalDiagramWidget>(DiagramOwner,
					mSkillTacticalDiagramWidgetClass);
		}
		else if (UWorld* World = mWorld.Get())
		{
			mSkillTacticalDiagramWidget =
				CreateWidget<USkillTacticalDiagramWidget>(World,
					mSkillTacticalDiagramWidgetClass);
		}
		if (mSkillTacticalDiagramWidget != nullptr)
		{
			mSkillTacticalDiagramWidget->OnPreviewVisibilityChanged().AddUObject(
				this, &USkillDetailOverlayPresenter::
				HandleSkillTacticalPreviewVisibilityChanged);
			if (UCanvasPanelSlot* DiagramSlot = mSkillVisualPreview->AddChildToCanvas(
				mSkillTacticalDiagramWidget))
			{
				// 버튼은 왼쪽 요약열로 옮겼다. 이 WBP는 토글 뒤 오른쪽 설명
				// 자리를 대신하는 범위판만 그린다.
				// 공용 상세 프레임의 검은 정보면 우측 절반을 그대로 사용한다.
				// 임의 여백을 두지 않아 9x9 판의 네 꼭짓점이 칠판 경계에 닿는다.
				DiagramSlot->SetPosition(FVector2D(698.f, 254.f));
				DiagramSlot->SetSize(FVector2D(872.f, 563.f));
				DiagramSlot->SetZOrder(30);
			}
			mSkillTacticalDiagramWidget->SetVisibility(
				ESlateVisibility::SelfHitTestInvisible);
			for (const TCHAR* WidgetName : {
				TEXT("TacticalDiagramBackdrop"), TEXT("TacticalLegendRule"),
				TEXT("TacticalSelectLegendButton"),
				TEXT("TacticalEffectLegendButton") })
			{
				DetailSetShown(mSkillTacticalDiagramWidget->GetWidgetFromName(WidgetName), false);
			}
		}
	}

	// 별도 양피지/격자 판을 더하지 않는다. 상세 WBP의 검은 정보면에 실제
	// 전투 장면을 바로 넣어, 사용자가 보고 있던 유닛·장애물·타일 관계가
	// 그대로 범위 설명이 된다.
	mSkillWorldPreviewImage = Tree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("RuntimeSkillWorldPreview"));
	if (UCanvasPanelSlot* PreviewSlot = mSkillVisualPreview->AddChildToCanvas(
		mSkillWorldPreviewImage))
	{
		PreviewSlot->SetPosition(FVector2D(615.f, 535.f));
		PreviewSlot->SetSize(FVector2D(820.f, 255.f));
		PreviewSlot->SetZOrder(20);
	}
	mSkillWorldPreviewImage->SetColorAndOpacity(FLinearColor::White);
	mSkillWorldPreviewImage->SetVisibility(ESlateVisibility::Collapsed);
}

void USkillDetailOverlayPresenter::SetSkillVisualPreviewShown(const bool bShown)
{
	DetailSetShown(mSkillVisualPreview, bShown);
	if (bShown == false)
	{
		HideWorldPreviewImage();
	}
}

void USkillDetailOverlayPresenter::HideWorldPreviewImage()
{
	DetailSetShown(mSkillWorldPreviewImage, false);
	// SceneCapture 장치는 호출자 소유다. 캡처를 실제로 멈추는 것은 호출자
	// 몫이라 신호만 보낸다 -- 프레젠터는 월드 액터를 만지지 않는다.
	mOnWorldPreviewStopRequested.Broadcast();
}

void USkillDetailOverlayPresenter::HandleSkillSelectRangeButtonClicked()
{
	if (mSkillTacticalDiagramWidget != nullptr)
	{
		mSkillTacticalDiagramWidget->ShowSelectRangePreview();
		RefreshSkillRangeButtonStyles();
	}
}

void USkillDetailOverlayPresenter::HandleSkillEffectRangeButtonClicked()
{
	if (mSkillTacticalDiagramWidget != nullptr)
	{
		mSkillTacticalDiagramWidget->ShowEffectRangePreview();
		RefreshSkillRangeButtonStyles();
	}
}

void USkillDetailOverlayPresenter::RefreshSkillRangeButtonStyles()
{
	const bool bSelectActive = mSkillTacticalDiagramWidget != nullptr
		&& mSkillTacticalDiagramWidget->IsSelectRangePreviewShown();
	const bool bEffectActive = mSkillTacticalDiagramWidget != nullptr
		&& mSkillTacticalDiagramWidget->IsEffectRangePreviewShown();
	if (mSkillSelectRangePlate != nullptr)
	{
		mSkillSelectRangePlate->SetBrushFromTexture(
			bSelectActive && mSkillRangeButtonSelectedTexture != nullptr
				? mSkillRangeButtonSelectedTexture : mSkillRangeButtonTexture, false);
	}
	else if (mSkillSelectRangeButton != nullptr)
	{
		mSkillSelectRangeButton->SetStyle(SkillVisualLayout::MakeRangeButtonStyle(
			mSkillRangeButtonTexture, mSkillRangeButtonSelectedTexture,
			bSelectActive));
	}
	if (mSkillEffectRangePlate != nullptr)
	{
		mSkillEffectRangePlate->SetBrushFromTexture(
			bEffectActive && mSkillRangeButtonSelectedTexture != nullptr
				? mSkillRangeButtonSelectedTexture : mSkillRangeButtonTexture, false);
	}
	else if (mSkillEffectRangeButton != nullptr)
	{
		mSkillEffectRangeButton->SetStyle(SkillVisualLayout::MakeRangeButtonStyle(
			mSkillRangeButtonTexture, mSkillRangeButtonSelectedTexture,
			bEffectActive));
	}
}

void USkillDetailOverlayPresenter::HandleSkillTacticalPreviewVisibilityChanged(
	const bool bPreviewShown)
{
	// 설명 ScrollBox와 범위판은 오른쪽 가변 영역을 교대로 쓴다. 왼쪽의
	// 아이콘·수치·두 버튼은 어느 상태에서도 고정되어 문맥을 잃지 않는다.
	if (mSkillDescriptionScrollBox != nullptr)
	{
		mSkillDescriptionScrollBox->SetVisibility(bPreviewShown
			? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (mSkillContentSwitcher != nullptr)
	{
		mSkillContentSwitcher->SetActiveWidgetIndex(bPreviewShown ? 1 : 0);
	}
	DetailSetShown(mDetailBodyText, false);
	RefreshSkillRangeButtonStyles();
}

void USkillDetailOverlayPresenter::SetSyntheticSkillDiagramShown(const bool bShown)
{
	for (UImage* Cell : mSkillVisualCells)
	{
		DetailSetShown(Cell, bShown);
	}
	for (UImage* Pip : mSkillVisualConnectorPips)
	{
		DetailSetShown(Pip, bShown);
	}
	DetailSetShown(mSkillVisualCasterHalo, bShown);
	DetailSetShown(mSkillVisualEffectHalo, bShown);
	DetailSetShown(mSkillVisualCasterMarker, bShown);
	DetailSetShown(mSkillVisualTargetMarker, bShown);
	DetailSetShown(mSkillVisualSelectLegend, bShown);
	DetailSetShown(mSkillVisualEffectLegend, bShown);
}

/** @brief 실제 스킬 DTO로 수치 메달과 하나로 합친 사각 전술 보드를 갱신한다. */
void USkillDetailOverlayPresenter::UpdateSkillVisualPreview(const FSkillDetailUI& Detail)
{
	BuildSkillVisualPreview();
	if (mSkillVisualPreview == nullptr)
	{
		return;
	}
	DetailSetTextIfPresent(mSkillDescriptionText, Detail.mDescription);
	if (mSkillDescriptionScrollBox != nullptr)
	{
		mSkillDescriptionScrollBox->ScrollToStart();
		mSkillDescriptionScrollBox->SetVisibility(ESlateVisibility::Visible);
	}
	const FString DamageText = Detail.mDamageMax <= 0
		? FString(TEXT("피해 -"))
		: (Detail.mDamageMin == Detail.mDamageMax
			? FString::Printf(TEXT("피해 %d"), Detail.mDamageMax)
			: FString::Printf(TEXT("피해 %d~%d"), Detail.mDamageMin, Detail.mDamageMax));
	const FString StatValues[SkillPreviewStatCount] = {
		FString::Printf(TEXT("AP %d"), Detail.mActionPointCost),
		DamageText,
		Detail.mCooldownTurns > 0
			? FString::Printf(TEXT("쿨타임 %d턴"), Detail.mCooldownTurns)
			: FString(TEXT("쿨타임 -")),
		Detail.mCriticalDamage > 0
			? FString::Printf(TEXT("치명타 %d"), Detail.mCriticalDamage)
			: FString(TEXT("치명타 -")) };
	for (int32 Index = 0; Index < mSkillVisualStatTexts.Num(); ++Index)
	{
		DetailSetTextIfPresent(mSkillVisualStatTexts[Index], FText::FromString(StatValues[Index]));
	}
	const int32 ActualSelectRange = FMath::Max(
		FMath::RoundToInt(Detail.mTargeting.mSelectRange), 0);
	const int32 ActualHitRange = FMath::Max(
		FMath::RoundToInt(Detail.mTargeting.mHitRange), 0);
	DetailSetTextIfPresent(mSkillSelectRangeText, FText::FromString(FString::Printf(
		TEXT("사정 범위  %d칸"), ActualSelectRange)));
	DetailSetTextIfPresent(mSkillEffectRangeText, FText::FromString(FString::Printf(
		TEXT("영향 범위  %d칸"), ActualHitRange)));

	const int32 PreviewSelectRange = FMath::Clamp(ActualSelectRange, 0, 2);
	const int32 PreviewHitRange = FMath::Clamp(ActualHitRange, 0, 2);
	const int32 CasterRow = SkillVisualLayout::CasterRow;
	const int32 CasterColumn = SkillVisualLayout::CasterColumn;
	int32 TargetRow = CasterRow;
	int32 TargetColumn = CasterColumn;
	if (Detail.mTargeting.mSelectShape != ECombatSkillSelectShapeUI::Single
		&& Detail.mTargeting.mSelectShape != ECombatSkillSelectShapeUI::None)
	{
		const int32 Step = FMath::Max(PreviewSelectRange, 1);
		TargetColumn = FMath::Min(CasterColumn + Step, SkillPreviewColumns - 1);
		if (Detail.mTargeting.mSelectShape == ECombatSkillSelectShapeUI::Diagonal)
		{
			TargetRow = FMath::Max(CasterRow - Step, 0);
		}
	}

	for (int32 Row = 0; Row < SkillPreviewRows; ++Row)
	{
		for (int32 Column = 0; Column < SkillPreviewColumns; ++Column)
		{
			const int32 Index = Row * SkillPreviewColumns + Column;
			UImage* Cell = mSkillVisualCells.IsValidIndex(Index)
				? mSkillVisualCells[Index].Get() : nullptr;
			if (Cell == nullptr)
			{
				continue;
			}
			const bool bSelect = CombatDetailGrid::SelectCovers(
				Detail.mTargeting.mSelectShape, PreviewSelectRange,
				Row - CasterRow, Column - CasterColumn);
			const bool bEffect = CombatDetailGrid::HitCovers(
				Detail.mTargeting.mHitShape, PreviewHitRange,
				Row - TargetRow, Column - TargetColumn);
			const bool bTarget = Row == TargetRow && Column == TargetColumn;
			const bool bCaster = Row == CasterRow && Column == CasterColumn;
			Cell->SetBrushFromTexture((bSelect || bEffect || bTarget || bCaster)
				? mSkillVisualCellSelectedTexture : mSkillVisualCellNormalTexture, false);
			FLinearColor CellColor(0.30f, 0.28f, 0.24f, 0.52f);
			if (bSelect)
			{
				CellColor = FLinearColor(0.12f, 0.56f, 1.f, 0.78f);
			}
			if (bEffect)
			{
				CellColor = FLinearColor(0.80f, 0.12f, 0.055f, 0.60f);
			}
			if (bCaster)
			{
				CellColor = FLinearColor(0.15f, 0.72f, 1.f, 1.f);
			}
			if (bTarget)
			{
				CellColor = FLinearColor(1.f, 0.76f, 0.18f, 1.f);
			}
			Cell->SetColorAndOpacity(CellColor);
		}
	}

	auto CellCenter = [](const int32 Row, const int32 Column)
	{
		return SkillVisualLayout::GridOrigin + FVector2D(
			SkillVisualLayout::CellStep.X * Column + SkillVisualLayout::CellSize.X * .5f,
			SkillVisualLayout::CellStep.Y * Row + SkillVisualLayout::CellSize.Y * .5f);
	};
	const FVector2D CasterCenter = CellCenter(CasterRow, CasterColumn);
	const FVector2D TargetCenter = CellCenter(TargetRow, TargetColumn);
	auto PlaceCentered = [](UWidget* Widget, const FVector2D Center, const FVector2D Size)
	{
		if (Widget != nullptr)
		{
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot))
			{
				Slot->SetPosition(Center - Size * .5f);
				Slot->SetSize(Size);
			}
		}
	};
	PlaceCentered(mSkillVisualCasterHalo, CasterCenter, FVector2D(82.f, 82.f));
	const float EffectDiameter = 74.f + PreviewHitRange * 82.f;
	PlaceCentered(mSkillVisualEffectHalo, TargetCenter,
		FVector2D(EffectDiameter, EffectDiameter));
	PlaceCentered(mSkillVisualCasterMarker, CasterCenter, FVector2D(42.f, 42.f));
	PlaceCentered(mSkillVisualTargetMarker, TargetCenter, FVector2D(42.f, 42.f));
	DetailSetShown(mSkillVisualTargetMarker, TargetRow != CasterRow || TargetColumn != CasterColumn);

	const bool bSeparateTarget = TargetRow != CasterRow || TargetColumn != CasterColumn;
	for (int32 Index = 0; Index < mSkillVisualConnectorPips.Num(); ++Index)
	{
		UImage* Pip = mSkillVisualConnectorPips[Index];
		DetailSetShown(Pip, bSeparateTarget);
		if (bSeparateTarget)
		{
			const float Alpha = static_cast<float>(Index + 1)
				/ static_cast<float>(mSkillVisualConnectorPips.Num() + 1);
			PlaceCentered(Pip, FMath::Lerp(CasterCenter, TargetCenter, Alpha),
				FVector2D(14.f, 14.f));
		}
	}
	DetailSetTextIfPresent(mSkillVisualSelectLegend, Detail.mTargeting.mSelectShape
		== ECombatSkillSelectShapeUI::Single
		? LOCTEXT("SkillPreviewSelfTarget", "선택 위치  현재 타일")
		: FText::FromString(FString::Printf(TEXT("선택 거리  %d칸"), ActualSelectRange)));
	DetailSetTextIfPresent(mSkillVisualEffectLegend, ActualHitRange <= 0
		? LOCTEXT("SkillPreviewSingleEffect", "효과 범위  대상 1칸")
		: FText::FromString(FString::Printf(TEXT("효과 범위  %d칸"), ActualHitRange)));

	// 전용 WBP가 표현을 맡는다. 구형 셀과 SceneCapture 이미지는 항상 접는다.
	SetSyntheticSkillDiagramShown(false);
	HideWorldPreviewImage();
	if (mSkillTacticalDiagramWidget != nullptr)
	{
		mSkillTacticalDiagramWidget->ApplySkillDetail(Detail);
		mSkillTacticalDiagramWidget->SetVisibility(
			ESlateVisibility::SelfHitTestInvisible);
		RefreshSkillRangeButtonStyles();
	}
}

/** @brief 스킬 상세 DTO 하나로 패널 전체를 채워 띄운다. */
void USkillDetailOverlayPresenter::Present(const FSkillDetailUI& Detail)
{
	if (EnsureOverlayWidget() == false)
	{
		return;
	}
	ApplyReadableDetailTypography(true);
	ApplyDetailColumnLayout(false);
	ApplyResponsiveSkillPanelScale(true);
	// 스킬 내부 정보면은 WBP_SkillDetailContent가 전부 소유한다. 공용 상세판의
	// 레거시 아이콘/양피지는 스킬 상태에서만 접고 다른 상세에서는 복구한다.
	DetailSetShown(mDetailOverlayWidget->GetWidgetFromName(
		TEXT("DetailIconFrame")), false);
	DetailSetShown(mDetailIconImage, false);
	DetailSetShown(mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailIdentityPlate")), false);
	DetailSetShown(mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailStatsPlate")), false);
	DetailSetShown(mDetailSubtitleText, false);

	DetailSetTextIfPresent(mDetailTitleText, Detail.mName);

	// 스킬 index는 유닛마다 0부터 다시 시작한다. 몬스터 상세에서 받은 index로
	// 플레이어 카드 레일을 조회하면 같은 슬롯의 전혀 다른 수치가 섞인다. 이름,
	// 설명과 함께 왕복한 상세 DTO를 이 화면의 단일 출처로 쓴다.
	ClearDetailChips();
	SetDetailChip(0, LOCTEXT("DetailChipCost", "AP"),
		FText::AsNumber(Detail.mActionPointCost));
	SetDetailChip(1, LOCTEXT("DetailChipDamage", "피해"),
		Detail.mDamageMax <= 0 ? FText::FromString(TEXT("-"))
			: (Detail.mDamageMin == Detail.mDamageMax
				? FText::AsNumber(Detail.mDamageMax)
				: FText::FromString(FString::Printf(TEXT("%d~%d"),
					Detail.mDamageMin, Detail.mDamageMax))));
	SetDetailChip(2, LOCTEXT("DetailChipCooldown", "쿨타임"),
		Detail.mCooldownTurns <= 0 ? FText::FromString(TEXT("-"))
			: FText::FromString(FString::Printf(TEXT("%d턴"), Detail.mCooldownTurns)));
	// 타수는 칩으로 두지 않는다. 2타 이상일 때 설명 문장에 적기로 합의돼
	// 있어(0802), 칩까지 두면 같은 정보가 두 군데 나온다.
	SetDetailChip(3, LOCTEXT("DetailChipRange", "사거리"),
		FText::AsNumber(FMath::RoundToInt(Detail.mTargeting.mSelectRange)));
	SetDetailChip(4, LOCTEXT("DetailChipCritical", "치명"),
		Detail.mCriticalDamage <= 0 ? FText::FromString(TEXT("-"))
			: FText::AsNumber(Detail.mCriticalDamage));
	// 이 화면에만 새 범위판/격자 에셋을 추가하지 않는다. 핵심 수치는 상단 요약,
	// 대상 규칙은 본문 안의 짧은 텍스트로 읽히므로 별도 열과 받침판을 모두 접는다.
	ClearDetailGrids();
	DetailSetShown(mDetailStatBlock, false);
	ShowDetailRightBlock(nullptr);
	DetailSetShown(mDetailStatColumn, false);
	DetailSetShown(mDetailRightColumn, false);
	DetailSetShown(mDetailDivider0, false);
	DetailSetShown(mDetailDivider1, false);
	if (UWidget* FreePlate = mDetailOverlayWidget->GetWidgetFromName(
		TEXT("DetailFreePlate")))
	{
		DetailSetShown(FreePlate, false);
	}

	// 본문은 설명에만 집중한다. 선택 거리와 효과 범위는 아래 통합 보드가
	// 색·위치·라벨로 함께 설명하므로 같은 내용을 텍스트로 반복하지 않는다.
	// 0822 확정: 조준/효과 차단 텍스트 설명은 걷었다. 차단 정보는
	// 통합 전술 보드의 색·범위 표시가 대신한다.
	FString Body = Detail.mDescription.ToString();
	DetailSetTextIfPresent(mDetailBodyText, FText::FromString(Body));
	UpdateSkillVisualPreview(Detail);
	// 공용 본문은 다른 상세 종류와의 호환을 위해 데이터만 유지한다. 실제
	// 스킬 화면은 오른쪽 ScrollBox를 사용해 긴 설명과 차단 규칙을 모두 담는다.
	DetailSetTextIfPresent(mSkillDescriptionText, FText::FromString(Body));
	DetailSetShown(mDetailBodyText, false);
	SetSkillVisualPreviewShown(true);

	DetailSetPortraitCropped(mSkillIconImage, Detail.mIcon);
	// 자기는 눌림을 안 받고 스킬 칸만 받는다. 그래서 칸 밖을 톡 치면 눌림이
	// 호스트까지 내려가 패널이 닫힌다.
	mDetailOverlayWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

/** @brief 아티팩트 효과 본문 전용 ScrollBox 묶음을 처음 한 번 짓는다. */
void USkillDetailOverlayPresenter::BuildArtifactDescriptionScroll()
{
	if (mArtifactDescriptionRoot != nullptr || mDetailOverlayWidget == nullptr
		|| mDetailOverlayWidget->WidgetTree == nullptr)
	{
		return;
	}
	UWidgetTree* Tree = mDetailOverlayWidget->WidgetTree;
	// 스킬 미리보기와 같은 규칙 -- 프레임이 실제로 얹힌 캔버스에 앉혀 외곽
	// WBP 의 ScaleBox 변환을 함께 받는다(BuildSkillVisualPreview 의 호스트 탐색).
	UCanvasPanel* Host = nullptr;
	UWidget* Frame = mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailFrameImage"));
	if (Frame != nullptr)
	{
		for (UPanelWidget* Parent = Frame->GetParent(); Parent != nullptr;
			Parent = Parent->GetParent())
		{
			if (UCanvasPanel* FrameCanvas = Cast<UCanvasPanel>(Parent))
			{
				Host = FrameCanvas;
				break;
			}
		}
	}
	if (Host == nullptr)
	{
		Host = Cast<UCanvasPanel>(
			mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailPanelRoot")));
	}
	if (Host == nullptr)
	{
		Host = Cast<UCanvasPanel>(Tree->RootWidget);
	}
	if (Host == nullptr)
	{
		return;
	}

	// 폴드처럼 화면이 16:9가 아니어도 본문 좌표계는 언제나 1920x1080이다.
	// 스킬 미리보기와 같은 ScaleToFit 계약을 세운다.
	UScaleBox* DesignScale = Tree->ConstructWidget<UScaleBox>(
		UScaleBox::StaticClass(), TEXT("RuntimeArtifactDesignScale"));
	if (UCanvasPanelSlot* ScaleSlot = Host->AddChildToCanvas(DesignScale))
	{
		ScaleSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		ScaleSlot->SetOffsets(FMargin(0.f));
		ScaleSlot->SetAlignment(FVector2D::ZeroVector);
		ScaleSlot->SetZOrder(40);
	}
	DesignScale->SetStretch(EStretch::ScaleToFit);
	DesignScale->SetStretchDirection(EStretchDirection::Both);
	DesignScale->SetClipping(EWidgetClipping::ClipToBounds);

	USizeBox* DesignSize = Tree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("RuntimeArtifactDesignSize"));
	DesignSize->SetWidthOverride(1920.f);
	DesignSize->SetHeightOverride(1080.f);
	DesignScale->AddChild(DesignSize);
	if (UScaleBoxSlot* DesignSlot = Cast<UScaleBoxSlot>(DesignSize->Slot))
	{
		DesignSlot->SetHorizontalAlignment(HAlign_Center);
		DesignSlot->SetVerticalAlignment(VAlign_Center);
	}

	UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("RuntimeArtifactDescriptionCanvas"));
	DesignSize->SetContent(Canvas);

	// 효과 줄은 아티팩트마다 길이가 다르다. 공용 DetailBodyText 의 고정 높이
	// Canvas 슬롯 대신 실제 ScrollBox 를 써서 긴 본문도 스크롤로 다 읽힌다.
	mArtifactDescriptionScrollBox = Tree->ConstructWidget<UScrollBox>(
		UScrollBox::StaticClass(), TEXT("RuntimeArtifactDescriptionScroll"));
	if (UCanvasPanelSlot* ScrollSlot = Canvas->AddChildToCanvas(
		mArtifactDescriptionScrollBox))
	{
		// HUD 아티팩트 상세가 쓰던 본문 자리(아이콘 오른쪽) 그대로다.
		ScrollSlot->SetPosition(FVector2D(780.f, 360.f));
		ScrollSlot->SetSize(FVector2D(730.f, 370.f));
	}
	mArtifactDescriptionScrollBox->SetScrollBarVisibility(ESlateVisibility::Visible);
	mArtifactDescriptionScrollBox->SetScrollbarThickness(FVector2D(5.f, 5.f));
	mArtifactDescriptionScrollBox->SetAlwaysShowScrollbar(false);
	mArtifactDescriptionScrollBox->SetAlwaysShowScrollbarTrack(false);
	mArtifactDescriptionScrollBox->SetAnimateWheelScrolling(true);
	mArtifactDescriptionScrollBox->SetAllowOverscroll(true);
	mArtifactDescriptionScrollBox->SetConsumeMouseWheel(
		EConsumeMouseWheel::WhenScrollingPossible);
	// 스크롤 드래그를 받아야 하므로 Visible -- 스킬 설명 ScrollBox 와 같은 이유.
	mArtifactDescriptionScrollBox->SetVisibility(ESlateVisibility::Visible);

	mArtifactDescriptionText = Tree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("RuntimeArtifactDescriptionText"));
	FSlateFontInfo DescriptionFont = mDetailBodyText != nullptr
		? mDetailBodyText->GetFont() : FSlateFontInfo();
	DescriptionFont.Size = 30;   // HUD 아티팩트 본문과 같은 크기.
	mArtifactDescriptionText->SetFont(DescriptionFont);
	mArtifactDescriptionText->SetColorAndOpacity(FSlateColor(
		FLinearColor(0.94f, 0.88f, 0.76f, 1.f)));
	mArtifactDescriptionText->SetAutoWrapText(true);
	mArtifactDescriptionText->SetWrapTextAt(690.f);
	mArtifactDescriptionText->SetJustification(ETextJustify::Left);
	mArtifactDescriptionText->SetLineHeightPercentage(1.12f);
	mArtifactDescriptionText->SetShadowColorAndOpacity(
		FLinearColor(0.f, 0.f, 0.f, 0.84f));
	mArtifactDescriptionText->SetShadowOffset(FVector2D(1.5f, 1.5f));
	mArtifactDescriptionText->SetVisibility(ESlateVisibility::HitTestInvisible);
	mArtifactDescriptionScrollBox->AddChild(mArtifactDescriptionText);
	if (UScrollBoxSlot* TextSlot = Cast<UScrollBoxSlot>(
		mArtifactDescriptionText->Slot))
	{
		TextSlot->SetPadding(FMargin(0.f, 0.f, 18.f, 18.f));
		TextSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	mArtifactDescriptionRoot = DesignScale;
	DetailSetShown(mArtifactDescriptionRoot, false);
}

/**
 * @brief 아티팩트 상세 DTO 하나로 아티팩트 전용 2열 화면을 그린다.
 *
 * @details 전투 HUD 의 ShowArtifactDetailOverlay 렌더를 그대로 옮겼다. HUD 는
 * 이제 얇은 래퍼로 이 함수를 부르고, 상점/지도 레일도 같은 화면을 쓴다.
 * HUD 상태(유닛 상세 스킬 줄, 월드 제스처 잠금)는 호출자 몫이다.
 */
void USkillDetailOverlayPresenter::PresentArtifact(const FCombatArtifactUI& Detail)
{
	if (EnsureOverlayWidget() == false)
	{
		return;
	}
	// 본문 위젯이 서체 적용 전에 만들어져 있어야 첫 표시부터 GowunBatang 을 받는다.
	BuildArtifactDescriptionScroll();
	ApplyReadableDetailTypography(true);
	ApplyDetailColumnLayout(true);
	// T_MB_GenericDetailPanel에서 실제 검은 정보면은 1920 기준
	// X 344~1574 / Y 254~817이다. 40px 안전 여백 안에서 큰 아이콘과
	// 오른쪽 설명이 각각 독립된 열을 갖도록 authored WBP 슬롯을 재배치한다.
	SetCanvasRect(mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailIconFrame")),
		FVector2D(380.f, 310.f), FVector2D(300.f, 300.f));
	SetCanvasRect(mDetailIconImage, FVector2D(428.f, 358.f),
		FVector2D(204.f, 204.f));
	SetCanvasRect(mDetailOverlayWidget->GetWidgetFromName(
		TEXT("DetailSubtitleText_Center")), FVector2D(660.f, 275.f),
		FVector2D(800.f, 90.f));
	// 등급/적용 범위는 짧은 메타 정보라 별도 양피지 판이 필요 없다. 공용
	// IdentityPlate를 접고 외곽의 검은 정보면 위에 직접 적는다.
	DetailSetShown(mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailIdentityPlate")), false);
	DetailSetShown(mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailStatsPlate")), false);
	if (mDetailSubtitleText != nullptr)
	{
		mDetailSubtitleText->SetColorAndOpacity(FSlateColor(
			FLinearColor(0.92f, 0.84f, 0.68f, 1.f)));
	}

	DetailSetTextIfPresent(mDetailTitleText, Detail.mName);

	// 상세는 정보 화면이다. 가격은 상점의 구매/판매 문맥에서만 의미가
	// 있으므로 여기에는 희귀도만 남긴다.
	DetailSetTextIfPresent(mDetailSubtitleText, Detail.mRarityName.IsEmpty()
		? LOCTEXT("ArtifactPartyWide", "아티팩트 · 파티 전체 적용")
		: FText::Format(LOCTEXT("ArtifactRarityPartyWide", "{0} · 파티 전체 적용"),
			Detail.mRarityName));

	FString Body;
	for (const FText& Line : Detail.mEffectDescriptions)
	{
		if (Body.IsEmpty() == false)
		{
			Body += LINE_TERMINATOR;
			Body += LINE_TERMINATOR;
		}
		Body += TEXT("· ");
		Body += Line.ToString();
	}
	if (Body.IsEmpty() == true)
	{
		Body = TEXT("효과 설명이 아직 없다.");
	}

	DetailSetPortraitCropped(mDetailIconImage, Detail.mIcon);

	// 아티팩트에는 사거리도 수치 칩도 없다. 스킬 상세가 남긴 것을 걷는다.
	//
	// 값만 비우면 "-" 다섯 개가 그대로 떠서 무언가 못 채운 화면처럼 보인다.
	// 실제로 그렇게 나왔다(0804 검수). 칩 묶음을 통째로 끈다.
	ClearDetailGrids();
	ClearDetailChips();
	DetailSetShown(mDetailStatBlock, false);
	// 유닛 상세의 스킬 칸 줄도 걷는다. HUD 밖 호스트도 이름으로 찾아 닫는다.
	DetailSetShown(mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailSkillRowHost")), false);
	// 아이콘·등급·효과를 하나의 연속된 정보면에서 읽는다. 효과 본문은 전용
	// ScrollBox 에 담아, 패시브 줄이 길어도 잘리지 않고 스크롤로 다 읽힌다.
	const FString EffectBody = FString(TEXT("효과")) + LINE_TERMINATOR + Body;
	// 공용 본문은 다른 상세 종류와의 호환을 위해 데이터만 유지한다 -- 스킬
	// 상세가 mDetailBodyText 를 데이터 전용으로 두는 것과 같은 규칙.
	DetailSetTextIfPresent(mDetailBodyText, FText::FromString(EffectBody));
	DetailSetShown(mDetailBodyText, false);
	DetailSetTextIfPresent(mArtifactDescriptionText, FText::FromString(EffectBody));
	if (mArtifactDescriptionScrollBox != nullptr)
	{
		mArtifactDescriptionScrollBox->ScrollToStart();
	}
	DetailSetShown(mArtifactDescriptionRoot, true);
	DetailSetTextIfPresent(mDetailExtraHeading, FText::GetEmpty());
	DetailSetTextIfPresent(mDetailExtraText, FText::GetEmpty());
	ShowDetailRightBlock(nullptr);

	// 확정 시안: 아티팩트에는 스킬용 받침판과 왼쪽 설명 글이 없다. 대신
	// 희귀도 보석 줄을 켠다 (일반 1 · 희귀 3 · 영웅 5).
	if (UWidget* FreePlate = mDetailOverlayWidget->GetWidgetFromName(
		TEXT("DetailFreePlate")))
	{
		FreePlate->SetVisibility(ESlateVisibility::Collapsed);
	}
	// 아티팩트에는 열을 가르는 장식선 자체가 필요 없다. 두 열용 공용 판은
	// 유지하되 선만 접어 아이콘과 효과가 한 화면으로 이어져 보이게 한다.
	DetailSetShown(mDetailDivider0, false);
	DetailSetShown(mDetailDivider1, false);
	static const int32 LitByRarity[] = { 1, 3, 5 };
	const int32 LitCount = LitByRarity[FMath::Clamp(Detail.mRarityLevel, 0, 2)];
	for (int32 Index = 0; Index < 5; ++Index)
	{
		UImage* Gem = Cast<UImage>(mDetailOverlayWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("DetailRarityGem_%d"), Index))));
		if (Gem == nullptr)
		{
			continue;
		}
		Gem->SetVisibility(ESlateVisibility::HitTestInvisible);
		// 안 켠 보석은 같은 그림을 어둡게 깐다 -- 자리를 비우면 등급이 몇 칸
		// 짜리인지 읽히지 않는다.
		Gem->SetColorAndOpacity(Index < LitCount
			? FLinearColor::White : FLinearColor(0.18f, 0.16f, 0.14f, 1.f));
	}
	mDetailOverlayWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

#undef LOCTEXT_NAMESPACE
