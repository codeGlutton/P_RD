#include "UI/MonsterTabWidgetBuilder.h"
#include "UI/UIPartRects.h"
#include "UI/UIFont.h"

#include "Animation/WidgetAnimation.h"
#include "AssetToolsModule.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditorUtils.h"
#include "WidgetBlueprintFactory.h"

namespace MonsterTabWidgetBuilder
{
	constexpr TCHAR PackagePath[] = TEXT("/Game/UI/MonsterTab");
	constexpr TCHAR AssetName[] = TEXT("WBP_MonsterTab_Marchbound");
	constexpr TCHAR AssetPath[] =
		TEXT("/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound.WBP_MonsterTab_Marchbound");
	TUniquePtr<FAutoConsoleCommand> BuildCommand;

	UTexture2D* Texture(const TCHAR* Path)
	{
		UTexture2D* Result = LoadObject<UTexture2D>(nullptr, Path);
		checkf(Result != nullptr, TEXT("Missing monster-tab texture: %s"), Path);
		return Result;
	}

	void Place(UCanvasPanel* Parent, UWidget* Child, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder)
	{
		UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Child);
		Slot->SetAnchors(FAnchors(0.0f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
	}

	void Expose(UWidgetBlueprint* Blueprint, UWidget* Widget)
	{
		check(Blueprint != nullptr && Widget != nullptr);
		// 이미 GUID 가 있는 이름을 또 등록하면 ensure 가 뜬다. 이 빌더는 이미
		// 만들어진 판 위에 다시 도는 것이 정상이라 두 번째부터는 늘 그랬다.
		if (Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
		{
			return;
		}
		Blueprint->OnVariableAdded(Widget->GetFName());
	}

	void StyleText(UTextBlock* Text, const int32 Size,
		const FLinearColor Color, const ETextJustify::Type Justification = ETextJustify::Center)
	{
		FSlateFontInfo Font = UIFont::Make(Text->GetFont(), Size);
		Font.OutlineSettings.OutlineSize = Size >= 27 ? 1 : 0;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.02f, 0.01f, 0.0f, 0.95f);
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(Justification);
		Text->SetShadowOffset(FVector2D(1.5f, 1.5f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.02f, 0.01f, 0.0f, 0.8f));
		Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	UTextBlock* AddText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, const FText Value, const int32 FontSize,
		const FVector2D Position, const FVector2D Size, const int32 ZOrder,
		const FLinearColor Color = FLinearColor(0.12f, 0.065f, 0.025f, 1.0f),
		const ETextJustify::Type Justification = ETextJustify::Center)
	{
		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), Name);
		Text->SetText(Value);
		StyleText(Text, FontSize, Color, Justification);
		Place(Parent, Text, Position, Size, ZOrder);
		Expose(Blueprint, Text);
		return Text;
	}

	UImage* AddImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, UTexture2D* Source, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder, const bool bExpose = false)
	{
		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), Name);
		Image->SetBrushFromTexture(Source, false);
		Image->SetColorAndOpacity(FLinearColor::White);
		Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Image, Position, Size, ZOrder);
		if (bExpose)
		{
			Expose(Blueprint, Image);
		}
		return Image;
	}

	/**
	 * @brief 늘려도 모서리가 안 뭉개지게 9-slice 로 그리는 그림.
	 *
	 * KitA 부품은 나무 틀에 금 모서리가 박혀 있어 통짜로 늘리면 장식까지 늘어난다.
	 * 여백은 실측값(Saved/UIKit/ConceptA/_nineslice.txt)에 잘라낼 때 남긴 투명
	 * 여백 8px 을 더한 값이고, Box 브러시 마진은 그림 크기 대비 비율이다.
	 */
	UImage* AddSlicedImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, UTexture2D* Source, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder, const FVector2D BorderPx,
		const float PadPx = 8.f)
	{
		UImage* Image = AddImage(Blueprint, Parent, Name, Source, Position, Size, ZOrder);
		if (Source == nullptr)
		{
			return Image;
		}
		// GetSizeX() 는 갓 LoadObject 한 텍스처에서 0 을 준다(플랫폼 데이터가
		// 아직 없다). 0 으로 나눠 마진이 inf 가 됐다 -- 실제로 그렇게 나왔다.
		const FIntPoint Imported = Source->GetImportedSize();
		if (Imported.X <= 0 || Imported.Y <= 0)
		{
			return Image;
		}
		const FVector2D Texel(Imported.X, Imported.Y);
		FSlateBrush Brush = Image->GetBrush();
		Brush.DrawAs = ESlateBrushDrawType::Box;
		// 투명 여백은 부품 시트에서 잘라낸 것만 8px 이다. 틀 그림은 원본에서
		// 바로 떠서 0 이다 -- 무조건 8 을 더하고 있어 테두리가 8px 씩 밀렸다.
		Brush.Margin = FMargin(
			(PadPx + BorderPx.X) / Texel.X, (PadPx + BorderPx.Y) / Texel.Y,
			(PadPx + BorderPx.X) / Texel.X, (PadPx + BorderPx.Y) / Texel.Y);
		Image->SetBrush(Brush);
		return Image;
	}

	UButton* AddTransparentButton(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, const FVector2D Position, const FVector2D Size, const int32 ZOrder)
	{
		UButton* Button = Blueprint->WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), Name);
		FButtonStyle Style;
		FSlateBrush Empty;
		Empty.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.SetNormal(Empty);
		Style.SetHovered(Empty);
		Style.SetPressed(Empty);
		Style.SetDisabled(Empty);
		Button->SetStyle(Style);
		Button->SetBackgroundColor(FLinearColor::Transparent);
		Button->SetColorAndOpacity(FLinearColor::White);
		Place(Parent, Button, Position, Size, ZOrder);
		Expose(Blueprint, Button);
		return Button;
	}

	UBorder* AddDarkWell(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, const FVector2D Position, const FVector2D Size, const int32 ZOrder)
	{
		UBorder* Border = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), Name);
		Border->SetBrushColor(FLinearColor(0.055f, 0.028f, 0.012f, 0.88f));
		Border->SetPadding(FMargin(10.0f));
		Border->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Border, Position, Size, ZOrder);
		Expose(Blueprint, Border);
		return Border;
	}

	UWidgetBlueprint* FindOrCreateBlueprint()
	{
		if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(nullptr, AssetPath))
		{
			return Existing;
		}

		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->ParentClass = UUserWidget::StaticClass();
		FAssetToolsModule& AssetTools =
			FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		return Cast<UWidgetBlueprint>(AssetTools.Get().CreateAsset(
			AssetName, PackagePath, UWidgetBlueprint::StaticClass(), Factory));
	}

	void Build()
	{
		UWidgetBlueprint* Blueprint = FindOrCreateBlueprint();
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_MONSTER_TAB_BUILD could not create %s"), AssetPath);
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		Blueprint->ParentClass = UUserWidget::StaticClass();
		// 이 빌더는 부분 갱신이 아니라 트리를 통째로 다시 만든다. 이전 Root를
		// 단순 교체하면 자식 UObject가 WidgetTree 안에 고아로 남아 GUID ensure를
		// 낸다. 에디터 삭제 경로로 자식까지 transient package로 옮긴다.
		if (UWidget* PreviousRoot = Blueprint->WidgetTree->RootWidget)
		{
			FWidgetBlueprintEditorUtils::DeleteWidgets(
				Blueprint, { PreviousRoot },
				FWidgetBlueprintEditorUtils::EDeleteWidgetWarningType::DeleteSilently);
		}

		UOverlay* Root = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("MonsterTabViewportRoot"));
		Blueprint->WidgetTree->RootWidget = Root;

		UBorder* Dimmer = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("MonsterTabWorldDimmer"));
		Dimmer->SetBrushColor(FLinearColor(0.015f, 0.008f, 0.003f, 0.64f));
		Root->AddChildToOverlay(Dimmer);
		CastChecked<UOverlaySlot>(Dimmer->Slot)->SetHorizontalAlignment(HAlign_Fill);
		CastChecked<UOverlaySlot>(Dimmer->Slot)->SetVerticalAlignment(VAlign_Fill);

		UScaleBox* ScreenScale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("MonsterTabScale"));
		ScreenScale->SetStretch(EStretch::ScaleToFit);
		ScreenScale->SetStretchDirection(EStretchDirection::Both);
		ScreenScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Root->AddChildToOverlay(ScreenScale);
		CastChecked<UOverlaySlot>(ScreenScale->Slot)->SetHorizontalAlignment(HAlign_Fill);
		CastChecked<UOverlaySlot>(ScreenScale->Slot)->SetVerticalAlignment(VAlign_Fill);

		USizeBox* DesignSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("MonsterTabDesignSize"));
		DesignSize->SetWidthOverride(1920.0f);
		DesignSize->SetHeightOverride(1080.0f);
		ScreenScale->AddChild(DesignSize);

		UCanvasPanel* Canvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("MonsterTabCanvas"));
		DesignSize->SetContent(Canvas);

		// 용병탭과 같은 한 장짜리 구성: 왼쪽 선택 카드, 가운데 초상화,
		// 오른쪽 상세. 옛 3열 기둥은 별도 화면처럼 보이고 좁은 화면에서
		// 글자와 초상화를 동시에 눌렀으므로 사용하지 않는다.
		const FVector2D LeftCell(150.f, 190.f);
		const FVector2D LeftSize(390.f, 760.f);
		const FVector2D MidCell(590.f, 190.f);
		const FVector2D MidSize(450.f, 760.f);
		const FVector2D RightCell(1080.f, 170.f);
		const FVector2D RightSize(690.f, 790.f);
		const float Gutter = 14.f;

		// 옛 통짜 3열 프레임은 지웠다. 열 경계가 그림에 박혀 있어
		// 열 비율을 못 바꾸고, 16:9 가 아닌 폰에서 나무가 늘어났다.
		// 이제 바깥 틀 한 장 + 기둥 두 개를 코드가 놓는다.
		UTexture2D* BaseFrame = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Common/Frames/T_KitA_Frame_Outer.T_KitA_Frame_Outer"));
		UTexture2D* RowNormal = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Combat/HUD/T_MB_MercenaryCard_Normal.T_MB_MercenaryCard_Normal"));
		UTexture2D* RowSelected = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Combat/HUD/T_MB_MercenaryCard_Selected.T_MB_MercenaryCard_Selected"));
		// 같은 기능인데 지도·탭·설정이 서로 다른 단추를 쓰고 있었다(0804 검수).
		// 공용 KitA 단추 하나로 모은다.
		UTexture2D* BackButtonTexture = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Common/Controls/Buttons/T_KitA_Button_Small_Normal.T_KitA_Button_Small_Normal"));
		// 바깥 틀은 9-slice 로 편다. 모서리 장식이 x91 · y98 까지라 그 안쪽만 늘어난다
		// (Saved/UIKit/_slice_margins.txt 실측).
		AddDarkWell(Blueprint, Canvas, TEXT("MonsterTabContentWell"),
			FVector2D(112.f, 176.f), FVector2D(1696.f, 760.f), -2);
		AddSlicedImage(Blueprint, Canvas, TEXT("MonsterTabBaseFrame"), BaseFrame,
			FVector2D::ZeroVector, FVector2D(1920.0f, 1080.0f), 0,
			FVector2D(91.0f, 98.0f), 0.f);

		UTextBlock* Title = AddText(Blueprint, Canvas, TEXT("MonsterTabTitleText"),
			NSLOCTEXT("MarchboundMonsterTab", "Title", "몬스터"), 62,
			FVector2D(689.0f, 45.0f), FVector2D(542.0f, 102.0f), 10,
			FLinearColor(1.0f, 0.93f, 0.78f, 1.0f));
		Title->SetShadowOffset(FVector2D(3.0f, 3.0f));

		/*
		 * 배치는 위에서 정한 세 칸을 쓴다. 기둥은 그 칸 사이에 세운다.
		 */
		const FLinearColor Accent(0.95f, 0.78f, 0.42f, 1.0f);
		const FLinearColor Ink(0.12f, 0.065f, 0.025f, 1.0f);

		// 용병판과 같은 하단 닫기 동선.
		AddSlicedImage(Blueprint, Canvas, TEXT("MonsterBackArt"), BackButtonTexture,
			FVector2D(825.0f, 944.0f), FVector2D(270.0f, 96.0f), 30, FVector2D(44.0f, 35.0f));
		AddText(Blueprint, Canvas, TEXT("MonsterBackText"),
			NSLOCTEXT("MarchboundMonsterTab", "Back", "닫기"), 34,
			FVector2D(825.0f, 956.0f), FVector2D(270.0f, 68.0f), 31,
			FLinearColor(1.0f, 0.93f, 0.78f, 1.0f));
		AddTransparentButton(Blueprint, Canvas, TEXT("MonsterBackButton"),
			FVector2D(825.0f, 944.0f), FVector2D(270.0f, 96.0f), 32);

		// ── 왼쪽 칸: 나온 순서대로 채우는 몬스터 명단 ──────────────────
		AddText(Blueprint, Canvas, TEXT("MonsterListHeading"),
			NSLOCTEXT("MarchboundMonsterTab", "ListHeading", "출현 몬스터"), 30,
			FVector2D(LeftCell.X + Gutter, LeftCell.Y + 14.0f),
			FVector2D(LeftSize.X - Gutter * 2.f, 44.0f), 20, Accent, ETextJustify::Left);

		const TCHAR* MonsterNames[3] = { TEXT("독수리"), TEXT("늑대인간"), TEXT("골렘") };
		const TCHAR* HeadPaths[3] = {
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Enemies/Portraits/KK_Face_Enemy_Eagle_HeadV2.KK_Face_Enemy_Eagle_HeadV2"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Enemies/Portraits/KK_Face_Enemy_Werewolf_HeadV2.KK_Face_Enemy_Werewolf_HeadV2"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Enemies/Portraits/KK_Face_Enemy_Golem_HeadV2.KK_Face_Enemy_Golem_HeadV2")
		};
		const float RowWidth = 350.f;
		const float RowHeight = 190.f;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			UCanvasPanel* Row = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(),
				FName(*FString::Printf(TEXT("MonsterRow_%d"), Index)));
			Place(Canvas, Row,
				FVector2D(LeftCell.X + Gutter, LeftCell.Y + 62.0f + (RowHeight + 26.0f) * Index),
				FVector2D(RowWidth, RowHeight), 20);
			Expose(Blueprint, Row);

			AddImage(Blueprint, Row,
				FName(*FString::Printf(TEXT("MonsterRowNormal_%d"), Index)), RowNormal,
				FVector2D::ZeroVector, FVector2D(RowWidth, RowHeight), 0, true);
			UImage* Selected = AddImage(Blueprint, Row,
				FName(*FString::Printf(TEXT("MonsterRowSelected_%d"), Index)), RowSelected,
				FVector2D::ZeroVector, FVector2D(RowWidth, RowHeight), 2, true);
			Selected->SetVisibility(Index == 0
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);

			UImage* Portrait = AddImage(Blueprint, Row,
				FName(*FString::Printf(TEXT("MonsterRowPortrait_%d"), Index)),
				Texture(HeadPaths[Index]), FVector2D(21.f, 24.f),
				FVector2D(142.f, 142.f), 6, true);
			Portrait->SetClipping(EWidgetClipping::ClipToBoundsAlways);
			AddText(Blueprint, Row,
				FName(*FString::Printf(TEXT("MonsterRowName_%d"), Index)),
				FText::FromString(MonsterNames[Index]), 32,
				FVector2D(171.f, 54.f), FVector2D(153.f, 64.f), 8,
				Ink, ETextJustify::Left);
			AddTransparentButton(Blueprint, Row,
				FName(*FString::Printf(TEXT("MonsterRowButton_%d"), Index)),
				FVector2D::ZeroVector, FVector2D(RowWidth, RowHeight), 20);
		}

		// ── 가운데 칸: 고른 몬스터의 모습과 이름 ────────────────────────
		UTexture2D* PortraitCell = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Common/Frames/T_KitA_Cell_Normal.T_KitA_Cell_Normal"));
		AddImage(Blueprint, Canvas, TEXT("MonsterPortraitFrame"), PortraitCell,
			FVector2D(MidCell.X + Gutter, MidCell.Y + 70.0f),
			FVector2D(MidSize.X - Gutter * 2.f, MidSize.X - Gutter * 2.f), 14, true);
		UScaleBox* PortraitScale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("MonsterDetailPortraitScale"));
		PortraitScale->SetStretch(EStretch::ScaleToFit);
		PortraitScale->SetStretchDirection(EStretchDirection::Both);
		PortraitScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Place(Canvas, PortraitScale, FVector2D(MidCell.X + Gutter, MidCell.Y + 70.0f),
			FVector2D(MidSize.X - Gutter * 2.f, MidSize.X - Gutter * 2.f), 15);
		UImage* DetailPortrait = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("MonsterDetailPortrait"));
		DetailPortrait->SetBrushFromTexture(Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Enemies/Portraits/KK_Face_Enemy_Werewolf_ActionV3.KK_Face_Enemy_Werewolf_ActionV3")), false);
		DetailPortrait->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		PortraitScale->AddChild(DetailPortrait);
		Expose(Blueprint, DetailPortrait);

		AddText(Blueprint, Canvas, TEXT("MonsterCenterNameText"),
			NSLOCTEXT("MarchboundMonsterTab", "CenterWerewolf", "늑대인간"), 44,
			FVector2D(MidCell.X + Gutter, MidCell.Y + 520.0f),
			FVector2D(MidSize.X - Gutter * 2.f, 76.0f), 20);
		AddText(Blueprint, Canvas, TEXT("MonsterDetailTypeText"),
			NSLOCTEXT("MarchboundMonsterTab", "DetailType", "야수 · 근접"), 28,
			FVector2D(MidCell.X + Gutter, MidCell.Y + 600.0f),
			FVector2D(MidSize.X - Gutter * 2.f, 48.0f), 20);

		/*
		 * ── 오른쪽 칸: 고른 몬스터의 값 ──────────────────────────────
		 *
		 * 왼쪽은 **고르는 곳**, 오른쪽은 **고른 결과**다. 전에는 수치와 상태가
		 * 왼쪽 목록 바로 밑에 붙어 있어서, 줄을 눌러도 무엇이 바뀌었는지
		 * 눈이 못 따라갔다.
		 *
		 *     이름
		 *     수치 칩 넷      HP · AP · 방어 · 속도
		 *     HP 막대
		 *     스킬 칸 넷      아이콘 한 줄
		 *     상태
		 */
		const float RightInner = RightSize.X - Gutter * 2.f;
		const float RightLeft = RightCell.X + Gutter;
		float Cursor = RightCell.Y + 20.f;

		AddText(Blueprint, Canvas, TEXT("MonsterDetailNameText"),
			NSLOCTEXT("MarchboundMonsterTab", "DetailWerewolf", "늑대인간"), 54,
			FVector2D(RightLeft, Cursor), FVector2D(RightInner, 82.0f), 20, Ink);
		Cursor += 96.f;

		UTexture2D* ChipRing = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Common/Frames/T_KitA_StatChip_Ring.T_KitA_StatChip_Ring"));
		const TCHAR* const ChipValueNames[4] = {
			TEXT("MonsterDetailHPText"), TEXT("MonsterDetailAPText"),
			TEXT("MonsterDetailDefenseText"), TEXT("MonsterDetailSpeedText") };
		const TCHAR* const ChipLabels[4] = {
			TEXT("HP"), TEXT("AP"), TEXT("방어"), TEXT("속도") };
		const float ChipRoom = RightInner / 4.f;
		const float ChipSide = FMath::Min(ChipRoom - 14.f, 152.f);
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FVector2D ChipPos(
				RightLeft + ChipRoom * Index + (ChipRoom - ChipSide) * 0.5f, Cursor);
			const FVector2D ChipSize(ChipSide, ChipSide);
			AddImage(Blueprint, Canvas, FName(*FString::Printf(
				TEXT("MonsterChip%dFrame"), Index)), ChipRing, ChipPos, ChipSize, 19, true);
			// 글자는 링 **안쪽 구멍**에만 넣는다. 링을 밟으면 안 읽힌다.
			const FBox2D Ring = UIPartRects::Inner(TEXT("T_KitA_StatChip_Ring"),
				ChipPos, ChipSize, false);
			const FVector2D RingSpan = Ring.GetSize();
			AddText(Blueprint, Canvas, FName(*FString::Printf(
				TEXT("MonsterChip%dLabel"), Index)), FText::FromString(ChipLabels[Index]),
				FMath::RoundToInt(RingSpan.Y * 0.26f), Ring.Min,
				FVector2D(RingSpan.X, RingSpan.Y * 0.38f), 20, Accent);
			AddText(Blueprint, Canvas, ChipValueNames[Index], FText::FromString(TEXT("-")),
				FMath::RoundToInt(RingSpan.Y * 0.42f),
				Ring.Min + FVector2D(0.f, RingSpan.Y * 0.38f),
				FVector2D(RingSpan.X, RingSpan.Y * 0.62f), 20, Ink);
		}
		Cursor += ChipSide + 18.f;

		// HP 막대. 칩의 숫자와 같은 값이지만 남은 양은 막대가 한눈에 보인다.
		UProgressBar* HPBar = Blueprint->WidgetTree->ConstructWidget<UProgressBar>(
			UProgressBar::StaticClass(), TEXT("MonsterDetailHPBar"));
		HPBar->SetPercent(1.0f);
		HPBar->SetFillColorAndOpacity(FLinearColor(0.74f, 0.04f, 0.025f, 1.0f));
		Place(Canvas, HPBar, FVector2D(RightLeft, Cursor), FVector2D(RightInner, 30.f), 20);
		Expose(Blueprint, HPBar);
		Cursor += 52.f;

		AddText(Blueprint, Canvas, TEXT("MonsterSkillHeading"),
			NSLOCTEXT("MarchboundMonsterTab", "SkillHeading", "스킬"), 32,
			FVector2D(RightLeft, Cursor), FVector2D(RightInner, 48.0f), 20, Accent);
		Cursor += 56.f;

		// 스킬은 아이콘 한 줄. 이름은 아이콘이 없을 때만 런타임이 켠다.
		UTexture2D* SkillSlotFrame = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Common/Frames/T_KitA_Cell_Normal.T_KitA_Cell_Normal"));
		const float SkillRoom = RightInner / 4.f;
		const float SkillSide = FMath::Min(SkillRoom - 14.f, 168.f);
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FVector2D SlotPos(
				RightLeft + SkillRoom * Index + (SkillRoom - SkillSide) * 0.5f, Cursor);
			const FVector2D SlotSize(SkillSide, SkillSide);
			AddImage(Blueprint, Canvas, FName(*FString::Printf(
				TEXT("MonsterSkillSlot_%d"), Index)), SkillSlotFrame, SlotPos, SlotSize, 21, true);
			const FBox2D SlotInner = UIPartRects::Inner(TEXT("T_KitA_Cell_Normal"),
				SlotPos, SlotSize, false);
			AddImage(Blueprint, Canvas, FName(*FString::Printf(
				TEXT("MonsterSkillIcon_%d"), Index)), nullptr,
				SlotInner.Min, SlotInner.GetSize(), 22, true);
			AddText(Blueprint, Canvas, FName(*FString::Printf(
				TEXT("MonsterSkillName_%d"), Index)), FText::FromString(TEXT("")), 24,
				SlotInner.Min, SlotInner.GetSize(), 23, Ink);
		}
		Cursor += SkillSide + 28.f;

		AddText(Blueprint, Canvas, TEXT("MonsterStatusHeading"),
			NSLOCTEXT("MarchboundMonsterTab", "StatusHeading", "상태"), 32,
			FVector2D(RightLeft, Cursor), FVector2D(RightInner, 48.0f), 20, Accent);
		Cursor += 56.f;
		for (int32 Index = 0; Index < 2; ++Index)
		{
			AddText(Blueprint, Canvas, FName(*FString::Printf(
				TEXT("MonsterStatusText_%d"), Index)), FText::FromString(TEXT("")), 27,
				FVector2D(RightLeft + RightInner * 0.5f * Index, Cursor),
				FVector2D(RightInner * 0.5f, 52.0f), 20, Ink);
		}

		/*
		 * 트리를 통째로 다시 만드는 빌더라 삭제한 이름의 GUID는 걷고, 새로 만든
		 * 비노출 위젯에도 GUID를 채운다. 컴파일러는 bIsVariable 여부와 무관하게
		 * 모든 source widget의 GUID를 검증한다.
		 */
		{
			TSet<FName> Live;
			Blueprint->ForEachSourceWidget([Blueprint, &Live](UWidget* Widget)
			{
				if (Widget != nullptr)
				{
					const FName Name = Widget->GetFName();
					Live.Add(Name);
					if (!Blueprint->WidgetVariableNameToGuidMap.Contains(Name))
					{
						Blueprint->WidgetVariableNameToGuidMap.Add(
							Name, FGuid::NewDeterministicGuid(Widget->GetPathName()));
					}
				}
			});
			for (const UWidgetAnimation* Animation : Blueprint->Animations)
			{
				if (Animation != nullptr)
				{
					const FName Name = Animation->GetFName();
					Live.Add(Name);
					if (!Blueprint->WidgetVariableNameToGuidMap.Contains(Name))
					{
						Blueprint->WidgetVariableNameToGuidMap.Add(
							Name, FGuid::NewDeterministicGuid(Animation->GetPathName()));
					}
				}
			}
			TArray<FName> Stale;
			for (const TPair<FName, FGuid>& Entry : Blueprint->WidgetVariableNameToGuidMap)
			{
				if (Live.Contains(Entry.Key) == false)
				{
					Stale.Add(Entry.Key);
				}
			}
			for (const FName& Name : Stale)
			{
				Blueprint->OnVariableRemoved(Name);
			}
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		if (!UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename,
			FSavePackageArgs()))
		{
			UE_LOG(LogTemp, Error, TEXT("RD_MONSTER_TAB_BUILD save failed"));
			return;
		}

		UE_LOG(LogTemp, Display,
			TEXT("RD_MONSTER_TAB_BUILD success asset=%s rows=3 skills=4 design=1920x1080 layout=mercenary-shell-v3"),
			AssetPath);
	}
}

void RegisterMonsterTabWidgetBuilderCommands()
{
	using namespace MonsterTabWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildMonsterTab"),
		TEXT("Create the independent responsive Marchbound monster-tab WBP."),
		FConsoleCommandDelegate::CreateStatic(&Build));
}

void UnregisterMonsterTabWidgetBuilderCommands()
{
	MonsterTabWidgetBuilder::BuildCommand.Reset();
}
