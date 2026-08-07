#include "UI/Combat/CombatLayoutHUDWidget.h"

#include "GameFramework/Actor.h"   // FUnitUI.mViewActor->GetActorLocation() (HP바 라이브 투영)
#include "Camera/PlayerCameraManager.h"   // 카메라 줌(OrthoWidth) 비례 HP바 스케일
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/Combat/CombatUIModel.h"
#include "GameplayTagType.h"   // EffectTags::GameplayEffect_StatusEffect_* (상태이상 태그→아이콘 매핑)

namespace
{
	// (WBP 클래스/채움 텍스처는 UCombatLayoutHUDWidget 생성자에서 ConstructorHelpers로 하드 레퍼런스한다 — #300 컨벤션.)

	// HP바는 기존 크기를 유지한다. 상태이상만 아래의 독립 레이아웃 값으로 키운다.
	constexpr float UnitHpBarRenderScale = 0.52f;
	// 줌 배율 기준이 되는 직교 카메라 폭(CombatCameraPawn 기본 OrthoWidth). 이 값일 때 배율 1.0.
	constexpr float UnitHpBarBaseOrthoWidth = 2000.0f;

	// 머리 위 HP바가 유닛 머리에서 얼마나 위로 뜰지(뷰포트 픽셀). 크기 바꾸면 같이 조절.
	//
	// 바가 멀리 떠 보이던 진짜 원인은 오프셋이 아니라 **자동 크기**였다 —
	// 루트 슬롯이 WBP 크기를 그대로 받는데, 판 아래 숨은 상태이상 줄까지
	// 상자에 들어가 "바닥 정렬" 기준이 그 빈 공간 밑바닥이었다. 이제 슬롯을
	// 판 높이로 고정하므로(-8) 이 값이 곧 판 바닥과 머리 사이 간격이다.
	// -8 로 했더니 투영점이 머리가 아니라 유닛 가운데라 바가 유닛을 덮었다.
	// -52 도 헬멧에 살짝 걸려("좀 더 위로") 한 뼘 더 올린다.
	constexpr float UnitHpBarHeadOffsetY = -64.0f;
	// 판(백플레이트)만의 디자인 크기. 그 아래 상태이상 줄 자리는 상자에서 뺀다.
	constexpr float UnitHpBarPlateWidth = 360.0f;
	constexpr float UnitHpBarPlateHeight = 68.0f;

	// HP 숫자 줄 시작(채움 묶음 위에서부터, px).
	// 백플레이트 개구는 세로 15..53(그어 둔 칸 실측), 38pt 숫자 잉크는 37px에
	// 윗공백 9.6px — 가운데 정렬은 줄 상자(57px)가 개구보다 커서 못 쓰고,
	// 위 정렬에 이 여백을 줘 잉크를 개구 한가운데(34) 에 놓는다.
	constexpr float UnitHpBarValueTextPadTop = -2.0f;

	// HP 숫자 폰트 크기(WBP 디자인 좌표 기준 — 렌더 스케일이 곱해져 화면에 보임). 크게 해서 바를 꽉 채운다.
	constexpr float UnitHpBarValueFontSize = 38.0f;

	// 모바일에서는 한 유닛당 세 개를 크게 보여 주고 나머지는 +N으로 압축한다.
	// 활성 개수에 맞춰 매 프레임 가운데 정렬하므로 상태 하나가 HP바 왼쪽에 떨어지지 않는다.
	constexpr int32 UnitHpBarVisibleStatusSlots = 3;
	constexpr float UnitHpBarDesignWidth = 360.0f;
	constexpr float UnitHpBarStatusFrameSize = 176.0f;   // 얇은 원형 개별 프레임
	constexpr float UnitHpBarStatusIconSize = 156.0f;    // 프레임 안을 크게 채우는 아이콘
	constexpr float UnitHpBarStatusIconStep = 184.0f;    // 아이콘 사이 최소 여백
	constexpr float UnitHpBarStatusRowTop = 72.0f;       // HP바와 시각적으로 분리
	constexpr float UnitHpBarStatusCountFontSize = 42.0f;

	/**
	 * @brief 위젯에서 위로 올라가며 캔버스 슬롯을 가진 조상을 찾는다.
	 *
	 * WBP에서 글자를 Overlay(_Center/Mount)로 감싼 뒤로, 글자의 Slot을 바로
	 * Cast<UCanvasPanelSlot> 하던 코드는 조용히 실패했다 — HP 숫자가 38pt로
	 * 커진 채 자리 보정 없이 남아 바 밖으로 넘친 원인. 자리는 감싼 판이
	 * 쥐고 있으므로 그 판의 슬롯을 잡아야 한다.
	 */
	UCanvasPanelSlot* FindCanvasSlotUp(UWidget* Widget)
	{
		for (UWidget* Node = Widget; Node != nullptr; Node = Node->GetParent())
		{
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Node->Slot))
			{
				return CanvasSlot;
			}
		}
		return nullptr;
	}

	/**
	 * @brief 글자와 캔버스 조상 사이의 Overlay 슬롯들을 꽉 채움/세로 가운데로 편다.
	 *
	 * 감싼 판(HpFillImageMount)이 이미 채움 바 영역을 차지하므로, 사이 여백을
	 * 지우면 글자가 그 영역 전체를 쓰며 세로 가운데에 온다 — 감싸기 전에
	 * 캔버스 슬롯을 (20,34)·323x44로 옮기던 것과 같은 의도다.
	 */
	void SpreadTextInWrappers(UWidget* Widget)
	{
		for (UWidget* Node = Widget; Node != nullptr; Node = Node->GetParent())
		{
			if (Cast<UCanvasPanelSlot>(Node->Slot) != nullptr)
			{
				break;
			}
			if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Node->Slot))
			{
				OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
				OverlaySlot->SetVerticalAlignment(VAlign_Center);
				OverlaySlot->SetPadding(FMargin(0.0f));
			}
		}
	}

}

/**
 * @brief HP 채움 드레인을 준비한다: 채움 이미지 슬롯을 좌상단 기준으로 두고 원본 폭을 기록한다.
 * @details ClipToBounds 클립 캔버스로 "크롭"하는 방식은 유닛 바에 렌더 스케일(변환)을 걸면
 *          모바일 Vulkan에서 스텐실 클립 파이프라인이 없어 렌더 스레드가 죽는다(실측).
 *          그래서 클립 대신 채움 이미지의 슬롯 폭을 % 만큼 줄여 왼쪽 기준으로 비운다(솔리드 채움이라 크롭과 시각적으로 동일).
 */
void UCombatLayoutHUDWidget::SetupUnitHpBarFillClip(FCombatUnitHpBarWidget& Bar)
{
	if (Bar.mFillImage == nullptr)
	{
		return;
	}

	UCanvasPanelSlot* FillSlot = Cast<UCanvasPanelSlot>(Bar.mFillImage->Slot);
	if (FillSlot == nullptr)
	{
		return;
	}

	// 좌상단(0,0) 기준으로 두면, 폭을 줄일 때 왼쪽은 고정되고 오른쪽부터 비어 HP 드레인처럼 보인다.
	FillSlot->SetAlignment(FVector2D(0.0f, 0.0f));
	FillSlot->SetAutoSize(false);
	Bar.mFillFullWidth = FillSlot->GetSize().X;
	Bar.mFillClipSlot = FillSlot;   // Update에서 폭을 % 만큼 조절할 대상(채움 이미지 자체 슬롯).
}

/**
 * @brief WBP의 상태 슬롯 위젯을 캐시하고 접는다.
 *
 * @details 월드 상태 아이콘은 폐기했다(0807 결정) -- 유닛과 타일을 가린다.
 * WBP 에 위젯이 남아 있어 찾아서 접기만 한다. 무엇이 걸렸는지는 요약판이,
 * 뭔가 걸렸다는 신호는 상태 띠가 맡는다.
 */
void UCombatLayoutHUDWidget::CacheUnitHpBarStatusSlots(FCombatUnitHpBarWidget& Bar) const
{
	if (Bar.mRoot == nullptr)
	{
		return;
	}

	Bar.mStatusIcons.Reset();
	Bar.mStatusCountTexts.Reset();
	for (int32 SlotIndex = 1; SlotIndex <= 5; ++SlotIndex)
	{
		UImage* Icon = Cast<UImage>(Bar.mRoot->GetWidgetFromName(
			FName(*FString::Printf(TEXT("StatusIcon_%02d"), SlotIndex))));
		UTextBlock* Count = Cast<UTextBlock>(Bar.mRoot->GetWidgetFromName(
			FName(*FString::Printf(TEXT("StatusCountText_%02d"), SlotIndex))));
		if (Icon != nullptr) { Icon->SetVisibility(ESlateVisibility::Collapsed); }
		if (Count != nullptr) { Count->SetVisibility(ESlateVisibility::Collapsed); }
	}
	Bar.mStatusOverflowText = Cast<UTextBlock>(
		Bar.mRoot->GetWidgetFromName(TEXT("StatusOverflowText")));
	if (Bar.mStatusOverflowText != nullptr)
	{
		Bar.mStatusOverflowText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCombatLayoutHUDWidget::RebuildUnitHpBars()
{
	if (mRootCanvas == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	// (mUnitHpBarWidgetClass / mUnitHpFill{Red,Green}Texture 는 생성자에서 하드 레퍼런스로 이미 로드됨 — #300 컨벤션.)

	const int32 DesiredCount = mUIModel != nullptr ? mUIModel->GetUnitUIs().Num() : 0;

	/*
	 * HP바는 **판이 그린 것 전부보다 뒤**에 깔린다.
	 *
	 * 옛 HUD 는 겹이 둘(풀뷰포트 + 디자인 캔버스)이라 디자인 캔버스보다 낮은
	 * 값을 골랐다. 새 HUD 는 캔버스가 하나뿐이라 그 기준이 없어졌는데 값만
	 * 남아 있었다 -- WBP 가 그린 카드와 아군 칸이 0 근처라, 5 는 그것들보다
	 * **위**였다. 머리 위 바가 스킬 카드를 덮었다.
	 *
	 * 음수로 내려 판이 그린 것 아래로 확실히 보낸다.
	 */
	const int32 UnitHpBarZOrder = -10;


	// 유닛 수보다 많은 바는 제거.
	for (int32 BarIndex = mUnitHpBars.Num() - 1; BarIndex >= DesiredCount; --BarIndex)
	{
		if (mUnitHpBars[BarIndex].mRoot != nullptr)
		{
			mUnitHpBars[BarIndex].mRoot->RemoveFromParent();
		}
		mUnitHpBars.RemoveAt(BarIndex);
	}

	// 모자란 바는 새로 만든다.
	for (int32 BarIndex = mUnitHpBars.Num(); BarIndex < DesiredCount; ++BarIndex)
	{
		FCombatUnitHpBarWidget NewBar;
		if (mUnitHpBarWidgetClass != nullptr)
		{
			NewBar.mRoot = CreateWidget<UUserWidget>(this, mUnitHpBarWidgetClass);
		}
		if (NewBar.mRoot == nullptr)
		{
			continue;
		}

		NewBar.mRoot->SetVisibility(ESlateVisibility::Collapsed);

		// named widget 캐시 + 트리 수술(상태 슬롯 숨김, 채움 크롭 래핑)은
		// 반드시 AddChildToCanvas(Slate 트리 생성) "이전"에 끝낸다.
		// 라이브 트리에 붙인 뒤 UMG WidgetTree를 재부모화하면 Slate 트리와 어긋나 렌더 크래시가 난다.
		NewBar.mFillImage = Cast<UImage>(NewBar.mRoot->GetWidgetFromName(TEXT("HpFillImage")));
		NewBar.mValueText = Cast<UTextBlock>(NewBar.mRoot->GetWidgetFromName(TEXT("HpValueText")));
		NewBar.mBackplateImage = Cast<UImage>(
			NewBar.mRoot->GetWidgetFromName(TEXT("HpBackplateImage")));
		// 상태 띠는 판이 깔아 둔 두 칸을 쓴다(HpStatusRailBuff/Debuff).
		NewBar.mStatusRailBuff = Cast<UImage>(
			NewBar.mRoot->GetWidgetFromName(TEXT("HpStatusRailBuff")));
		NewBar.mStatusRailDebuff = Cast<UImage>(
			NewBar.mRoot->GetWidgetFromName(TEXT("HpStatusRailDebuff")));
		for (UImage* Rail : { NewBar.mStatusRailBuff.Get(), NewBar.mStatusRailDebuff.Get() })
		{
			if (Rail != nullptr)
			{
				Rail->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		CacheUnitHpBarStatusSlots(NewBar);   // 상태칸 캐시+숨김. 실제 표시는 UpdateUnitHpBarStatus에서 DTO로 매 프레임 채운다.
		SetupUnitHpBarFillClip(NewBar);

		// HP 숫자: 크게 + 외곽선(두껍게/대비) + 바(채움) 폭에 맞춰 가운데 정렬 → 작아도 잘 보이게.
		if (NewBar.mValueText != nullptr)
		{
			FSlateFontInfo ValueFont = NewBar.mValueText->GetFont();
			ValueFont.Size = UnitHpBarValueFontSize;
			ValueFont.OutlineSettings.OutlineSize = 3;
			ValueFont.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.9f);
			NewBar.mValueText->SetFont(ValueFont);
			NewBar.mValueText->SetJustification(ETextJustify::Center);
			NewBar.mValueText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			if (UCanvasPanelSlot* ValueSlot = Cast<UCanvasPanelSlot>(NewBar.mValueText->Slot))
			{
				// WBP 실측: 채움 영역 (x=20, y=8, w=323, h=53). 그 폭에 맞춰 가로로 꽉, 세로 중앙 근처.
				ValueSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				ValueSlot->SetAlignment(FVector2D(0.0f, 0.5f));
				ValueSlot->SetPosition(FVector2D(20.0f, 34.0f));
				ValueSlot->SetSize(FVector2D(323.0f, 44.0f));
				ValueSlot->SetAutoSize(false);
			}
			else
			{
				// 글자가 Overlay로 감싸져 캔버스 슬롯이 없다. 감싼 판이 이미
				// 채움 바 자리를 쥐고 있으니, 사이 여백만 지워 같은 결과를 낸다.
				SpreadTextInWrappers(NewBar.mValueText);
				// 세로는 가운데 정렬 대신 위 정렬 + 실측 여백. 38pt 줄 상자가
				// 묶음(53px)보다 커서 가운데 정렬은 조용히 위에 붙고 바닥이 잘렸다.
				if (UOverlaySlot* ValueTextSlot = Cast<UOverlaySlot>(NewBar.mValueText->Slot))
				{
					ValueTextSlot->SetVerticalAlignment(VAlign_Top);
					ValueTextSlot->SetPadding(FMargin(0.0f, UnitHpBarValueTextPadTop, 0.0f, 0.0f));
				}
			}
		}

		// HP바 왼쪽 방어도 표시(아이콘 + 수치). WBP 내부 캔버스에 런타임 생성 — 트리 수술 규칙에 따라
		// 반드시 AddChildToCanvas(라이브 부착) 전에 만든다. 방어도 0이면 Update에서 숨긴다.
		if (UWidgetTree* BarTree = NewBar.mRoot->WidgetTree)
		{
			if (UCanvasPanel* BarCanvas = Cast<UCanvasPanel>(BarTree->RootWidget))
			{
				if (mUnitDefenseIconTexture != nullptr)
				{
					if (UImage* DefenseIcon = BarTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DefenseIcon")))
					{
						DefenseIcon->SetBrushFromTexture(mUnitDefenseIconTexture, false);
						DefenseIcon->SetVisibility(ESlateVisibility::Collapsed);
						if (UCanvasPanelSlot* IconSlot = BarCanvas->AddChildToCanvas(DefenseIcon))
						{
							// WBP 실측(채움 x=20,y=8,w=323,h=53) 기준, 바 왼쪽 바깥에 세로 중앙 정렬.
							IconSlot->SetAnchors(FAnchors(0.0f, 0.0f));
							IconSlot->SetAlignment(FVector2D(1.0f, 0.5f));
							IconSlot->SetPosition(FVector2D(14.0f, 34.0f));
							IconSlot->SetSize(FVector2D(64.0f, 64.0f));
							IconSlot->SetAutoSize(false);
						}
						NewBar.mDefenseIcon = DefenseIcon;
					}
				}
				if (UTextBlock* DefenseText = BarTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DefenseValueText")))
				{
					FSlateFontInfo DefenseFont = DefenseText->GetFont();
					DefenseFont.Size = 26;
					DefenseFont.OutlineSettings.OutlineSize = 3;
					DefenseFont.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.9f);
					DefenseText->SetFont(DefenseFont);
					DefenseText->SetJustification(ETextJustify::Center);
					DefenseText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
					DefenseText->SetVisibility(ESlateVisibility::Collapsed);
					if (UCanvasPanelSlot* TextSlot = BarCanvas->AddChildToCanvas(DefenseText))
					{
						// 아이콘(방패) 위 중앙에 수치를 겹쳐 그린다.
						TextSlot->SetAnchors(FAnchors(0.0f, 0.0f));
						TextSlot->SetAlignment(FVector2D(1.0f, 0.5f));
						TextSlot->SetPosition(FVector2D(10.0f, 34.0f));
						TextSlot->SetSize(FVector2D(56.0f, 40.0f));
						TextSlot->SetAutoSize(false);
						TextSlot->SetZOrder(1);
					}
					NewBar.mDefenseText = DefenseText;
				}
			}
		}

		// 크기 축소: 렌더 스케일을 하단 중앙 피벗으로 적용해, 축소해도 바 밑변이 투영 지점(머리 위)에 고정되게 한다.
		NewBar.mRoot->SetRenderTransformPivot(FVector2D(0.5f, 1.0f));
		NewBar.mRoot->SetRenderScale(FVector2D(UnitHpBarRenderScale, UnitHpBarRenderScale));

		// [Adreno Vulkan RHIThread 크래시 방지] 위 렌더 스케일(변환)이 걸린 HP바 서브트리 안에 ClipToBounds 위젯이 있으면
		// 모바일 Vulkan에서 스텐실-클립 파이프라인이 없어 vkCmdBindPipeline(null)로 렌더 스레드가 즉사한다(실측).
		// (기존 대응은 채움 크롭의 클립만 슬롯폭으로 우회 — 변환은 남아있음.) 변환 서브트리의 나머지 클립도 강제로 끈다.
		// 솔리드 채움/아이콘/텍스트라 클립이 없어도 시각 차이는 거의 없다.
		NewBar.mRoot->SetClipping(EWidgetClipping::Inherit);
		if (UWidgetTree* BarWidgetTree = NewBar.mRoot->WidgetTree)
		{
			BarWidgetTree->ForEachWidget([](UWidget* ChildWidget)
			{
				if (ChildWidget != nullptr && ChildWidget->GetClipping() == EWidgetClipping::ClipToBounds)
				{
					ChildWidget->SetClipping(EWidgetClipping::Inherit);
				}
			});
		}

		// 트리 확정 후 라이브 캔버스에 붙인다.
		mRootCanvas->AddChildToCanvas(NewBar.mRoot);
		if (UCanvasPanelSlot* RootSlot = Cast<UCanvasPanelSlot>(NewBar.mRoot->Slot))
		{
			// 자동 크기를 쓰면 판 아래 숨은 상태이상 줄 자리(빈 33px)까지
			// 상자에 들어가, 바닥 정렬 기준이 그 밑바닥이 된다 — 바가 유닛
			// 머리에서 멀리 떠 보이던 원인. 판 크기로 못 박는다.
			RootSlot->SetAutoSize(false);
			RootSlot->SetSize(FVector2D(UnitHpBarPlateWidth, UnitHpBarPlateHeight));
			RootSlot->SetAlignment(FVector2D(0.5f, 1.0f));    // 가로 중앙, 세로 아래 기준(유닛 머리 위에 놓기).
			RootSlot->SetZOrder(UnitHpBarZOrder);             // 스킨 뒤로(다른 UI를 안 덮게).
		}

		mUnitHpBars.Add(NewBar);
	}

	UpdateUnitHpBars();
}

void UCombatLayoutHUDWidget::UpdateUnitHpBars()
{
	// 지도(풀스크린) 열림 중에는 유닛 머리 위 HP바를 숨긴다 — 탑바만 남기는 뷰.
	// 이 함수가 매 틱 HP바를 강제 표시(라인 283)하므로, SetCombatPlayControlsVisible에서 한 번 숨기는 것으로는
	// 다음 틱에 되살아난다. 여기서 게이트해야 지도 뷰 동안 계속 숨겨진다.
	if (GetVisibility() == ESlateVisibility::Collapsed)
	{
		for (FCombatUnitHpBarWidget& Bar : mUnitHpBars)
		{
			if (Bar.mRoot != nullptr) { Bar.mRoot->SetVisibility(ESlateVisibility::Collapsed); }
		}
		return;
	}

	if (mUIModel == nullptr)
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController == nullptr)
	{
		return;
	}

	// 카메라 줌(직교 OrthoWidth)에 비례해 HP바 크기를 키운다 — 줌인하면 유닛과 함께 바도 커진다.
	// 기준 OrthoWidth(2000) 대비 배율, 과도한 확대/축소는 클램프.
	float ZoomScale = 1.0f;
	if (PlayerController->PlayerCameraManager != nullptr)
	{
		const float CurrentOrthoWidth = PlayerController->PlayerCameraManager->GetCameraCacheView().OrthoWidth;
		if (CurrentOrthoWidth > KINDA_SMALL_NUMBER)
		{
			ZoomScale = FMath::Clamp(UnitHpBarBaseOrthoWidth / CurrentOrthoWidth, 0.6f, 3.0f);
		}
	}
	const FVector2D BarRenderScale(UnitHpBarRenderScale * ZoomScale, UnitHpBarRenderScale * ZoomScale);

	const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();
	for (int32 BarIndex = 0; BarIndex < mUnitHpBars.Num(); ++BarIndex)
	{
		FCombatUnitHpBarWidget& Bar = mUnitHpBars[BarIndex];
		if (Bar.mRoot == nullptr || Units.IsValidIndex(BarIndex) == false)
		{
			continue;
		}
		const FUnitUI& Unit = Units[BarIndex];

		const float Percent = Unit.mMaxHP > 0.0f ? FMath::Clamp(Unit.mHP / Unit.mMaxHP, 0.0f, 1.0f) : 0.0f;

		// 채움 색: 아군=초록, 적=빨강 텍스처. (없으면 기존 브러시 유지)
		if (Bar.mFillImage != nullptr)
		{
			UTexture2D* FillTexture = Unit.mIsPlayer ? mUnitHpFillGreenTexture : mUnitHpFillRedTexture;
			if (FillTexture != nullptr)
			{
				Bar.mFillImage->SetBrushResourceObject(FillTexture);
			}
		}

		// 채움 드레인: 채움 이미지 슬롯 폭을 원본 폭 × 퍼센트로 줄인다(좌상단 고정이라 오른쪽부터 HP가 빠진다).
		if (Bar.mFillClipSlot != nullptr && Bar.mFillFullWidth > 0.0f)
		{
			FVector2D FillSize = Bar.mFillClipSlot->GetSize();
			FillSize.X = Bar.mFillFullWidth * Percent;
			Bar.mFillClipSlot->SetSize(FillSize);
		}

		// HP 숫자: 모바일에서 현재치만 보면 최대치와 피해 비율을 다시
		// 추론해야 한다. 바 안에 현재/최대를 같이 크게 표시한다.
		if (Bar.mValueText != nullptr)
		{
			Bar.mValueText->SetText(FText::FromString(FString::Printf(
				TEXT("%d/%d"), FMath::RoundToInt(Unit.mHP),
				FMath::RoundToInt(Unit.mMaxHP))));
		}

		// 전투 HUD의 공개 수치는 HP/AP/속도뿐이다. 구형 방어도 슬롯이
		// 남아 있더라도 월드 HP바에서는 항상 감춘다.
		const ESlateVisibility DefenseVisibility = ESlateVisibility::Collapsed;
		if (Bar.mDefenseIcon != nullptr)
		{
			Bar.mDefenseIcon->SetVisibility(DefenseVisibility);
		}
		if (Bar.mDefenseText != nullptr)
		{
			Bar.mDefenseText->SetVisibility(DefenseVisibility);
		}

		// 유닛 월드 위치를 화면(위젯) 좌표로 투영. 화면 밖이면 숨긴다.
		// 이동을 매 프레임 따라가도록 뷰 액터의 라이브 위치를 우선 투영(유효 시). 없으면 스냅샷(mWorldLocation) 폴백.
		const FVector ProjectLocation = Unit.mViewActor.IsValid()
			? Unit.mViewActor->GetActorLocation()
			: Unit.mWorldLocation;
		FVector2D ScreenPosition;
		const bool bOnScreen = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PlayerController, ProjectLocation, ScreenPosition, false);
		if (bOnScreen == false)
		{
			Bar.mRoot->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		if (UCanvasPanelSlot* RootSlot = Cast<UCanvasPanelSlot>(Bar.mRoot->Slot))
		{
			RootSlot->SetPosition(ScreenPosition + FVector2D(0.0f, UnitHpBarHeadOffsetY));   // 유닛 머리 위로 띄운다.
		}

		// 줌 배율 반영(하단 중앙 피벗이라 커져도 밑변이 머리 위 투영점에 고정).
		Bar.mRoot->SetRenderScale(BarRenderScale);

		// HP바 밑 상태이상 아이콘/개수 갱신(온스크린 확정된 바에만).
		UpdateUnitHpBarStatus(Bar, Unit);

		Bar.mRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

/**
 * @brief 유닛 HP바의 상태 표시(테두리 색 + 밑 상태 띠)를 DTO(mStatusEffects)로 갱신한다.
 */
void UCombatLayoutHUDWidget::UpdateUnitHpBarStatus(FCombatUnitHpBarWidget& Bar, const FUnitUI& Unit) const
{
	/*
	 * 테두리 색으로 버프/디버프를 알린다(0806 합의).
	 *
	 * 월드 위 상태 아이콘은 유닛과 타일을 가려서 껐다. 그래도 "지금 뭔가
	 * 걸려 있다" 는 판에서 바로 보여야 한다. 무엇이 걸렸는지는 요약판이
	 * 아이콘으로 맡고, 여기서는 좋은 것/나쁜 것만 색으로 구분한다.
	 * 둘 다 걸리면 두 색을 섞는다 -- 하나만 골라 보이면 나머지가 숨는다.
	 */
	const FLinearColor BuffTint(0.42f, 1.00f, 0.55f, 1.0f);    // 이로운 것
	const FLinearColor DebuffTint(1.00f, 0.42f, 0.42f, 1.0f);  // 해로운 것
	bool bHasBuff = false;
	bool bHasDebuff = false;
	for (const FStatusEffectUI& Status : Unit.mStatusEffects)
	{
		const FString TagName = Status.mTag.GetTagName().ToString();
		bHasDebuff |= TagName.Contains(TEXT(".Debuff."));
		// "Debuff" 안에도 "Buff" 가 들어 있어 앞에 점을 붙여 가른다.
		bHasBuff |= TagName.Contains(TEXT(".Buff."));
	}
	FLinearColor StatusTint = FLinearColor::White;
	if (bHasBuff && bHasDebuff)
	{
		StatusTint = (BuffTint + DebuffTint) * 0.5f;
		StatusTint.A = 1.0f;
	}
	else if (bHasBuff)
	{
		StatusTint = BuffTint;
	}
	else if (bHasDebuff)
	{
		StatusTint = DebuffTint;
	}
	if (Bar.mBackplateImage != nullptr)
	{
		Bar.mBackplateImage->SetColorAndOpacity(StatusTint);
	}

	/*
	 * HP바 밑 상태 띠.
	 *
	 * 종류를 **자리로** 가른다 -- 왼쪽이 이로운 것, 오른쪽이 해로운 것.
	 * 하나만 걸렸으면 그 띠가 폭을 다 쓰고, 둘 다면 반씩 나눠 가진다.
	 * 색만으로 뜻을 나르면 색을 못 가리는 사람에게는 아무 표시도 아니다.
	 */
	const float RailInset = 20.0f;   // 판 테두리 안쪽(채움과 같은 여백)
	const float RailWidth = UnitHpBarPlateWidth - RailInset * 2.0f;
	const float RailTop = UnitHpBarPlateHeight - 2.0f;
	const float RailHeight = 9.0f;
	const bool bBothRails = bHasBuff && bHasDebuff;
	const float HalfWidth = (RailWidth - 4.0f) * 0.5f;

	if (Bar.mStatusRailBuff != nullptr)
	{
		Bar.mStatusRailBuff->SetVisibility(bHasBuff
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bHasBuff == true)
		{
			if (UCanvasPanelSlot* RailSlot =
				Cast<UCanvasPanelSlot>(Bar.mStatusRailBuff->Slot))
			{
				RailSlot->SetPosition(FVector2D(RailInset, RailTop));
				RailSlot->SetSize(FVector2D(
					bBothRails ? HalfWidth : RailWidth, RailHeight));
			}
			Bar.mStatusRailBuff->SetColorAndOpacity(BuffTint);
		}
	}
	if (Bar.mStatusRailDebuff != nullptr)
	{
		Bar.mStatusRailDebuff->SetVisibility(bHasDebuff
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bHasDebuff == true)
		{
			if (UCanvasPanelSlot* RailSlot =
				Cast<UCanvasPanelSlot>(Bar.mStatusRailDebuff->Slot))
			{
				// 둘 다면 오른쪽 반, 아니면 띠 전체를 쓴다.
				RailSlot->SetPosition(FVector2D(
					bBothRails ? RailInset + HalfWidth + 4.0f : RailInset, RailTop));
				RailSlot->SetSize(FVector2D(
					bBothRails ? HalfWidth : RailWidth, RailHeight));
			}
			Bar.mStatusRailDebuff->SetColorAndOpacity(DebuffTint);
		}
	}

	/*
	 * 월드 상태 아이콘은 폐기했다(0807 결정). 유닛과 타일을 가려서 껐다가
	 * 다시 안 살리기로 했다 -- "무엇이 걸렸는지" 는 요약판 아이콘이,
	 * "뭔가 걸렸다" 는 위의 상태 띠가 맡는다. WBP 에 남은 아이콘 줄 위젯은
	 * 여기서 늘 접어 둔다.
	 */
	for (UImage* Icon : Bar.mStatusIcons)
	{
		if (Icon != nullptr) { Icon->SetVisibility(ESlateVisibility::Collapsed); }
	}
	for (UImage* Frame : Bar.mStatusFrames)
	{
		if (Frame != nullptr) { Frame->SetVisibility(ESlateVisibility::Collapsed); }
	}
	for (UTextBlock* Count : Bar.mStatusCountTexts)
	{
		if (Count != nullptr) { Count->SetVisibility(ESlateVisibility::Collapsed); }
	}
	if (Bar.mStatusOverflowText != nullptr)
	{
		Bar.mStatusOverflowText->SetVisibility(ESlateVisibility::Collapsed);
	}
}
