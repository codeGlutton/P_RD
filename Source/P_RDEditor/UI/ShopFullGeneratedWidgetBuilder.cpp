
#include "UI/ShopFullGeneratedWidgetBuilder.h"

#include "AssetToolsModule.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Brushes/SlateColorBrush.h"
#include "UI/UIFont.h"
#include "UI/Shop/ShopFullGeneratedWidgetBase.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditorUtils.h"
#include "WidgetBlueprintFactory.h"

namespace ShopFullGeneratedWidgetBuilder
{
	constexpr TCHAR PackagePath[] = TEXT("/Game/UI/Shop");
	constexpr TCHAR AssetName[] = TEXT("WBP_Shop_FullGenerated");
	constexpr TCHAR AssetPath[] = TEXT("/Game/UI/Shop/WBP_Shop_FullGenerated.WBP_Shop_FullGenerated");
	constexpr float DesignWidth = 1600.f;
	constexpr float DesignHeight = 1000.f;

	TUniquePtr<FAutoConsoleCommand> BuildCommand;

	UTexture2D* Texture(const TCHAR* Path)
	{
		UTexture2D* Result = LoadObject<UTexture2D>(nullptr, Path);
		checkf(Result != nullptr, TEXT("Missing shop UI texture: %s"), Path);
		return Result;
	}

	FSlateBrush TextureBrush(UTexture2D* Source, const bool bNineSlice = false,
		const FLinearColor& Tint = FLinearColor::White)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Source);
		Brush.DrawAs = bNineSlice ? ESlateBrushDrawType::Box : ESlateBrushDrawType::Image;
		Brush.TintColor = FSlateColor(Tint);
		if (bNineSlice)
		{
			Brush.Margin = FMargin(.16f);
		}
		if (Source != nullptr)
		{
			const FIntPoint Size = Source->GetImportedSize();
			Brush.ImageSize = FVector2D(Size.X, Size.Y);
		}
		return Brush;
	}

	void FillOverlay(UOverlay* Parent, UWidget* Child)
	{
		UOverlaySlot* Slot = Parent->AddChildToOverlay(Child);
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
		Slot->SetPadding(FMargin(0.f));
	}

	void Place(UCanvasPanel* Parent, UWidget* Child, const FVector2D& Position,
		const FVector2D& Size, const int32 ZOrder)
	{
		UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Child);
		Slot->SetAnchors(FAnchors(0.f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
	}

	void Anchor(UCanvasPanel* Parent, UWidget* Child, const FAnchors& Anchors,
		const FMargin& Offsets, const int32 ZOrder)
	{
		UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Child);
		Slot->SetAnchors(Anchors);
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetOffsets(Offsets);
		Slot->SetZOrder(ZOrder);
	}

	void StyleText(UTextBlock* Text, const int32 Size,
		const FLinearColor& Color = FLinearColor(1.f, .91f, .73f, 1.f),
		const ETextJustify::Type Justification = ETextJustify::Center)
	{
		FSlateFontInfo Font = UIFont::MakeSettingsExact(Text->GetFont(), Size);
		Font.OutlineSettings.OutlineSize = Size >= 30 ? 2 : 1;
		Font.OutlineSettings.OutlineColor = FLinearColor(.025f, .012f, .004f, .95f);
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(2.f, 2.f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .85f));
		Text->SetJustification(Justification);
		Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void ExposeWithGuid(UWidgetBlueprint* Blueprint, UWidget* Widget)
	{
		if (Blueprint == nullptr || Widget == nullptr)
		{
			return;
		}
		const FName WidgetName = Widget->GetFName();
		if (!Blueprint->WidgetVariableNameToGuidMap.Contains(WidgetName))
		{
			Blueprint->OnVariableAdded(WidgetName);
		}
	}

	FButtonStyle TransparentButtonStyle()
	{
		FSlateBrush Empty;
		Empty.DrawAs = ESlateBrushDrawType::NoDrawType;
		FButtonStyle Style;
		Style.SetNormal(Empty);
		Style.SetHovered(Empty);
		Style.SetPressed(Empty);
		Style.SetDisabled(Empty);
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f));
		return Style;
	}

	UImage* AddImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent, const FName Name,
		UTexture2D* Source, const FVector2D& Position, const FVector2D& Size,
		const int32 ZOrder, const bool bNineSlice = false,
		const FLinearColor& Tint = FLinearColor::White)
	{
		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), Name);
		Image->SetBrush(TextureBrush(Source, bNineSlice, Tint));
		Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Image, Position, Size, ZOrder);
		return Image;
	}

	UImage* AddSolidImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, const FLinearColor& Color, const FVector2D& Position,
		const FVector2D& Size, const int32 ZOrder)
	{
		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), Name);
		Image->SetBrush(FSlateColorBrush(Color));
		Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Image, Position, Size, ZOrder);
		return Image;
	}

	UTextBlock* AddText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, const FText& Value, const int32 FontSize,
		const FVector2D& Position, const FVector2D& Size, const int32 ZOrder,
		const FLinearColor& Color = FLinearColor(1.f, .91f, .73f, 1.f),
		const ETextJustify::Type Justification = ETextJustify::Center)
	{
		UOverlay* Center = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), FName(*(Name.ToString() + TEXT("_Center"))));
		Place(Parent, Center, Position, Size, ZOrder);

		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), Name);
		Text->SetText(Value);
		StyleText(Text, FontSize, Color, Justification);
		Text->SetMargin(FMargin(0.f));
		Text->SetRenderTransform(FWidgetTransform());
		Text->SetRenderTransformPivot(FVector2D(.5f, .5f));
		UOverlaySlot* TextSlot = Center->AddChildToOverlay(Text);
		TextSlot->SetPadding(FMargin(0.f));
		TextSlot->SetHorizontalAlignment(HAlign_Fill);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		return Text;
	}

	UButton* AddButton(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, const FVector2D& Position, const FVector2D& Size,
		const int32 ZOrder)
	{
		UButton* Button = Blueprint->WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), Name);
		Button->SetStyle(TransparentButtonStyle());
		Button->SetBackgroundColor(FLinearColor::Transparent);
		Button->SetColorAndOpacity(FLinearColor::White);
		Button->SetClickMethod(EButtonClickMethod::MouseDown);
		Place(Parent, Button, Position, Size, ZOrder);
		return Button;
	}

	void AddLabeledButton(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName HolderName, const FName ArtName, const FName TextName,
		const FName ButtonName, UTexture2D* Art, const FText& Label,
		const FVector2D& Position, const FVector2D& Size, const int32 ZOrder,
		const int32 FontSize = 30)
	{
		UCanvasPanel* Holder = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), HolderName);
		Place(Parent, Holder, Position, Size, ZOrder);
		AddImage(Blueprint, Holder, ArtName, Art, FVector2D::ZeroVector, Size, 0, true);
		AddText(Blueprint, Holder, TextName, Label, FontSize,
			FVector2D(Size.X * .09f, Size.Y * .12f),
			FVector2D(Size.X * .82f, Size.Y * .68f), 2);
		AddButton(Blueprint, Holder, ButtonName, FVector2D::ZeroVector, Size, 5);
	}

	UWidgetBlueprint* EnsureBlueprint()
	{
		if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(nullptr, AssetPath))
		{
			return Existing;
		}

		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->ParentClass = UShopFullGeneratedWidgetBase::StaticClass();
		FAssetToolsModule& AssetTools =
			FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		return Cast<UWidgetBlueprint>(AssetTools.Get().CreateAsset(
			AssetName, PackagePath, UWidgetBlueprint::StaticClass(), Factory));
	}

	void ResetTree(UWidgetBlueprint* Blueprint)
	{
		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		if (UWidget* PreviousRoot = Blueprint->WidgetTree->RootWidget)
		{
			// DeleteWidgets compiles once as part of the deletion. Temporarily remove
			// the BindWidget parent so the intentionally empty intermediate tree is valid.
			Blueprint->ParentClass = UUserWidget::StaticClass();
			TSet<UWidget*> Widgets;
			Widgets.Add(PreviousRoot);
			FWidgetBlueprintEditorUtils::DeleteWidgets(Blueprint, MoveTemp(Widgets),
				FWidgetBlueprintEditorUtils::EDeleteWidgetWarningType::DeleteSilently);
		}
		Blueprint->ParentClass = UShopFullGeneratedWidgetBase::StaticClass();
	}

	void Build()
	{
		UE_LOG(LogTemp, Display, TEXT("RD_SHOP_FULL_GENERATED_BUILD begin"));

		// Resolve all hard dependencies before replacing the existing tree. A missing
		// generated background therefore cannot leave the alternate WBP half rebuilt.
		UTexture2D* ArtifactBackground = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Backgrounds/T_ShopFG_Artifact_Background.T_ShopFG_Artifact_Background"));
		UTexture2D* SkillBackground = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Backgrounds/T_ShopFG_Skill_Background.T_ShopFG_Skill_Background"));
		UTexture2D* RestBackground = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Backgrounds/T_ShopFG_Rest_Background.T_ShopFG_Rest_Background"));
		UTexture2D* TitlePlate = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_TitlePlate.T_ShopFG_TitlePlate"));
		UTexture2D* TabNormal = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_Tab_Normal.T_ShopFG_Tab_Normal"));
		UTexture2D* TabSelected = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_Tab_Selected.T_ShopFG_Tab_Selected"));
		UTexture2D* GoldPlateArt = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_GoldPlate.T_ShopFG_GoldPlate"));
		UTexture2D* RailCardNormal = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_RailCard_Normal.T_ShopFG_RailCard_Normal"));
		UTexture2D* RailCardSelected = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_RailCard_Selected.T_ShopFG_RailCard_Selected"));
		UTexture2D* UnitSlotNormal = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_UnitSlot_Normal.T_ShopFG_UnitSlot_Normal"));
		UTexture2D* UnitSlotSelected = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_UnitSlot_Selected.T_ShopFG_UnitSlot_Selected"));
		UTexture2D* SkillSlotNormal = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_SkillSlot_Normal.T_ShopFG_SkillSlot_Normal"));
		UTexture2D* SkillSlotSelected = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_SkillSlot_Selected.T_ShopFG_SkillSlot_Selected"));
		UTexture2D* RestUnitPanel = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_RestUnitPanel.T_ShopFG_RestUnitPanel"));
		UTexture2D* RestCostPlateArt = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_RestCostPlate.T_ShopFG_RestCostPlate"));
		UTexture2D* InventoryPanel = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_InventoryPanel.T_ShopFG_InventoryPanel"));
		UTexture2D* SelectionPointer = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_SelectionPointer.T_ShopFG_SelectionPointer"));
		UTexture2D* ButtonBack = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Controls/T_ShopFG_Button_Back.T_ShopFG_Button_Back"));
		UTexture2D* ButtonPrimary = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Controls/T_ShopFG_Button_Primary.T_ShopFG_Button_Primary"));
		UTexture2D* ButtonSecondary = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Controls/T_ShopFG_Button_Secondary.T_ShopFG_Button_Secondary"));
		UTexture2D* ArrowLeft = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Controls/T_ShopFG_Arrow_Left.T_ShopFG_Arrow_Left"));
		UTexture2D* ArrowRight = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Controls/T_ShopFG_Arrow_Right.T_ShopFG_Arrow_Right"));
		UTexture2D* MeterTrack = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Meters/T_ShopFG_MeterTrack.T_ShopFG_MeterTrack"));
		UTexture2D* MeterFill = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Meters/T_ShopFG_MeterFill.T_ShopFG_MeterFill"));

		UWidgetBlueprint* Blueprint = EnsureBlueprint();
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_SHOP_FULL_GENERATED_BUILD could not create %s"), AssetPath);
			return;
		}
		ResetTree(Blueprint);

		UOverlay* Root = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("ShopViewportRoot"));
		Blueprint->WidgetTree->RootWidget = Root;

		UBorder* Letterbox = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("ShopLetterbox"));
		Letterbox->SetBrushColor(FLinearColor(.018f, .024f, .032f, 1.f));
		Letterbox->SetPadding(FMargin(0.f));
		FillOverlay(Root, Letterbox);

		UScaleBox* MasterScale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("ShopMasterScale"));
		MasterScale->SetStretch(EStretch::ScaleToFit);
		MasterScale->SetStretchDirection(EStretchDirection::Both);
		MasterScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		FillOverlay(Root, MasterScale);

		USizeBox* DesignSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("ShopDesignSize"));
		DesignSize->SetWidthOverride(DesignWidth);
		DesignSize->SetHeightOverride(DesignHeight);
		MasterScale->AddChild(DesignSize);

		UCanvasPanel* Screen = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("ShopDesignCanvas"));
		DesignSize->SetContent(Screen);

		// Each mode owns its scene art and mode-only content. Runtime switches these
		// panels while the five-slot horizontal rail stays stable above both.
		UCanvasPanel* ArtifactPanel = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("mArtifactShopPanel"));
		Anchor(Screen, ArtifactPanel, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f), 0);
		AddImage(Blueprint, ArtifactPanel, TEXT("ArtifactShopBackground"), ArtifactBackground,
			FVector2D::ZeroVector, FVector2D(DesignWidth, DesignHeight), 0);

		UCanvasPanel* SkillPanel = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("mSkillShopPanel"));
		Anchor(Screen, SkillPanel, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f), 0);
		AddImage(Blueprint, SkillPanel, TEXT("SkillShopBackground"), SkillBackground,
			FVector2D::ZeroVector, FVector2D(DesignWidth, DesignHeight), 0);
		SkillPanel->SetVisibility(ESlateVisibility::Collapsed);

		UCanvasPanel* RestPanel = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("mRestShopPanel"));
		Anchor(Screen, RestPanel, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f), 0);
		AddImage(Blueprint, RestPanel, TEXT("RestShopBackground"), RestBackground,
			FVector2D::ZeroVector, FVector2D(DesignWidth, DesignHeight), 0);
		RestPanel->SetVisibility(ESlateVisibility::Collapsed);

		// Lower contrast under the interactive rail keeps item silhouettes readable
		// without baking any text or icons into the generated scene backgrounds.
		// Each mode owns its scrim so its own skill-slot controls can paint above it.
		auto AddRailScrim = [Blueprint](UCanvasPanel* ModePanel, const FName Name)
		{
			UBorder* RailScrim = Blueprint->WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(), Name);
			RailScrim->SetBrushColor(FLinearColor(.008f, .018f, .03f, .74f));
			RailScrim->SetPadding(FMargin(0.f));
			RailScrim->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Place(ModePanel, RailScrim, FVector2D(0.f, 438.f),
				FVector2D(1600.f, 562.f), 5);
		};
		AddRailScrim(ArtifactPanel, TEXT("ArtifactRailScrim"));
		AddRailScrim(SkillPanel, TEXT("SkillRailScrim"));

		UBorder* RestScrim = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("RestContentScrim"));
		RestScrim->SetBrushColor(FLinearColor(.008f, .018f, .03f, .72f));
		RestScrim->SetPadding(FMargin(0.f));
		RestScrim->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(RestPanel, RestScrim, FVector2D(125.f, 112.f),
			FVector2D(1350.f, 750.f), 3);

		// Header and category selectors.
		AddImage(Blueprint, Screen, TEXT("ShopTitlePlate"), TitlePlate,
			FVector2D(20.f, 14.f), FVector2D(356.f, 88.f), 10, true);
		AddText(Blueprint, Screen, TEXT("mTitleText"),
			NSLOCTEXT("ShopHorizontalRail", "Title", "상점"), 38,
			FVector2D(52.f, 27.f), FVector2D(292.f, 54.f), 12);

		AddLabeledButton(Blueprint, Screen, TEXT("ArtifactTabHolder"),
			TEXT("ArtifactTabPlate"), TEXT("mArtifactTabText"), TEXT("mArtifactTabButton"),
			TabSelected, NSLOCTEXT("ShopHorizontalRail", "Artifact", "아티팩트"),
			FVector2D(402.f, 21.f), FVector2D(242.f, 69.f), 12, 27);
		AddLabeledButton(Blueprint, Screen, TEXT("SkillTabHolder"),
			TEXT("SkillTabPlate"), TEXT("mSkillTabText"), TEXT("mSkillTabButton"),
			TabNormal, NSLOCTEXT("ShopHorizontalRail", "Skill", "스킬"),
			FVector2D(650.f, 21.f), FVector2D(242.f, 69.f), 12, 27);
		AddLabeledButton(Blueprint, Screen, TEXT("RestTabHolder"),
			TEXT("RestTabPlate"), TEXT("mRestTabText"), TEXT("mRestTabButton"),
			TabNormal, NSLOCTEXT("ShopHorizontalRail", "Rest", "휴식"),
			FVector2D(898.f, 21.f), FVector2D(242.f, 69.f), 12, 27);

		AddImage(Blueprint, Screen, TEXT("GoldPlate"), GoldPlateArt,
			FVector2D(1290.f, 24.f), FVector2D(286.f, 64.f), 10, true);
		AddText(Blueprint, Screen, TEXT("mGoldText"), FText::FromString(TEXT("0 G")), 26,
			FVector2D(1314.f, 28.f), FVector2D(238.f, 48.f), 12);
		AddLabeledButton(Blueprint, ArtifactPanel, TEXT("InventoryHolder"),
			TEXT("InventoryPlate"), TEXT("mInventoryButtonText"), TEXT("mInventoryButton"),
			ButtonSecondary, NSLOCTEXT("ShopHorizontalRail", "Inventory", "인벤토리"),
			FVector2D(1148.f, 24.f), FVector2D(136.f, 64.f), 42, 18);

		// Existing contract boxes remain real widgets with their exact legacy types.
		// The horizontal-rail runtime uses the fixed controls below; these boxes stay
		// collapsed as a compatibility sink for older data-population code.
		UHorizontalBox* ItemBox = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("mItemBox"));
		Place(Screen, ItemBox, FVector2D(100.f, 130.f), FVector2D(1400.f, 260.f), 7);
		ItemBox->SetVisibility(ESlateVisibility::Collapsed);

		UWrapBox* ArtifactItemBox = Blueprint->WidgetTree->ConstructWidget<UWrapBox>(
			UWrapBox::StaticClass(), TEXT("mArtifactItemBox"));
		Place(ArtifactPanel, ArtifactItemBox, FVector2D(100.f, 130.f),
			FVector2D(1400.f, 260.f), 4);
		ArtifactItemBox->SetVisibility(ESlateVisibility::Collapsed);

		UWrapBox* SkillItemBox = Blueprint->WidgetTree->ConstructWidget<UWrapBox>(
			UWrapBox::StaticClass(), TEXT("mSkillItemBox"));
		Place(SkillPanel, SkillItemBox, FVector2D(100.f, 130.f),
			FVector2D(1400.f, 260.f), 4);
		SkillItemBox->SetVisibility(ESlateVisibility::Collapsed);

		UWrapBox* OwnedUnitBox = Blueprint->WidgetTree->ConstructWidget<UWrapBox>(
			UWrapBox::StaticClass(), TEXT("mOwnedUnitBox"));
		Place(SkillPanel, OwnedUnitBox, FVector2D(1030.f, 108.f),
			FVector2D(530.f, 110.f), 4);
		OwnedUnitBox->SetVisibility(ESlateVisibility::Collapsed);

		// Skill target rail: select a party member before replacing one of four slots.
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const FVector2D Position(610.f + Index * 126.f, 108.f);
			const FVector2D Extent(112.f, 112.f);
			AddImage(Blueprint, SkillPanel,
				FName(*FString::Printf(TEXT("UnitSelectPlate_%d"), Index)),
				Index == 0 ? UnitSlotSelected : UnitSlotNormal, Position, Extent, 8, true);
			AddImage(Blueprint, SkillPanel,
				FName(*FString::Printf(TEXT("mUnitSelectIcon_%d"), Index)), nullptr,
				Position + FVector2D(13.f), Extent - FVector2D(26.f), 10);
			AddButton(Blueprint, SkillPanel,
				FName(*FString::Printf(TEXT("mUnitSelectButton_%d"), Index)),
				Position, Extent, 12);
		}

		// Rest mode is a three-row before/after ledger. Runtime owns the values and
		// transaction; the builder only supplies stable, named UImage/UTextBlock
		// targets matching that contract. Existing shop/mercenary chrome is reused.
		const int32 RestHPBefore[3] = { 42, 31, 58 };
		const int32 RestAPBefore[3] = { 6, 3, 8 };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const float RowY = 142.f + Index * 196.f;
			UCanvasPanel* RowHolder = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(),
				FName(*FString::Printf(TEXT("RestUnitRowHolder_%d"), Index)));
			RowHolder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Place(RestPanel, RowHolder, FVector2D::ZeroVector,
				FVector2D(DesignWidth, DesignHeight), 5);

			AddImage(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitPlate_%d"), Index)), RestUnitPanel,
				FVector2D(180.f, RowY), FVector2D(1240.f, 176.f), 6, true,
				FLinearColor(.54f, .66f, .78f, 1.f));
			AddImage(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitIcon_%d"), Index)), nullptr,
				FVector2D(210.f, RowY + 15.f), FVector2D(146.f, 146.f), 8);
			AddText(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitNameText_%d"), Index)),
				FText::Format(NSLOCTEXT("ShopHorizontalRail", "RestUnitLevel", "용병 {0}"),
					FText::AsNumber(Index + 1)), 22,
				FVector2D(372.f, RowY + 12.f), FVector2D(134.f, 38.f), 9,
				FLinearColor(.95f, .88f, .7f, 1.f));

			AddText(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitHPLabel_%d"), Index)), FText::FromString(TEXT("HP")), 20,
				FVector2D(382.f, RowY + 57.f), FVector2D(58.f, 40.f), 9,
				FLinearColor(1.f, .54f, .52f, 1.f));
			AddImage(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitHPBeforeTrack_%d"), Index)),
				MeterTrack, FVector2D(450.f, RowY + 64.f), FVector2D(310.f, 24.f), 8, true);
			AddImage(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitHPBeforeFill_%d"), Index)),
				MeterFill, FVector2D(454.f, RowY + 68.f), FVector2D(302.f, 16.f), 9,
				true, FLinearColor(.86f, .24f, .25f, 1.f));
			AddText(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitHPBeforeText_%d"), Index)),
				FText::FromString(FString::Printf(TEXT("%d/100"), RestHPBefore[Index])), 17,
				FVector2D(450.f, RowY + 56.f), FVector2D(310.f, 40.f), 10,
				FLinearColor::White);
			AddText(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitHPArrow_%d"), Index)), FText::FromString(TEXT("→")), 28,
				FVector2D(772.f, RowY + 55.f), FVector2D(62.f, 42.f), 10,
				FLinearColor(.85f, .88f, .9f, 1.f));
			AddImage(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitHPAfterTrack_%d"), Index)),
				MeterTrack, FVector2D(846.f, RowY + 64.f), FVector2D(420.f, 24.f), 8, true);
			AddImage(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitHPAfterFill_%d"), Index)),
				MeterFill, FVector2D(850.f, RowY + 68.f), FVector2D(412.f, 16.f), 9,
				true, FLinearColor(.86f, .24f, .25f, 1.f));
			AddText(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitHPAfterText_%d"), Index)), FText::FromString(TEXT("100/100")), 17,
				FVector2D(846.f, RowY + 56.f), FVector2D(420.f, 40.f), 10,
				FLinearColor::White);

			AddText(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitAPLabel_%d"), Index)), FText::FromString(TEXT("AP")), 20,
				FVector2D(382.f, RowY + 108.f), FVector2D(58.f, 40.f), 9,
				FLinearColor(.38f, .72f, 1.f, 1.f));
			AddImage(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitAPBeforeTrack_%d"), Index)),
				MeterTrack, FVector2D(450.f, RowY + 115.f), FVector2D(310.f, 24.f), 8, true);
			AddImage(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitAPBeforeFill_%d"), Index)),
				MeterFill, FVector2D(454.f, RowY + 119.f), FVector2D(302.f, 16.f), 9,
				true, FLinearColor(.16f, .56f, 1.f, 1.f));
			AddText(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitAPBeforeText_%d"), Index)),
				FText::FromString(FString::Printf(TEXT("%d/12"), RestAPBefore[Index])), 17,
				FVector2D(450.f, RowY + 107.f), FVector2D(310.f, 40.f), 10,
				FLinearColor::White);
			AddText(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitAPArrow_%d"), Index)), FText::FromString(TEXT("→")), 28,
				FVector2D(772.f, RowY + 106.f), FVector2D(62.f, 42.f), 10,
				FLinearColor(.85f, .88f, .9f, 1.f));
			AddImage(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitAPAfterTrack_%d"), Index)),
				MeterTrack, FVector2D(846.f, RowY + 115.f), FVector2D(420.f, 24.f), 8, true);
			AddImage(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitAPAfterFill_%d"), Index)),
				MeterFill, FVector2D(850.f, RowY + 119.f), FVector2D(412.f, 16.f), 9,
				true, FLinearColor(.16f, .56f, 1.f, 1.f));
			AddText(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitAPAfterText_%d"), Index)), FText::FromString(TEXT("12/12")), 17,
				FVector2D(846.f, RowY + 107.f), FVector2D(420.f, 40.f), 10,
				FLinearColor::White);
		}

		AddImage(Blueprint, RestPanel, TEXT("RestCostPlate"), RestCostPlateArt,
			FVector2D(616.f, 742.f), FVector2D(368.f, 82.f), 12, true);
		AddText(Blueprint, RestPanel, TEXT("RestCostLabel"),
			NSLOCTEXT("ShopHorizontalRail", "RestCost", "비용"), 21,
			FVector2D(646.f, 759.f), FVector2D(90.f, 46.f), 14);
		AddText(Blueprint, RestPanel, TEXT("RestCostText"), FText::FromString(TEXT("100 G")), 24,
			FVector2D(752.f, 757.f), FVector2D(202.f, 48.f), 14,
			FLinearColor(1.f, .82f, .35f, 1.f));
		AddLabeledButton(Blueprint, RestPanel, TEXT("RestButtonHolder"),
			TEXT("RestButtonPlate"), TEXT("mRestButtonText"), TEXT("mRestButton"),
			ButtonPrimary, NSLOCTEXT("ShopHorizontalRail", "RestCTA", "휴식하기"),
			FVector2D(1280.f, 890.f), FVector2D(292.f, 96.f), 40, 30);

		// Shared five-item horizontal rail. Slot 2 is the expanded selection; runtime
		// updates every icon/price and collapses unavailable slots.
		const FVector2D RailPositions[5] = {
			FVector2D(80.f, 520.f), FVector2D(310.f, 485.f),
			FVector2D(640.f, 365.f), FVector2D(1080.f, 485.f),
			FVector2D(1340.f, 520.f)
		};
		const FVector2D RailSizes[5] = {
			FVector2D(180.f, 225.f), FVector2D(210.f, 263.f),
			FVector2D(320.f, 400.f), FVector2D(210.f, 263.f),
			FVector2D(180.f, 225.f)
		};
		for (int32 Index = 0; Index < 5; ++Index)
		{
			const FVector2D Position = RailPositions[Index];
			const FVector2D Size = RailSizes[Index];
			AddImage(Blueprint, Screen,
				FName(*FString::Printf(TEXT("ShopRailPlate_%d"), Index)),
				Index == 2 ? RailCardSelected : RailCardNormal, Position, Size, 20, true);

			const float IconExtent = Index == 2 ? 164.f : (Index == 0 || Index == 4 ? 102.f : 124.f);
			const FVector2D IconPosition(
				Position.X + (Size.X - IconExtent) * .5f,
				Position.Y + (Index == 2 ? 34.f : 28.f));
			UImage* Icon = AddImage(Blueprint, Screen,
				FName(*FString::Printf(TEXT("ShopRailIcon_%d"), Index)), nullptr,
				IconPosition, FVector2D(IconExtent), 22);
			if (Index == 2)
			{
				Icon->SetVisibility(ESlateVisibility::Collapsed);
			}

			UTextBlock* Price = AddText(Blueprint, Screen,
				FName(*FString::Printf(TEXT("ShopRailPriceText_%d"), Index)),
				FText::Format(NSLOCTEXT("ShopHorizontalRail", "PreviewPrice", "{0} G"),
					FText::AsNumber(60 + Index * 10)), 22,
				FVector2D(Position.X + 18.f, Position.Y + Size.Y - 62.f),
				FVector2D(Size.X - 36.f, 42.f), 24,
				FLinearColor(1.f, .82f, .35f, 1.f));
			if (Index == 2)
			{
				Price->SetVisibility(ESlateVisibility::Collapsed);
			}
			AddButton(Blueprint, Screen,
				FName(*FString::Printf(TEXT("ShopRailButton_%d"), Index)),
				Position, Size, 29);
		}

		AddImage(Blueprint, SkillPanel, TEXT("ShopSelectionPointer"), SelectionPointer,
			FVector2D(545.f, 650.f), FVector2D(96.f, 144.f), 26);

		AddImage(Blueprint, Screen, TEXT("mSelectedItemIcon"), nullptr,
			FVector2D(718.f, 405.f), FVector2D(164.f, 164.f), 23);
		AddText(Blueprint, Screen, TEXT("mSelectedItemNameText"),
			NSLOCTEXT("ShopHorizontalRail", "PreviewName", "피의 성배"), 29,
			FVector2D(660.f, 570.f), FVector2D(280.f, 42.f), 25);
		UTextBlock* Description = AddText(Blueprint, Screen,
			TEXT("mSelectedItemDescriptionText"),
			NSLOCTEXT("ShopHorizontalRail", "PreviewDescription", "처치 시 체력 5 회복"), 19,
			FVector2D(668.f, 614.f), FVector2D(264.f, 58.f), 25,
			FLinearColor(.89f, .92f, .94f, 1.f));
		Description->SetAutoWrapText(true);
		AddText(Blueprint, Screen, TEXT("mSelectedItemPriceText"),
			NSLOCTEXT("ShopHorizontalRail", "SelectedPrice", "75 G"), 24,
			FVector2D(688.f, 696.f), FVector2D(224.f, 44.f), 25,
			FLinearColor(1.f, .82f, .35f, 1.f));

		AddLabeledButton(Blueprint, Screen, TEXT("PreviousHolder"),
			TEXT("PreviousPlate"), TEXT("PreviousLabel"), TEXT("mPreviousButton"),
			ArrowLeft, FText::GetEmpty(),
			FVector2D(14.f, 570.f), FVector2D(58.f, 98.f), 30, 34);
		AddLabeledButton(Blueprint, Screen, TEXT("NextHolder"),
			TEXT("NextPlate"), TEXT("NextLabel"), TEXT("mNextButton"),
			ArrowRight, FText::GetEmpty(),
			FVector2D(1528.f, 570.f), FVector2D(58.f, 98.f), 30, 34);

		// Skill replacement slots live in the skill panel and collapse with it.
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FVector2D Position(530.f + Index * 142.f, 790.f);
			const FVector2D Extent(126.f, 86.f);
			AddImage(Blueprint, SkillPanel,
				FName(*FString::Printf(TEXT("SkillSlotPlate_%d"), Index)),
				Index == 2 ? SkillSlotSelected : SkillSlotNormal, Position, Extent, 35, true);
			AddImage(Blueprint, SkillPanel,
				FName(*FString::Printf(TEXT("mSkillSlotIcon_%d"), Index)), nullptr,
				Position + FVector2D(26.f, 6.f), FVector2D(74.f), 37);
			AddButton(Blueprint, SkillPanel,
				FName(*FString::Printf(TEXT("mSkillSlotButton_%d"), Index)),
				Position, Extent, 39);
		}

		// Footer chrome stays clear of the rail at every supported aspect ratio.
		AddLabeledButton(Blueprint, Screen, TEXT("CloseHolder"), TEXT("ClosePlate"),
			TEXT("mCloseButtonText"), TEXT("mCloseButton"), ButtonBack,
			NSLOCTEXT("ShopHorizontalRail", "Back", "뒤로"),
			FVector2D(28.f, 890.f), FVector2D(286.f, 96.f), 40, 30);
		AddLabeledButton(Blueprint, Screen, TEXT("BuyHolder"), TEXT("ButtonPrimary"),
			TEXT("mBuyButtonText"), TEXT("mBuyButton"), ButtonPrimary,
			NSLOCTEXT("ShopHorizontalRail", "Buy", "구매"),
			FVector2D(1280.f, 890.f), FVector2D(292.f, 96.f), 40, 30);

		// Artifact inventory is a shop-local, read-only overlay. It intentionally
		// reuses the existing owned-artifact DTO/WrapBox contract and has no discard
		// affordance. A scroll container keeps every dynamically sized party list
		// reachable without introducing a separate Inventory world widget.
		UCanvasPanel* ArtifactInventoryPanel =
			Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(), TEXT("mArtifactInventoryPanel"));
		Anchor(Screen, ArtifactInventoryPanel, FAnchors(0.f, 0.f, 1.f, 1.f),
			FMargin(0.f), 80);
		ArtifactInventoryPanel->SetVisibility(ESlateVisibility::Collapsed);
		AddSolidImage(Blueprint, ArtifactInventoryPanel,
			TEXT("ArtifactInventoryDim"), FLinearColor(.005f, .009f, .016f, .82f),
			FVector2D::ZeroVector, FVector2D(DesignWidth, DesignHeight), 0);
		AddImage(Blueprint, ArtifactInventoryPanel, TEXT("ArtifactInventoryFrame"),
			InventoryPanel, FVector2D(245.f, 145.f), FVector2D(1110.f, 700.f), 2, true,
			FLinearColor(.72f, .78f, .86f, 1.f));
		AddImage(Blueprint, ArtifactInventoryPanel, TEXT("ArtifactInventoryTitlePlate"),
			TitlePlate, FVector2D(550.f, 153.f), FVector2D(500.f, 104.f), 3, true);
		AddText(Blueprint, ArtifactInventoryPanel, TEXT("ArtifactInventoryTitleText"),
			NSLOCTEXT("ShopHorizontalRail", "OwnedArtifacts", "보유 아티팩트"), 34,
			FVector2D(620.f, 171.f), FVector2D(360.f, 58.f), 5);
		AddLabeledButton(Blueprint, ArtifactInventoryPanel,
			TEXT("ArtifactInventoryCloseHolder"), TEXT("ArtifactInventoryClosePlate"),
			TEXT("ArtifactInventoryCloseText"), TEXT("mArtifactInventoryCloseButton"),
			ButtonSecondary, NSLOCTEXT("ShopHorizontalRail", "CloseInventory", "닫기"),
			FVector2D(1130.f, 174.f), FVector2D(174.f, 64.f), 6, 21);

		UScrollBox* ArtifactInventoryScroll =
			Blueprint->WidgetTree->ConstructWidget<UScrollBox>(
				UScrollBox::StaticClass(), TEXT("ArtifactInventoryScroll"));
		Place(ArtifactInventoryPanel, ArtifactInventoryScroll,
			FVector2D(325.f, 285.f), FVector2D(950.f, 480.f), 4);
		UWrapBox* OwnedArtifactBox = Blueprint->WidgetTree->ConstructWidget<UWrapBox>(
			UWrapBox::StaticClass(), TEXT("mOwnedArtifactBox"));
		OwnedArtifactBox->SetVisibility(ESlateVisibility::Collapsed);
		ArtifactInventoryScroll->AddChild(OwnedArtifactBox);

		Blueprint->ForEachSourceWidget([Blueprint](UWidget* Widget)
		{
			ExposeWithGuid(Blueprint, Widget);
		});
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);

		const FString Filename = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		if (!UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename,
			FSavePackageArgs()))
		{
			UE_LOG(LogTemp, Error, TEXT("RD_SHOP_FULL_GENERATED_BUILD save failed"));
			return;
		}

		UE_LOG(LogTemp, Display,
			TEXT("RD_SHOP_FULL_GENERATED_BUILD success asset=%s design=1600x1000 rail_slots=5 unit_slots=3 skill_slots=4"),
			AssetPath);
	}
}

void RegisterShopFullGeneratedWidgetBuilderCommands()
{
	using namespace ShopFullGeneratedWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildShopFullGenerated"),
		TEXT("Rebuild WBP_Shop_FullGenerated with generated shop artwork."),
		FConsoleCommandDelegate::CreateStatic(&Build));
}

void UnregisterShopFullGeneratedWidgetBuilderCommands()
{
	ShopFullGeneratedWidgetBuilder::BuildCommand.Reset();
}
