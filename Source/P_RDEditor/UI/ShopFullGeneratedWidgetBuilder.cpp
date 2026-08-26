
#include "UI/ShopFullGeneratedWidgetBuilder.h"

#include "AssetToolsModule.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UI/UIFont.h"
#include "UI/Hire/MercenaryHireWidget.h"
#include "UI/RunOptionsRailWidget.h"
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
	constexpr TCHAR ArtPackagePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated");
	constexpr TCHAR OptionsRailPackagePath[] = TEXT("/Game/UI/Common");
	constexpr TCHAR OptionsRailAssetName[] = TEXT("WBP_RunOptionsRail");
	constexpr TCHAR OptionsRailAssetPath[] = TEXT("/Game/UI/Common/WBP_RunOptionsRail.WBP_RunOptionsRail");
	constexpr TCHAR SharedBackgroundName[] = TEXT("T_ShopFG_KayKit_WagonHorse");
	constexpr float DesignWidth = 1600.f;
	constexpr float DesignHeight = 1000.f;
	constexpr float TopZoneHeight = 210.f;
	constexpr float CenterZoneHeight = 580.f;
	constexpr float BottomZoneHeight = 210.f;

	TUniquePtr<FAutoConsoleCommand> BuildCommand;

	FString SharedBackgroundObjectPath()
	{
		return FString::Printf(TEXT("%s/%s.%s"), ArtPackagePath,
			SharedBackgroundName, SharedBackgroundName);
	}

	void SaveObject(UObject* Object)
	{
		check(Object != nullptr);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		checkf(UPackage::SavePackage(Object->GetPackage(), Object, *Filename,
			FSavePackageArgs()), TEXT("Could not save %s"), *Object->GetPathName());
	}

	UTexture2D* EnsureSharedBackgroundTexture()
	{
		// 배경 텍스처는 SVN(OutSideAsset)에서 관리한다. 재임포트하지 않고
		// 이미 커밋된 에셋을 그대로 읽는다.
		const FString ObjectPath = SharedBackgroundObjectPath();
		UTexture2D* Background = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		checkf(Background != nullptr,
			TEXT("Missing selected KayKit shop background (SVN 업데이트 필요): %s"),
			*ObjectPath);
		return Background;
	}

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
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		// Screen-space adaptation belongs to ShopMasterScale. Every descendant keeps
		// authored 1600x1000-local coordinates so positions, extents and text baselines
		// always receive the same scale, exactly like Combat HUD's design board.
		Slot->SetAnchors(FAnchors(0.f));
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
		const int32 FontSize = 30, const bool bNineSlice = true)
	{
		UCanvasPanel* Holder = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), HolderName);
		Place(Parent, Holder, Position, Size, ZOrder);
		AddImage(Blueprint, Holder, ArtName, Art, FVector2D::ZeroVector, Size, 0,
			bNineSlice);
		UButton* Button = AddButton(Blueprint, Holder, ButtonName,
			FVector2D::ZeroVector, Size, 5);

		// Button labels are actual button content, not a visual sibling layered above
		// the hit target. This keeps alignment, opacity, focus, and accessibility in
		// one widget hierarchy.
		UOverlay* TextCenter = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), FName(*(TextName.ToString() + TEXT("_Center"))));
		UButtonSlot* ContentSlot = CastChecked<UButtonSlot>(Button->AddChild(TextCenter));
		ContentSlot->SetPadding(FMargin(0.f));
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Fill);

		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TextName);
		Text->SetText(Label);
		StyleText(Text, FontSize, FLinearColor(1.f, .91f, .73f, 1.f),
			ETextJustify::Center);
		Text->SetMargin(FMargin(0.f));
		UOverlaySlot* TextSlot = TextCenter->AddChildToOverlay(Text);
		TextSlot->SetPadding(FMargin(0.f));
		TextSlot->SetHorizontalAlignment(HAlign_Fill);
		TextSlot->SetVerticalAlignment(VAlign_Center);
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

	UWidgetBlueprint* EnsureOptionsRailBlueprint()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(
			nullptr, OptionsRailAssetPath);
		if (Blueprint == nullptr)
		{
			UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
			Factory->ParentClass = URunOptionsRailWidget::StaticClass();
			FAssetToolsModule& AssetTools =
				FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			Blueprint = Cast<UWidgetBlueprint>(AssetTools.Get().CreateAsset(
				OptionsRailAssetName, OptionsRailPackagePath,
				UWidgetBlueprint::StaticClass(), Factory));
		}
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			return nullptr;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		if (UWidget* PreviousRoot = Blueprint->WidgetTree->RootWidget)
		{
			// The shared WBP deliberately has no authored tree. Its native parent builds
			// the exact Combat HUD rail so Shop and Map cannot drift independently.
			Blueprint->ParentClass = UUserWidget::StaticClass();
			TSet<UWidget*> Widgets;
			Widgets.Add(PreviousRoot);
			FWidgetBlueprintEditorUtils::DeleteWidgets(Blueprint, MoveTemp(Widgets),
				FWidgetBlueprintEditorUtils::EDeleteWidgetWarningType::DeleteSilently);
		}
		Blueprint->ParentClass = URunOptionsRailWidget::StaticClass();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		SaveObject(Blueprint);
		return Blueprint;
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
		UTexture2D* SharedBackground = EnsureSharedBackgroundTexture();
		UTexture2D* TitlePlate = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_RoundBadge_Frame.T_MB_RoundBadge_Frame"));
		UTexture2D* TabNormal = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Button_Wide_Normal.T_KitA_Button_Wide_Normal"));
		UTexture2D* TabSelected = TabNormal;
		UTexture2D* GoldPlateArt = TitlePlate;
		UTexture2D* RailCardNormal = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_SkillCard_Frame_Combat.T_SkillCard_Frame_Combat"));
		UTexture2D* RailCardSelected = RailCardNormal;
		UTexture2D* UnitSlotNormal = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Normal.T_KitA_Cell_Normal"));
		UTexture2D* UnitSlotSelected = UnitSlotNormal;
		UTexture2D* SkillSlotNormal = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/T_ShopFG_SkillSlot07_Square.T_ShopFG_SkillSlot07_Square"));
		UTexture2D* SkillSlotSelected = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/T_ShopFG_SkillSlot07_Square_Selected.T_ShopFG_SkillSlot07_Square_Selected"));
		UTexture2D* RestUnitPanel = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/T_ShopFG_RestUnitPanel_Clean.T_ShopFG_RestUnitPanel_Clean"));
		UTexture2D* RestCostPlateArt = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireNamePlate.T_MB_HireNamePlate"));
		UTexture2D* ButtonBack = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/HUD04/T_Combat_Button_Wood_SkillConfirm_20260811_v3.T_Combat_Button_Wood_SkillConfirm_20260811_v3"));
		UTexture2D* ButtonPrimary = ButtonBack;
		UTexture2D* ArrowLeft = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Controls/T_ShopFG_Arrow_Left.T_ShopFG_Arrow_Left"));
		UTexture2D* ArrowRight = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Controls/T_ShopFG_Arrow_Right.T_ShopFG_Arrow_Right"));
		UTexture2D* MeterTrack = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Meters/T_ShopFG_MeterTrack.T_ShopFG_MeterTrack"));
		UTexture2D* MeterFill = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Meters/T_ShopFG_MeterFill.T_ShopFG_MeterFill"));
		UWidgetBlueprint* OptionsRailBlueprint = EnsureOptionsRailBlueprint();
		UClass* OptionsRailWidgetClass = OptionsRailBlueprint != nullptr
			? OptionsRailBlueprint->GeneratedClass : nullptr;
		if (OptionsRailWidgetClass == nullptr
			|| !OptionsRailWidgetClass->IsChildOf(URunOptionsRailWidget::StaticClass()))
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_SHOP_FULL_GENERATED_BUILD could not resolve embedded %s"),
				OptionsRailAssetPath);
			return;
		}
		UClass* MercenaryHireWidgetClass = LoadClass<UMercenaryHireWidget>(nullptr,
			TEXT("/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound."
				"WBP_MercenaryHire_Marchbound_C"));
		if (MercenaryHireWidgetClass == nullptr)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_SHOP_FULL_GENERATED_BUILD could not resolve mercenary hire WBP"));
			return;
		}

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

		// The scene is allowed to crop independently so it always covers the viewport.
		UScaleBox* BackgroundScale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("ShopBackgroundScale"));
		BackgroundScale->SetStretch(EStretch::ScaleToFill);
		BackgroundScale->SetStretchDirection(EStretchDirection::Both);
		BackgroundScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		BackgroundScale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		FillOverlay(Root, BackgroundScale);

		UImage* BackgroundArt = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("ShopBackgroundArt"));
		BackgroundArt->SetBrush(TextureBrush(SharedBackground));
		BackgroundArt->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		BackgroundScale->AddChild(BackgroundArt);

		// 용병 고용은 검증된 기존 선택 WBP 전체를 재사용한다. 상점 MasterScale보다
		// 먼저 쌓아 용병 배경/후보/상세는 보이되, MasterScale의 상단 탭·설정바는
		// 항상 그 위에 남도록 한다. 런타임은 용병 모드가 아닐 때 이 겹을 접는다.
		UMercenaryHireWidget* EmbeddedMercenaryHire =
			Blueprint->WidgetTree->ConstructWidget<UMercenaryHireWidget>(
				MercenaryHireWidgetClass, TEXT("mMercenaryHireWidget"));
		EmbeddedMercenaryHire->SetVisibility(ESlateVisibility::Collapsed);
		FillOverlay(Root, EmbeddedMercenaryHire);

		// Combat HUD pattern: scale one fixed design board as a unit. The former
		// per-widget normalized anchors moved positions without scaling sizes, which
		// broke card text and rest meters on tall Fold viewports.
		UScaleBox* MasterScale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("ShopMasterScale"));
		MasterScale->SetStretch(EStretch::ScaleToFit);
		MasterScale->SetStretchDirection(EStretchDirection::Both);
		MasterScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		// The master layer owns the interactive shop chrome, but its empty canvas
		// must not swallow input meant for the embedded mercenary picker below it.
		MasterScale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		FillOverlay(Root, MasterScale);

		USizeBox* DesignSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("ShopDesignSize"));
		DesignSize->SetWidthOverride(DesignWidth);
		DesignSize->SetHeightOverride(DesignHeight);
		MasterScale->AddChild(DesignSize);

		UCanvasPanel* Screen = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("ShopDesignCanvas"));
		DesignSize->AddChild(Screen);

		// The fixed design board is split into three non-overlapping local regions.
		// Mode-specific content is parented to the region that owns its semantics,
		// so later layout edits cannot make headers, cards and actions drift together.
		UCanvasPanel* TopZone = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("TopZone"));
		Place(Screen, TopZone, FVector2D::ZeroVector,
			FVector2D(DesignWidth, TopZoneHeight), 1);
		UCanvasPanel* CenterZone = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("CenterZone"));
		Place(Screen, CenterZone, FVector2D(0.f, TopZoneHeight),
			FVector2D(DesignWidth, CenterZoneHeight), 1);
		UCanvasPanel* BottomZone = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("BottomZone"));
		Place(Screen, BottomZone,
			FVector2D(0.f, TopZoneHeight + CenterZoneHeight),
			FVector2D(DesignWidth, BottomZoneHeight), 1);

		// Each mode owns its scene art and mode-only content. Runtime switches these
		// center panels while the shared item carousel stays in CenterZone.
		UCanvasPanel* ArtifactPanel = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("mArtifactShopPanel"));
		Anchor(CenterZone, ArtifactPanel, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f), 0);

		UCanvasPanel* SkillPanel = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("mSkillShopPanel"));
		Anchor(CenterZone, SkillPanel, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f), 0);
		SkillPanel->SetVisibility(ESlateVisibility::Collapsed);

		UCanvasPanel* RestPanel = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("mRestShopPanel"));
		Anchor(CenterZone, RestPanel, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f), 0);
		RestPanel->SetVisibility(ESlateVisibility::Collapsed);

		UCanvasPanel* ItemCarouselPanel = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("ItemCarouselPanel"));
		Anchor(CenterZone, ItemCarouselPanel, FAnchors(0.f, 0.f, 1.f, 1.f),
			FMargin(0.f), 5);

		UCanvasPanel* SkillTopContextPanel = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SkillTopContextPanel"));
		Anchor(TopZone, SkillTopContextPanel, FAnchors(0.f, 0.f, 1.f, 1.f),
			FMargin(0.f), 20);
		SkillTopContextPanel->SetVisibility(ESlateVisibility::Collapsed);

		UCanvasPanel* SkillBottomContextPanel = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SkillBottomContextPanel"));
		Anchor(BottomZone, SkillBottomContextPanel, FAnchors(0.f, 0.f, 1.f, 1.f),
			FMargin(0.f), 5);
		SkillBottomContextPanel->SetVisibility(ESlateVisibility::Collapsed);

		UCanvasPanel* RestBottomContextPanel = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("RestBottomContextPanel"));
		Anchor(BottomZone, RestBottomContextPanel, FAnchors(0.f, 0.f, 1.f, 1.f),
			FMargin(0.f), 5);
		RestBottomContextPanel->SetVisibility(ESlateVisibility::Collapsed);

		// Mode-wide black rail/rest scrims were removed. The authored shared
		// background now remains visible behind every functional control.

		// Header and category selectors.
		AddImage(Blueprint, TopZone, TEXT("ShopTitlePlate"), TitlePlate,
			FVector2D(24.f, 9.5f), FVector2D(280.f, 87.243f), 10);
		AddText(Blueprint, TopZone, TEXT("mTitleText"),
			NSLOCTEXT("ShopHorizontalRail", "Title", "상점"), 34,
			FVector2D(52.f, 20.f), FVector2D(224.f, 58.f), 12);

		AddLabeledButton(Blueprint, TopZone, TEXT("ArtifactTabHolder"),
			TEXT("ArtifactTabPlate"), TEXT("mArtifactTabText"), TEXT("mArtifactTabButton"),
			TabSelected, NSLOCTEXT("ShopHorizontalRail", "Artifact", "아티팩트"),
			FVector2D(330.f, 28.f), FVector2D(190.f, 49.372f), 12, 20, false);
		AddLabeledButton(Blueprint, TopZone, TEXT("SkillTabHolder"),
			TEXT("SkillTabPlate"), TEXT("mSkillTabText"), TEXT("mSkillTabButton"),
			TabNormal, NSLOCTEXT("ShopHorizontalRail", "Skill", "스킬"),
			FVector2D(540.f, 28.f), FVector2D(190.f, 49.372f), 12, 20, false);
		AddLabeledButton(Blueprint, TopZone, TEXT("RestTabHolder"),
			TEXT("RestTabPlate"), TEXT("mRestTabText"), TEXT("mRestTabButton"),
			TabNormal, NSLOCTEXT("ShopHorizontalRail", "Rest", "휴식"),
			FVector2D(750.f, 28.f), FVector2D(190.f, 49.372f), 12, 20, false);
		AddLabeledButton(Blueprint, TopZone, TEXT("MercenaryTabHolder"),
			TEXT("MercenaryTabPlate"), TEXT("mMercenaryTabText"), TEXT("mMercenaryTabButton"),
			TabNormal, NSLOCTEXT("ShopHorizontalRail", "Mercenary", "용병 고용"),
			FVector2D(960.f, 28.f), FVector2D(190.f, 49.372f), 12, 19, false);

		// The shared Combat options rail owns the upper-right corner. Keep party gold
		// directly below it so neither control steals the other's hit area.
		URunOptionsRailWidget* EmbeddedOptionsRail =
			Blueprint->WidgetTree->ConstructWidget<URunOptionsRailWidget>(
				OptionsRailWidgetClass, TEXT("mRunOptionsRailWidget"));
		Place(TopZone, EmbeddedOptionsRail, FVector2D(1198.f, 0.f),
			FVector2D(378.f, 136.f), 30);

		AddImage(Blueprint, TopZone, TEXT("GoldPlate"), GoldPlateArt,
			FVector2D(1296.f, 136.f), FVector2D(280.f, 68.f), 10);
		AddText(Blueprint, TopZone, TEXT("mGoldText"), FText::FromString(TEXT("0 G")), 26,
			FVector2D(1296.f, 136.f), FVector2D(280.f, 68.f), 12);

		// Existing contract boxes remain real widgets with their exact legacy types.
		// The horizontal-rail runtime uses the fixed controls below; these boxes stay
		// collapsed as a compatibility sink for older data-population code.
		UHorizontalBox* ItemBox = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("mItemBox"));
		Place(ItemCarouselPanel, ItemBox, FVector2D(100.f, 0.f), FVector2D(1400.f, 260.f), 7);
		ItemBox->SetVisibility(ESlateVisibility::Collapsed);

		UWrapBox* ArtifactItemBox = Blueprint->WidgetTree->ConstructWidget<UWrapBox>(
			UWrapBox::StaticClass(), TEXT("mArtifactItemBox"));
		Place(ArtifactPanel, ArtifactItemBox, FVector2D(100.f, 0.f),
			FVector2D(1400.f, 260.f), 4);
		ArtifactItemBox->SetVisibility(ESlateVisibility::Collapsed);

		UWrapBox* SkillItemBox = Blueprint->WidgetTree->ConstructWidget<UWrapBox>(
			UWrapBox::StaticClass(), TEXT("mSkillItemBox"));
		Place(SkillPanel, SkillItemBox, FVector2D(100.f, 0.f),
			FVector2D(1400.f, 260.f), 4);
		SkillItemBox->SetVisibility(ESlateVisibility::Collapsed);

		UWrapBox* OwnedUnitBox = Blueprint->WidgetTree->ConstructWidget<UWrapBox>(
			UWrapBox::StaticClass(), TEXT("mOwnedUnitBox"));
		Place(SkillPanel, OwnedUnitBox, FVector2D(1030.f, 0.f),
			FVector2D(530.f, 110.f), 4);
		OwnedUnitBox->SetVisibility(ESlateVisibility::Collapsed);

		// 여섯 직업을 모두 보여 준다. 미보유 직업은 런타임이 회색 처리하지만
		// 눌러 전용 상품의 상세를 살펴볼 수 있다.
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const FVector2D Position(424.f + Index * 128.f, 94.f);
			const FVector2D Extent(110.f, 106.f);
			AddImage(Blueprint, SkillTopContextPanel,
				FName(*FString::Printf(TEXT("UnitSelectPlate_%d"), Index)),
				Index == 0 ? UnitSlotSelected : UnitSlotNormal, Position, Extent, 8,
				false, Index == 0 ? FLinearColor::White
					: FLinearColor(.58f, .62f, .66f, .82f));
			AddImage(Blueprint, SkillTopContextPanel,
				FName(*FString::Printf(TEXT("mUnitSelectIcon_%d"), Index)), nullptr,
				Position + FVector2D(13.f, 10.f), FVector2D(84.f), 10);
			AddButton(Blueprint, SkillTopContextPanel,
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
			const float RowY = 8.f + Index * 190.f;
			UCanvasPanel* RowHolder = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(),
				FName(*FString::Printf(TEXT("RestUnitRowHolder_%d"), Index)));
			RowHolder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Place(RestPanel, RowHolder, FVector2D(190.f, RowY),
				FVector2D(1220.f, 183.f), 5);

			AddImage(Blueprint, RowHolder,
				FName(*FString::Printf(TEXT("RestUnitPlate_%d"), Index)), RestUnitPanel,
				FVector2D::ZeroVector, FVector2D(1220.f, 183.f), 6, false,
				FLinearColor(.54f, .66f, .78f, 1.f));

			// Keep the panel chrome independent from its data. The content canvas is the
			// tight 938x153 authored bounding box of name, portrait and HP/AP ledger,
			// centered inside the 1220x183 row. This prevents the former left/top bias
			// and keeps the complete unit block centered when the master ScaleBox adapts.
			UCanvasPanel* ContentHolder = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(),
				FName(*FString::Printf(TEXT("RestUnitContent_%d"), Index)));
			ContentHolder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Place(RowHolder, ContentHolder, FVector2D(141.f, 15.f),
				FVector2D(938.f, 153.f), 7);

			AddImage(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitIcon_%d"), Index)), nullptr,
				FVector2D(26.f, 33.f), FVector2D(120.f, 120.f), 8);
			AddText(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitNameText_%d"), Index)),
				FText::Format(NSLOCTEXT("ShopHorizontalRail", "RestUnitLevel", "용병 {0}"),
					FText::AsNumber(Index + 1)), 22,
				FVector2D(0.f, 0.f), FVector2D(172.f, 32.f), 9,
				FLinearColor(.95f, .88f, .7f, 1.f));

			AddText(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitHPLabel_%d"), Index)), FText::FromString(TEXT("HP")), 20,
				FVector2D(190.f, 50.f), FVector2D(56.f, 40.f), 9,
				FLinearColor(1.f, .54f, .52f, 1.f));
			AddImage(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitHPBeforeTrack_%d"), Index)),
				MeterTrack, FVector2D(258.f, 57.f), FVector2D(300.f, 28.125f), 8, true);
			AddImage(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitHPBeforeFill_%d"), Index)),
				MeterFill, FVector2D(262.f, 57.375f), FVector2D(292.f, 27.375f), 9,
				true, FLinearColor(.86f, .24f, .25f, 1.f));
			AddText(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitHPBeforeText_%d"), Index)),
				FText::FromString(FString::Printf(TEXT("%d/100"), RestHPBefore[Index])), 17,
				FVector2D(258.f, 49.f), FVector2D(300.f, 40.f), 10,
				FLinearColor::White);
			AddText(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitHPArrow_%d"), Index)), FText::FromString(TEXT("→")), 28,
				FVector2D(568.f, 48.f), FVector2D(60.f, 42.f), 10,
				FLinearColor(.85f, .88f, .9f, 1.f));
			AddImage(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitHPAfterTrack_%d"), Index)),
				MeterTrack, FVector2D(638.f, 57.f), FVector2D(300.f, 28.125f), 8, true);
			AddImage(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitHPAfterFill_%d"), Index)),
				MeterFill, FVector2D(642.f, 57.375f), FVector2D(292.f, 27.375f), 9,
				true, FLinearColor(.86f, .24f, .25f, 1.f));
			AddText(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitHPAfterText_%d"), Index)), FText::FromString(TEXT("100/100")), 17,
				FVector2D(638.f, 49.f), FVector2D(300.f, 40.f), 10,
				FLinearColor::White);

			AddText(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitAPLabel_%d"), Index)), FText::FromString(TEXT("AP")), 20,
				FVector2D(190.f, 101.f), FVector2D(56.f, 40.f), 9,
				FLinearColor(.38f, .72f, 1.f, 1.f));
			AddImage(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitAPBeforeTrack_%d"), Index)),
				MeterTrack, FVector2D(258.f, 108.f), FVector2D(300.f, 28.125f), 8, true);
			AddImage(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitAPBeforeFill_%d"), Index)),
				MeterFill, FVector2D(262.f, 108.375f), FVector2D(292.f, 27.375f), 9,
				true, FLinearColor(.16f, .56f, 1.f, 1.f));
			AddText(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitAPBeforeText_%d"), Index)),
				FText::FromString(FString::Printf(TEXT("%d/12"), RestAPBefore[Index])), 17,
				FVector2D(258.f, 100.f), FVector2D(300.f, 40.f), 10,
				FLinearColor::White);
			AddText(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitAPArrow_%d"), Index)), FText::FromString(TEXT("→")), 28,
				FVector2D(568.f, 99.f), FVector2D(60.f, 42.f), 10,
				FLinearColor(.85f, .88f, .9f, 1.f));
			AddImage(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitAPAfterTrack_%d"), Index)),
				MeterTrack, FVector2D(638.f, 108.f), FVector2D(300.f, 28.125f), 8, true);
			AddImage(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitAPAfterFill_%d"), Index)),
				MeterFill, FVector2D(642.f, 108.375f), FVector2D(292.f, 27.375f), 9,
				true, FLinearColor(.16f, .56f, 1.f, 1.f));
			AddText(Blueprint, ContentHolder,
				FName(*FString::Printf(TEXT("RestUnitAPAfterText_%d"), Index)), FText::FromString(TEXT("12/12")), 17,
				FVector2D(638.f, 100.f), FVector2D(300.f, 40.f), 10,
				FLinearColor::White);
		}

		AddImage(Blueprint, RestBottomContextPanel, TEXT("RestCostPlate"), RestCostPlateArt,
			FVector2D(570.f, 20.f), FVector2D(460.f, 106.953f), 12, false);
		AddText(Blueprint, RestBottomContextPanel, TEXT("RestCostLabel"),
			NSLOCTEXT("ShopHorizontalRail", "RestCost", "비용"), 21,
			FVector2D(640.f, 48.f), FVector2D(120.f, 50.f), 14,
			FLinearColor::White);
		AddText(Blueprint, RestBottomContextPanel, TEXT("RestCostText"), FText::FromString(TEXT("100 G")), 24,
			FVector2D(760.f, 48.f), FVector2D(200.f, 50.f), 14,
			FLinearColor(.20f, .09f, .025f, 1.f));
		AddLabeledButton(Blueprint, RestBottomContextPanel, TEXT("RestButtonHolder"),
			TEXT("RestButtonPlate"), TEXT("mRestButtonText"), TEXT("mRestButton"),
			ButtonPrimary, NSLOCTEXT("ShopHorizontalRail", "RestCTA", "휴식하기"),
			FVector2D(1268.f, 48.f), FVector2D(304.f, 137.96f), 40, 30, false);

		// Shared five-item horizontal rail. Slot 2 is the expanded selection; runtime
		// updates every icon/price and collapses unavailable slots.
		const FVector2D RailPositions[5] = {
			FVector2D(80.f, 240.f), FVector2D(300.f, 190.f),
			FVector2D(540.f, 25.f), FVector2D(1030.f, 190.f),
			FVector2D(1290.f, 240.f)
		};
		const FVector2D RailSizes[5] = {
			FVector2D(230.f, 244.174f), FVector2D(270.f, 286.64f),
			FVector2D(520.f, 552.047f), FVector2D(270.f, 286.64f),
			FVector2D(230.f, 244.174f)
		};
		for (int32 Index = 0; Index < 5; ++Index)
		{
			const FVector2D Position = RailPositions[Index];
			const FVector2D Size = RailSizes[Index];
			const int32 CarouselDepth = 2 - FMath::Abs(Index - 2);
			AddImage(Blueprint, ItemCarouselPanel,
				FName(*FString::Printf(TEXT("ShopRailPlate_%d"), Index)),
				Index == 2 ? RailCardSelected : RailCardNormal, Position, Size,
				20 + CarouselDepth,
				false, Index == 2 ? FLinearColor::White
					: FLinearColor(.58f, .62f, .66f, .86f));

			const bool bOuterSlot = Index == 0 || Index == 4;
			const float IconExtent = Index == 2 ? 205.f : (bOuterSlot ? 116.f : 145.f);
			const FVector2D IconPosition(
				Position.X + (Size.X - IconExtent) * .5f,
				Position.Y + (Index == 2 ? 62.f : 42.f));
			UImage* Icon = AddImage(Blueprint, ItemCarouselPanel,
				FName(*FString::Printf(TEXT("ShopRailIcon_%d"), Index)), nullptr,
				IconPosition, FVector2D(IconExtent), 22 + CarouselDepth);
			if (Index == 2)
			{
				Icon->SetVisibility(ESlateVisibility::Collapsed);
			}

			const float PriceBottomInset = bOuterSlot ? 84.f : 92.f;
			UTextBlock* Price = AddText(Blueprint, ItemCarouselPanel,
				FName(*FString::Printf(TEXT("ShopRailPriceText_%d"), Index)),
				FText::Format(NSLOCTEXT("ShopHorizontalRail", "PreviewPrice", "{0} G"),
					FText::AsNumber(60 + Index * 10)), 22,
				FVector2D(Position.X + 20.f,
					Position.Y + Size.Y - PriceBottomInset),
				FVector2D(Size.X - 40.f, 42.f), 24 + CarouselDepth,
				FLinearColor(1.f, .82f, .35f, 1.f));
			if (Index == 2)
			{
				Price->SetVisibility(ESlateVisibility::Collapsed);
			}
			AddButton(Blueprint, ItemCarouselPanel,
				FName(*FString::Printf(TEXT("ShopRailButton_%d"), Index)),
				Position, Size, 29 + CarouselDepth);
		}

		AddImage(Blueprint, ItemCarouselPanel, TEXT("mSelectedItemIcon"), nullptr,
			FVector2D(697.5f, 121.f), FVector2D(205.f, 205.f), 25);
		// 가운데 카드는 이름 -> 상태 한 줄 -> 값 순서로 읽힌다. 상태 줄은 예전에
		// 늘 접혀 있어서 이름/값과 겹친 자리에 남아 있었다. 스킬 탭이 모든 직업의
		// 전용 스킬을 함께 늘어놓게 되면서(0824) 이 줄이 "누구 스킬인가 / 이미
		// 가졌나"를 알리는 자리가 되므로, 셋이 겹치지 않게 다시 쌓는다.
		AddText(Blueprint, ItemCarouselPanel, TEXT("mSelectedItemNameText"),
			NSLOCTEXT("ShopHorizontalRail", "PreviewName", "피의 성배"), 29,
			FVector2D(610.f, 336.f), FVector2D(380.f, 52.f), 27);
		UTextBlock* Description = AddText(Blueprint, ItemCarouselPanel,
			TEXT("mSelectedItemDescriptionText"),
			FText::GetEmpty(), 19,
			FVector2D(590.f, 392.f), FVector2D(420.f, 40.f), 27,
			FLinearColor(.89f, .92f, .94f, 1.f));
		// 한 줄짜리 상태 문구다. 줄바꿈 대신 AutoFit 배율로 줄인다.
		Description->SetAutoWrapText(false);
		Description->SetVisibility(ESlateVisibility::Collapsed);
		AddText(Blueprint, ItemCarouselPanel, TEXT("mSelectedItemPriceText"),
			NSLOCTEXT("ShopHorizontalRail", "SelectedPrice", "75 G"), 24,
			FVector2D(650.f, 436.f), FVector2D(300.f, 52.f), 27,
			FLinearColor(1.f, .82f, .35f, 1.f));

		AddLabeledButton(Blueprint, ItemCarouselPanel, TEXT("PreviousHolder"),
			TEXT("PreviousPlate"), TEXT("PreviousLabel"), TEXT("mPreviousButton"),
			ArrowLeft, FText::GetEmpty(),
			FVector2D(14.f, 300.f), FVector2D(84.f, 126.f), 30, 34, false);
		AddLabeledButton(Blueprint, ItemCarouselPanel, TEXT("NextHolder"),
			TEXT("NextPlate"), TEXT("NextLabel"), TEXT("mNextButton"),
			ArrowRight, FText::GetEmpty(),
			FVector2D(1502.f, 300.f), FVector2D(84.f, 126.f), 30, 34, false);

		// Skill replacement slots live in the skill panel and collapse with it.
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FVector2D Position(522.f + Index * 144.f, 8.f);
			const FVector2D Extent(128.f, 128.f);
			AddImage(Blueprint, SkillBottomContextPanel,
				FName(*FString::Printf(TEXT("SkillSlotPlate_%d"), Index)),
				Index == 2 ? SkillSlotSelected : SkillSlotNormal, Position, Extent, 35, false);
			AddImage(Blueprint, SkillBottomContextPanel,
				FName(*FString::Printf(TEXT("mSkillSlotIcon_%d"), Index)), nullptr,
				Position + FVector2D(27.f), FVector2D(74.f), 37);
			AddButton(Blueprint, SkillBottomContextPanel,
				FName(*FString::Printf(TEXT("mSkillSlotButton_%d"), Index)),
				Position, Extent, 39);
		}

		// Footer chrome stays clear of the rail at every supported aspect ratio.
		AddLabeledButton(Blueprint, BottomZone, TEXT("CloseHolder"), TEXT("ClosePlate"),
			TEXT("mCloseButtonText"), TEXT("mCloseButton"), ButtonBack,
			NSLOCTEXT("ShopHorizontalRail", "Leave", "나가기"),
			FVector2D(28.f, 48.f), FVector2D(304.f, 137.96f), 40, 30, false);
		AddLabeledButton(Blueprint, BottomZone, TEXT("BuyHolder"), TEXT("ButtonPrimary"),
			TEXT("mBuyButtonText"), TEXT("mBuyButton"), ButtonPrimary,
			NSLOCTEXT("ShopHorizontalRail", "Buy", "구매"),
			FVector2D(1268.f, 48.f), FVector2D(304.f, 137.96f), 40, 30, false);

		Blueprint->ForEachSourceWidget([Blueprint](UWidget* Widget)
		{
			if (UTextBlock* Text = Cast<UTextBlock>(Widget))
			{
				Text->SetJustification(ETextJustify::Center);
			}
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
		UE_LOG(LogTemp, Display,
			TEXT("RD_SHOP_FULL_GENERATED_BUILD embedded_common_rail=%s zones=210/580/210"),
			OptionsRailAssetPath);
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
