#include "UI/MarchboundHireWidgetBuilder.h"

#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"

namespace MarchboundHireWidgetBuilder
{
	constexpr TCHAR AssetPath[] =
		TEXT("/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound.WBP_MercenaryHire_Marchbound");
	// Each screen region owns its own mobile-responsive scale box.
	const FVector2D DesignSize(1920.0f, 1080.0f);

	TUniquePtr<FAutoConsoleCommand> BuildCommand;

	template <typename T>
	T* FindOrCreate(UWidgetBlueprint* Blueprint, const FName Name)
	{
		if (UWidget* Existing = Blueprint->WidgetTree->FindWidget(Name))
		{
			T* Typed = Cast<T>(Existing);
			checkf(Typed != nullptr, TEXT("%s is not %s"),
				*Name.ToString(), *T::StaticClass()->GetName());
			return Typed;
		}
		T* NewWidget = Blueprint->WidgetTree->ConstructWidget<T>(T::StaticClass(), Name);
		Blueprint->OnVariableAdded(Name);
		return NewWidget;
	}

	void EnsureParent(UPanelWidget* Parent, UWidget* Child)
	{
		check(Parent != nullptr);
		check(Child != nullptr);
		if (Child->GetParent() == Parent)
		{
			return;
		}
		if (UPanelWidget* OldParent = Child->GetParent())
		{
			OldParent->RemoveChild(Child);
		}
		Parent->AddChild(Child);
	}

	void PlaceCanvas(UPanelWidget* Parent, UWidget* Child,
		const FVector2D Position, const FVector2D Size, const int32 ZOrder)
	{
		EnsureParent(Parent, Child);
		UCanvasPanelSlot* Slot = CastChecked<UCanvasPanelSlot>(Child->Slot);
		Slot->SetAnchors(FAnchors(0.0f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
	}

	void StretchCanvas(UCanvasPanel* Parent, UWidget* Child,
		const FAnchors Anchors, const int32 ZOrder)
	{
		EnsureParent(Parent, Child);
		UCanvasPanelSlot* Slot = CastChecked<UCanvasPanelSlot>(Child->Slot);
		Slot->SetAnchors(Anchors);
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetOffsets(FMargin(0.0f));
		Slot->SetZOrder(ZOrder);
	}

	UTexture2D* Texture(const TCHAR* Path)
	{
		UTexture2D* Result = LoadObject<UTexture2D>(nullptr, Path);
		checkf(Result != nullptr, TEXT("Missing Marchbound UI texture: %s"), Path);
		return Result;
	}

	void SetImage(UImage* Image, UTexture2D* Source)
	{
		Image->SetBrushFromTexture(Source, false);
		Image->SetColorAndOpacity(FLinearColor::White);
		Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void SetFont(UTextBlock* Text, const FSlateFontInfo& Template, const int32 Size)
	{
		FSlateFontInfo Font = Template;
		Font.Size = Size;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.12f, 0.065f, 0.025f, 1.0f)));
		Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Text->SetShadowColorAndOpacity(FLinearColor(1.0f, 0.86f, 0.60f, 0.35f));
		Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void SetLightFont(UTextBlock* Text, const FSlateFontInfo& Template, const int32 Size)
	{
		FSlateFontInfo Font = Template;
		Font.Size = Size;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.91f, 0.73f, 1.0f)));
		Text->SetShadowOffset(FVector2D(2.0f, 2.0f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.03f, 0.012f, 0.004f, 0.95f));
		Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void SetTransparentButton(UButton* Button)
	{
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
		Button->SetVisibility(ESlateVisibility::Visible);
	}

	UImage* AddImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, UTexture2D* Source, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder)
	{
		UImage* Result = FindOrCreate<UImage>(Blueprint, Name);
		PlaceCanvas(Parent, Result, Position, Size, ZOrder);
		SetImage(Result, Source);
		return Result;
	}

	UTextBlock* AddText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, const FText& Value, const FSlateFontInfo& Font,
		const int32 FontSize, const FVector2D Position, const FVector2D Size,
		const int32 ZOrder, const ETextJustify::Type Justify = ETextJustify::Center)
	{
		UTextBlock* Result = FindOrCreate<UTextBlock>(Blueprint, Name);
		PlaceCanvas(Parent, Result, Position, Size, ZOrder);
		Result->SetText(Value);
		Result->SetJustification(Justify);
		SetFont(Result, Font, FontSize);
		return Result;
	}

	void Build()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_MB_HIRE_BUILD missing %s"), AssetPath);
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		UCanvasPanel* Root = Cast<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("RootCanvas")));
		if (Root == nullptr)
		{
			Root = Cast<UCanvasPanel>(Blueprint->WidgetTree->RootWidget);
		}
		if (Root == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_MB_HIRE_BUILD RootCanvas is not a CanvasPanel"));
			return;
		}

		// 배경은 모든 화면비를 채우며 중앙 크롭한다. UI는 한 장짜리 1920x1080
		// 프레임으로 고정하지 않고 좌/중/우 영역이 각자 화면 폭에 맞춰 줄어든다.
		UOverlay* ViewportRoot = FindOrCreate<UOverlay>(Blueprint, TEXT("HireViewportRoot"));
		if (UWidget* LegacyUIScale = Blueprint->WidgetTree->FindWidget(TEXT("HireUIScale")))
		{
			if (UPanelWidget* Parent = LegacyUIScale->GetParent())
			{
				Parent->RemoveChild(LegacyUIScale);
			}
			LegacyUIScale->SetVisibility(ESlateVisibility::Collapsed);
		}
		EnsureParent(ViewportRoot, Root);
		Blueprint->WidgetTree->RootWidget = ViewportRoot;

		UTextBlock* FontSource = FindOrCreate<UTextBlock>(Blueprint, TEXT("HireName_0"));
		const FSlateFontInfo Font = FontSource->GetFont();

		UTexture2D* KnightBackground = Texture(
			TEXT("/Game/UI/Art/Marchbound/Mercenaries/T_MB_HireHero_Knight.T_MB_HireHero_Knight"));
		UTexture2D* ListFrame = Texture(
			TEXT("/Game/UI/Art/Marchbound/Hire/T_MB_HireListFrame.T_MB_HireListFrame"));
		UTexture2D* RowNormal = Texture(
			TEXT("/Game/UI/Art/Marchbound/Hire/T_MB_HireRowNormal.T_MB_HireRowNormal"));
		UTexture2D* RowSelected = Texture(
			TEXT("/Game/UI/Art/Marchbound/Hire/T_MB_HireRowSelected.T_MB_HireRowSelected"));
		UTexture2D* BackPlate = Texture(
			TEXT("/Game/UI/Art/Marchbound/Hire/T_MB_HireBackButton.T_MB_HireBackButton"));
		UTexture2D* TitlePlate = Texture(
			TEXT("/Game/UI/Art/Marchbound/Hire/T_MB_HireTitlePlate.T_MB_HireTitlePlate"));
		UTexture2D* PartyFrame = Texture(
			TEXT("/Game/UI/Art/Marchbound/Hire/T_MB_HirePartyFrame.T_MB_HirePartyFrame"));
		UTexture2D* PartyPlus = Texture(
			TEXT("/Game/UI/Art/Marchbound/Hire/T_MB_HirePartyRowPlus.T_MB_HirePartyRowPlus"));
		UTexture2D* PartyEmpty = Texture(
			TEXT("/Game/UI/Art/Marchbound/Hire/T_MB_HirePartyRowEmpty.T_MB_HirePartyRowEmpty"));
		UTexture2D* DepartPlate = Texture(
			TEXT("/Game/UI/Art/Marchbound/Hire/T_MB_HireDepartButton.T_MB_HireDepartButton"));
		UTexture2D* NamePlate = Texture(
			TEXT("/Game/UI/Art/Marchbound/Hire/T_MB_HireNamePlate.T_MB_HireNamePlate"));
		UTexture2D* StatsStrip = Texture(
			TEXT("/Game/UI/Art/Marchbound/Hire/T_MB_HireStatsStrip.T_MB_HireStatsStrip"));
		UTexture2D* SkillFrame = Texture(
			TEXT("/Game/UI/Art/Marchbound/Hire/T_MB_HireSkillButtonFrame.T_MB_HireSkillButtonFrame"));

		UCanvasPanel* OldBackdrop = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("Backdrop"));
		OldBackdrop->SetVisibility(ESlateVisibility::Collapsed);
		if (UWidget* LegacyBoard = Blueprint->WidgetTree->FindWidget(TEXT("Board")))
		{
			LegacyBoard->SetVisibility(ESlateVisibility::Collapsed);
		}
		UScaleBox* BackgroundScale = FindOrCreate<UScaleBox>(Blueprint, TEXT("HireBackgroundScale"));
		BackgroundScale->SetStretch(EStretch::ScaleToFill);
		BackgroundScale->SetStretchDirection(EStretchDirection::Both);
		BackgroundScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		UImage* BackgroundArt = FindOrCreate<UImage>(Blueprint, TEXT("Backdrop_Art"));
		EnsureParent(BackgroundScale, BackgroundArt);
		SetImage(BackgroundArt, KnightBackground);

		// Overlay의 자식 순서가 곧 그리기 순서다. 배경을 먼저, 반응형 UI 캔버스를
		// 나중에 다시 넣어 재빌드해도 항상 같은 계층과 Z 순서를 보장한다.
		if (UPanelWidget* Parent = BackgroundScale->GetParent())
		{
			Parent->RemoveChild(BackgroundScale);
		}
		if (UPanelWidget* Parent = Root->GetParent())
		{
			Parent->RemoveChild(Root);
		}
		ViewportRoot->AddChild(BackgroundScale);
		if (UOverlaySlot* Slot = Cast<UOverlaySlot>(BackgroundScale->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
		ViewportRoot->AddChild(Root);
		if (UOverlaySlot* Slot = Cast<UOverlaySlot>(Root->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}

		// 각 영역은 자기 디자인 크기를 유지하면서 할당된 화면 비율 안에서만
		// 축소/확대된다. 16:9, 20:9, 정사각형 창에서도 서로를 밀어내지 않는다.
		auto MakeRegion = [Blueprint, Root](const FName ScaleName,
			const FName SizeName, const FName CanvasName, const FVector2D RegionDesignSize,
			const FAnchors RegionAnchors, const int32 ZOrder)
		{
			UScaleBox* Scale = FindOrCreate<UScaleBox>(Blueprint, ScaleName);
			Scale->SetStretch(EStretch::ScaleToFit);
			Scale->SetStretchDirection(EStretchDirection::Both);
			USizeBox* Size = FindOrCreate<USizeBox>(Blueprint, SizeName);
			Size->SetWidthOverride(RegionDesignSize.X);
			Size->SetHeightOverride(RegionDesignSize.Y);
			UCanvasPanel* Canvas = FindOrCreate<UCanvasPanel>(Blueprint, CanvasName);
			EnsureParent(Size, Canvas);
			EnsureParent(Scale, Size);
			StretchCanvas(Root, Scale, RegionAnchors, ZOrder);
			return Canvas;
		};

		UCanvasPanel* LeftRegion = MakeRegion(TEXT("HireLeftScale"), TEXT("HireLeftSize"),
			TEXT("HireLeftRegion"), FVector2D(555.0f, 1080.0f),
			FAnchors(0.0f, 0.0f, 0.30f, 1.0f), 10);
		UCanvasPanel* CenterRegion = MakeRegion(TEXT("HireCenterScale"), TEXT("HireCenterSize"),
			TEXT("HireCenterRegion"), FVector2D(845.0f, 1080.0f),
			FAnchors(0.27f, 0.0f, 0.73f, 1.0f), 20);
		UCanvasPanel* RightRegion = MakeRegion(TEXT("HireRightScale"), TEXT("HireRightSize"),
			TEXT("HireRightRegion"), FVector2D(520.0f, 1080.0f),
			FAnchors(0.70f, 0.0f, 1.0f, 1.0f), 10);

		AddImage(Blueprint, LeftRegion, TEXT("HireListFrameArt"), ListFrame,
			FVector2D(55.0f, 100.0f), FVector2D(500.0f, 850.0f), 0);

		UCanvasPanel* TitlePanel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("HireTitlePanel"));
		PlaceCanvas(CenterRegion, TitlePanel, FVector2D(190.0f, 18.0f), FVector2D(430.0f, 106.0f), 30);
		AddImage(Blueprint, TitlePanel, TEXT("HireTitleArt"), TitlePlate,
			FVector2D::ZeroVector, FVector2D(430.0f, 106.0f), 0);
		UTextBlock* TitleText = AddText(Blueprint, TitlePanel, TEXT("HireTitleText"),
			NSLOCTEXT("MarchboundHire", "Title", "용병 선택"), Font, 42,
			FVector2D(30.0f, 20.0f), FVector2D(370.0f, 64.0f), 10);
		SetLightFont(TitleText, Font, 42);

		const FVector2D CardSize(420.0f, 116.0f);
		const TCHAR* DefaultNames[6] = {
			TEXT("기사"), TEXT("마법사"), TEXT("레인저"),
			TEXT("도적"), TEXT("야만전사"), TEXT("드루이드")
		};
		const TCHAR* DefaultRoles[6] = {
			TEXT("근접"), TEXT("마법"), TEXT("원거리"),
			TEXT("근접"), TEXT("근접"), TEXT("지원")
		};
		const TCHAR* PortraitPaths[6] = {
			TEXT("/Game/UI/Art/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"),
			TEXT("/Game/UI/Art/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage"),
			TEXT("/Game/UI/Art/Marchbound/Mercenaries/T_MB_HireIcon_Ranger.T_MB_HireIcon_Ranger"),
			TEXT("/Game/UI/Art/Marchbound/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue"),
			TEXT("/Game/UI/Art/Marchbound/Mercenaries/T_MB_HireIcon_Barbarian.T_MB_HireIcon_Barbarian"),
			TEXT("/Game/UI/Art/Marchbound/Mercenaries/T_MB_HireIcon_Druid.T_MB_HireIcon_Druid")
		};
		for (int32 Index = 0; Index < 6; ++Index)
		{
			UCanvasPanel* Card = FindOrCreate<UCanvasPanel>(Blueprint,
				FName(*FString::Printf(TEXT("HireCard_%d"), Index)));
			PlaceCanvas(LeftRegion, Card, FVector2D(95.0f, 158.0f + 124.0f * Index), CardSize, 10);
			Card->SetClipping(EWidgetClipping::ClipToBoundsAlways);

			UImage* Art = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("HireCard_%d_Art"), Index)));
			PlaceCanvas(Card, Art, FVector2D::ZeroVector, CardSize, 0);
			SetImage(Art, RowNormal);

			UImage* Selected = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("HireSelected_%d"), Index)));
			PlaceCanvas(Card, Selected, FVector2D::ZeroVector, CardSize, 5);
			SetImage(Selected, RowSelected);
			Selected->SetVisibility(Index == 0
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);

			UImage* Portrait = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("HirePortrait_%d"), Index)));
			PlaceCanvas(Card, Portrait, FVector2D(24.0f, 11.0f), FVector2D(88.0f, 94.0f), 12);
			SetImage(Portrait, Texture(PortraitPaths[Index]));
			Portrait->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UTextBlock* Name = FindOrCreate<UTextBlock>(Blueprint,
				FName(*FString::Printf(TEXT("HireName_%d"), Index)));
			PlaceCanvas(Card, Name, FVector2D(126.0f, 19.0f), FVector2D(230.0f, 44.0f), 15);
			Name->SetText(FText::FromString(DefaultNames[Index]));
			Name->SetJustification(ETextJustify::Left);
			SetFont(Name, Font, 29);

			UTextBlock* Role = FindOrCreate<UTextBlock>(Blueprint,
				FName(*FString::Printf(TEXT("HireRole_%d"), Index)));
			PlaceCanvas(Card, Role, FVector2D(126.0f, 62.0f), FVector2D(220.0f, 36.0f), 15);
			Role->SetText(FText::FromString(DefaultRoles[Index]));
			Role->SetJustification(ETextJustify::Left);
			SetFont(Role, Font, 20);

			for (const TCHAR* Prefix : {TEXT("HireHP"), TEXT("HireBadge"), TEXT("HireTrait")})
			{
				FindOrCreate<UTextBlock>(Blueprint,
					FName(*FString::Printf(TEXT("%s_%d"), Prefix, Index)))->SetVisibility(
						ESlateVisibility::Collapsed);
			}
			for (int32 SkillLine = 0; SkillLine < 2; ++SkillLine)
			{
				FindOrCreate<UTextBlock>(Blueprint,
					FName(*FString::Printf(TEXT("HireSkill_%d_%d"), Index, SkillLine)))->SetVisibility(
						ESlateVisibility::Collapsed);
			}

			UImage* Seal = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("HireSeal_%d"), Index)));
			PlaceCanvas(Card, Seal, FVector2D(350.0f, 24.0f), FVector2D(58.0f, 68.0f), 20);
			Seal->SetVisibility(ESlateVisibility::Collapsed);

			UButton* Button = FindOrCreate<UButton>(Blueprint,
				FName(*FString::Printf(TEXT("HireButton_%d"), Index)));
			PlaceCanvas(Card, Button, FVector2D::ZeroVector, CardSize, 40);
			SetTransparentButton(Button);
		}

		UCanvasPanel* NamePanel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("HireDetailNamePanel"));
		PlaceCanvas(CenterRegion, NamePanel, FVector2D(180.0f, 628.0f), FVector2D(520.0f, 120.0f), 20);
		AddImage(Blueprint, NamePanel, TEXT("HireDetailNameArt"), NamePlate,
			FVector2D::ZeroVector, FVector2D(520.0f, 120.0f), 0);
		AddText(Blueprint, NamePanel, TEXT("HireDetailName"), NSLOCTEXT("MarchboundHire", "Knight", "기사"),
			Font, 38, FVector2D(45.0f, 30.0f), FVector2D(430.0f, 60.0f), 10);

		UCanvasPanel* StatsPanel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("HireDetailStatsPanel"));
		PlaceCanvas(CenterRegion, StatsPanel, FVector2D(95.0f, 746.0f), FVector2D(690.0f, 96.0f), 20);
		AddImage(Blueprint, StatsPanel, TEXT("HireDetailStatsArt"), StatsStrip,
			FVector2D::ZeroVector, FVector2D(690.0f, 96.0f), 0);
		AddText(Blueprint, StatsPanel, TEXT("HireDetailHP"), FText::FromString(TEXT("HP 100")), Font, 28,
			FVector2D(24.0f, 22.0f), FVector2D(205.0f, 54.0f), 10);
		AddText(Blueprint, StatsPanel, TEXT("HireDetailAP"), FText::FromString(TEXT("AP 7")), Font, 28,
			FVector2D(242.0f, 22.0f), FVector2D(205.0f, 54.0f), 10);
		AddText(Blueprint, StatsPanel, TEXT("HireDetailSpeed"), NSLOCTEXT("MarchboundHire", "SpeedDefault", "속도 3"), Font, 28,
			FVector2D(460.0f, 22.0f), FVector2D(205.0f, 54.0f), 10);

		const TCHAR* DefaultSkillLabels[6] = {
			TEXT("평타"), TEXT("이동"), TEXT("스킬 1"),
			TEXT("스킬 2"), TEXT("스킬 3"), TEXT("스킬 4")
		};
		for (int32 Index = 0; Index < 6; ++Index)
		{
			UCanvasPanel* SkillPanel = FindOrCreate<UCanvasPanel>(Blueprint,
				FName(*FString::Printf(TEXT("HireDetailSkill_%d"), Index)));
			PlaceCanvas(CenterRegion, SkillPanel, FVector2D(53.0f + 126.0f * Index, 858.0f),
				FVector2D(116.0f, 116.0f), 20);
			AddImage(Blueprint, SkillPanel,
				FName(*FString::Printf(TEXT("HireDetailSkillArt_%d"), Index)), SkillFrame,
				FVector2D::ZeroVector, FVector2D(116.0f, 116.0f), 0);
			UTextBlock* SkillText = AddText(Blueprint, SkillPanel,
				FName(*FString::Printf(TEXT("HireDetailSkillText_%d"), Index)),
				FText::FromString(DefaultSkillLabels[Index]), Font, 18,
				FVector2D(8.0f, 37.0f), FVector2D(100.0f, 42.0f), 10);
			SetLightFont(SkillText, Font, 18);
			UButton* SkillButton = FindOrCreate<UButton>(Blueprint,
				FName(*FString::Printf(TEXT("HireDetailSkillButton_%d"), Index)));
			PlaceCanvas(SkillPanel, SkillButton, FVector2D::ZeroVector,
				FVector2D(116.0f, 116.0f), 30);
			SetTransparentButton(SkillButton);
		}

		UCanvasPanel* PartyPanel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("HireBottomBar"));
		PlaceCanvas(RightRegion, PartyPanel, FVector2D(40.0f, 145.0f), FVector2D(420.0f, 660.0f), 20);
		UImage* PartyFrameArt = FindOrCreate<UImage>(Blueprint, TEXT("HireBottomBar_Art"));
		PlaceCanvas(PartyPanel, PartyFrameArt, FVector2D::ZeroVector, FVector2D(420.0f, 660.0f), 0);
		SetImage(PartyFrameArt, PartyFrame);

		UTextBlock* PartyCount = FindOrCreate<UTextBlock>(Blueprint, TEXT("PartyCountText"));
		PlaceCanvas(PartyPanel, PartyCount, FVector2D(50.0f, 28.0f), FVector2D(320.0f, 54.0f), 15);
		PartyCount->SetText(NSLOCTEXT("MarchboundHire", "PartyDefault", "파티 0/3"));
		PartyCount->SetJustification(ETextJustify::Center);
		SetLightFont(PartyCount, Font, 32);

		for (int32 Index = 0; Index < 3; ++Index)
		{
			UCanvasPanel* SlotPanel = FindOrCreate<UCanvasPanel>(Blueprint,
				FName(*FString::Printf(TEXT("PartySlot_%d"), Index)));
			PlaceCanvas(PartyPanel, SlotPanel, FVector2D(40.0f, 112.0f + 150.0f * Index),
				FVector2D(340.0f, 140.0f), 10);

			AddImage(Blueprint, SlotPanel,
				FName(*FString::Printf(TEXT("PartySlotArt_%d"), Index)), PartyEmpty,
				FVector2D::ZeroVector, FVector2D(340.0f, 140.0f), 0);
			AddImage(Blueprint, SlotPanel,
				FName(*FString::Printf(TEXT("PartySlotPlus_%d"), Index)), PartyPlus,
				FVector2D::ZeroVector, FVector2D(340.0f, 140.0f), 2);

			UImage* Face = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("PartySlotFace_%d"), Index)));
			PlaceCanvas(SlotPanel, Face, FVector2D(22.0f, 17.0f), FVector2D(106.0f, 106.0f), 10);
			Face->SetVisibility(ESlateVisibility::Collapsed);

			UTextBlock* Name = FindOrCreate<UTextBlock>(Blueprint,
				FName(*FString::Printf(TEXT("PartySlotName_%d"), Index)));
			PlaceCanvas(SlotPanel, Name, FVector2D(140.0f, 42.0f), FVector2D(160.0f, 54.0f), 12);
			Name->SetJustification(ETextJustify::Center);
			SetFont(Name, Font, 26);
			Name->SetVisibility(ESlateVisibility::Collapsed);

			UButton* SlotButton = FindOrCreate<UButton>(Blueprint,
				FName(*FString::Printf(TEXT("PartySlotButton_%d"), Index)));
			PlaceCanvas(SlotPanel, SlotButton, FVector2D::ZeroVector,
				FVector2D(340.0f, 140.0f), 30);
			SetTransparentButton(SlotButton);
		}

		UTextBlock* Notice = FindOrCreate<UTextBlock>(Blueprint, TEXT("NoticeText"));
		PlaceCanvas(PartyPanel, Notice, FVector2D(28.0f, 562.0f), FVector2D(364.0f, 58.0f), 15);
		Notice->SetText(NSLOCTEXT("MarchboundHire", "NoticeDefault", "1명 이상 선택하면 출발 가능"));
		Notice->SetJustification(ETextJustify::Center);
		SetLightFont(Notice, Font, 20);

		UCanvasPanel* Depart = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("DepartHolder"));
		PlaceCanvas(RightRegion, Depart, FVector2D(80.0f, 824.0f), FVector2D(340.0f, 160.0f), 30);
		AddImage(Blueprint, Depart, TEXT("DepartArt"), DepartPlate,
			FVector2D::ZeroVector, FVector2D(340.0f, 160.0f), 0);
		UTextBlock* DepartLabel = FindOrCreate<UTextBlock>(Blueprint, TEXT("DepartLabel"));
		PlaceCanvas(Depart, DepartLabel, FVector2D(30.0f, 43.0f), FVector2D(280.0f, 72.0f), 15);
		DepartLabel->SetText(NSLOCTEXT("MarchboundHire", "Depart", "출발"));
		DepartLabel->SetJustification(ETextJustify::Center);
		SetLightFont(DepartLabel, Font, 44);
		UButton* DepartButton = FindOrCreate<UButton>(Blueprint, TEXT("DepartButton"));
		PlaceCanvas(Depart, DepartButton, FVector2D::ZeroVector, FVector2D(340.0f, 160.0f), 30);
		SetTransparentButton(DepartButton);

		UCanvasPanel* Back = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("HireBackHolder"));
		PlaceCanvas(LeftRegion, Back, FVector2D(70.0f, 952.0f), FVector2D(270.0f, 106.0f), 30);
		AddImage(Blueprint, Back, TEXT("HireBackArt"), BackPlate,
			FVector2D::ZeroVector, FVector2D(270.0f, 106.0f), 0);
		UTextBlock* BackLabel = AddText(Blueprint, Back, TEXT("HireBackLabel"),
			NSLOCTEXT("MarchboundHire", "Back", "뒤로"), Font, 32,
			FVector2D(25.0f, 24.0f), FVector2D(220.0f, 58.0f), 15);
		SetLightFont(BackLabel, Font, 32);
		UButton* BackButton = FindOrCreate<UButton>(Blueprint, TEXT("HireBackButton"));
		PlaceCanvas(Back, BackButton, FVector2D::ZeroVector, FVector2D(270.0f, 106.0f), 30);
		SetTransparentButton(BackButton);

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (!UPackage::SavePackage(Blueprint->GetPackage(), Blueprint,
			*FPackageName::LongPackageNameToFilename(
				Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()),
			FSavePackageArgs()))
		{
			UE_LOG(LogTemp, Error, TEXT("RD_MB_HIRE_BUILD save failed"));
			return;
		}

		UE_LOG(LogTemp, Display,
			TEXT("RD_MB_HIRE_BUILD success asset=%s cards=6 party_slots=3 skills=6 design=1920x1080"),
			AssetPath);
	}
}

void RegisterMarchboundHireWidgetBuilderCommands()
{
	using namespace MarchboundHireWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildMercenaryHire"),
		TEXT("Rebuild WBP_MercenaryHire_Marchbound with the Marchbound split UI parts."),
		FConsoleCommandDelegate::CreateStatic(&Build));
}

void UnregisterMarchboundHireWidgetBuilderCommands()
{
	MarchboundHireWidgetBuilder::BuildCommand.Reset();
}
