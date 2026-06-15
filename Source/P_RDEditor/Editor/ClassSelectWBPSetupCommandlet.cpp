#include "Editor/ClassSelectWBPSetupCommandlet.h"

#include "RDMinimal.h"

DEFINE_LOG_CATEGORY_STATIC(LogClassSelectWBPSetup, Log, All);

#if WITH_EDITOR
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "IAssetTools.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UI/CharacterCardWidget.h"
#include "UI/CharacterSelectWidget.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#endif

namespace
{
#if WITH_EDITOR
	constexpr const TCHAR* ClassSelectAssetPath = TEXT("/Game/BP/UI/WBP_CharacterSelect.WBP_CharacterSelect");
	constexpr const TCHAR* CharacterCardAssetPath = TEXT("/Game/BP/UI/WBP_CharacterCard.WBP_CharacterCard");
	constexpr const TCHAR* CharacterCardClassPath = TEXT("/Game/BP/UI/WBP_CharacterCard.WBP_CharacterCard_C");
	constexpr const TCHAR* CharacterCardAssetFolder = TEXT("/Game/BP/UI");
	constexpr const TCHAR* ClassSelectTexturePath = TEXT("/Game/SVN/OutSideAsset/AICreation/ClassSelect");

	struct FClassSelectTextureSpec
	{
		const TCHAR* SourceFileName = nullptr;
		const TCHAR* AssetName = nullptr;
	};

	const FClassSelectTextureSpec TextureSpecs[] = {
		{ TEXT("classselect_knight_action_illustration_1920x1080.png"), TEXT("classselect_knight_action_illustration_1920x1080") },
		{ TEXT("classselect_mage_action_illustration_1920x1080.png"), TEXT("classselect_mage_action_illustration_1920x1080") },
		{ TEXT("classselect_rogue_action_illustration_1920x1080.png"), TEXT("classselect_rogue_action_illustration_1920x1080") },
		{ TEXT("classselect_knight_symbol_button_1024x512.png"), TEXT("classselect_knight_symbol_button_1024x512") },
		{ TEXT("classselect_mage_symbol_button_1024x512.png"), TEXT("classselect_mage_symbol_button_1024x512") },
		{ TEXT("classselect_rogue_symbol_button_1024x512.png"), TEXT("classselect_rogue_symbol_button_1024x512") },
		{ TEXT("classselect_navigation_button_darkfantasy_base.png"), TEXT("classselect_navigation_button_darkfantasy_base") },
	};

	struct FClassSelectCardWBPVariantSpec
	{
		const TCHAR* WidgetName = nullptr;
		const TCHAR* AssetName = nullptr;
		const TCHAR* IconTextureName = nullptr;
	};

	const FClassSelectCardWBPVariantSpec CardVariantSpecs[] = {
		{ TEXT("ClassSelectCard_Knight"), TEXT("WBP_CharacterCard_Knight"), TEXT("classselect_knight_symbol_button_1024x512") },
		{ TEXT("ClassSelectCard_Rogue"), TEXT("WBP_CharacterCard_Rogue"), TEXT("classselect_rogue_symbol_button_1024x512") },
		{ TEXT("ClassSelectCard_Mage"), TEXT("WBP_CharacterCard_Mage"), TEXT("classselect_mage_symbol_button_1024x512") },
	};

	struct FClassSelectActionImageSpec
	{
		const TCHAR* WidgetName = nullptr;
		const TCHAR* TextureName = nullptr;
		EPlayerJobType JobType = EPlayerJobType::None;
	};

	const FClassSelectActionImageSpec ActionImageSpecs[] = {
		{ TEXT("mKnightActionImage"), TEXT("classselect_knight_action_illustration_1920x1080"), EPlayerJobType::Knight },
		{ TEXT("mRogueActionImage"), TEXT("classselect_rogue_action_illustration_1920x1080"), EPlayerJobType::Archer },
		{ TEXT("mMageActionImage"), TEXT("classselect_mage_action_illustration_1920x1080"), EPlayerJobType::Mage },
	};

	FString GetTextureAssetObjectPath(const TCHAR* AssetName)
	{
		return FString::Printf(TEXT("%s/%s.%s"), ClassSelectTexturePath, AssetName, AssetName);
	}

	UTexture2D* LoadTextureAsset(const TCHAR* AssetName)
	{
		return LoadObject<UTexture2D>(nullptr, *GetTextureAssetObjectPath(AssetName));
	}

	FString GetWidgetBlueprintObjectPath(const TCHAR* AssetName)
	{
		return FString::Printf(TEXT("%s/%s.%s"), CharacterCardAssetFolder, AssetName, AssetName);
	}

	FString GetWidgetBlueprintGeneratedClassPath(const TCHAR* AssetName)
	{
		return FString::Printf(TEXT("%s/%s.%s_C"), CharacterCardAssetFolder, AssetName, AssetName);
	}

	void SaveAssetObject(UObject* Asset)
	{
		if (Asset == nullptr)
		{
			return;
		}

		UPackage* Package = Asset->GetOutermost();
		if (Package == nullptr)
		{
			return;
		}

		Package->MarkPackageDirty();
		const FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		UPackage::SavePackage(Package, nullptr, *PackageFileName, SaveArgs);
	}

	void ImportClassSelectTextures()
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		TArray<UAssetImportTask*> ImportTasks;

		for (const FClassSelectTextureSpec& Spec : TextureSpecs)
		{
			if (LoadTextureAsset(Spec.AssetName) != nullptr)
			{
				continue;
			}

			const FString SourceFilePath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("SVN/OutSideAsset/AICreation/ClassSelect"), Spec.SourceFileName);
			if (!FPaths::FileExists(SourceFilePath))
			{
				UE_LOG(LogClassSelectWBPSetup, Warning, TEXT("ClassSelectWBPSetup: source texture missing: %s"), *SourceFilePath);
				continue;
			}

			UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
			TextureFactory->bCreateMaterial = false;
			TextureFactory->bDeferCompression = true;
			TextureFactory->CompressionSettings = TC_Default;

			UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
			ImportTask->Filename = SourceFilePath;
			ImportTask->DestinationPath = ClassSelectTexturePath;
			ImportTask->DestinationName = Spec.AssetName;
			ImportTask->bReplaceExisting = true;
			ImportTask->bReplaceExistingSettings = false;
			ImportTask->bAutomated = true;
			ImportTask->bSave = true;
			ImportTask->Factory = TextureFactory;
			ImportTasks.Add(ImportTask);
		}

		if (!ImportTasks.IsEmpty())
		{
			AssetToolsModule.Get().ImportAssetTasks(ImportTasks);
		}
	}

	void SetObjectTextureProperty(UObject* Object, const TCHAR* PropertyName, UTexture2D* Texture)
	{
		if (Object == nullptr)
		{
			return;
		}

		if (FObjectPropertyBase* ObjectProperty = FindFProperty<FObjectPropertyBase>(Object->GetClass(), PropertyName))
		{
			Object->Modify();
			ObjectProperty->SetObjectPropertyValue_InContainer(Object, Texture);
		}
		else
		{
			UE_LOG(LogClassSelectWBPSetup, Warning, TEXT("ClassSelectWBPSetup: missing object property %s on %s"), PropertyName, *Object->GetName());
		}
	}

	void SetVectorProperty(UObject* Object, const TCHAR* PropertyName, const FVector2D& Value)
	{
		if (Object == nullptr)
		{
			return;
		}

		if (FStructProperty* StructProperty = FindFProperty<FStructProperty>(Object->GetClass(), PropertyName))
		{
			Object->Modify();
			void* Destination = StructProperty->ContainerPtrToValuePtr<void>(Object);
			StructProperty->CopyCompleteValue(Destination, &Value);
		}
		else
		{
			UE_LOG(LogClassSelectWBPSetup, Warning, TEXT("ClassSelectWBPSetup: missing vector property %s on %s"), PropertyName, *Object->GetName());
		}
	}

	void SetLinearColorProperty(UObject* Object, const TCHAR* PropertyName, const FLinearColor& Value)
	{
		if (Object == nullptr)
		{
			return;
		}

		if (FStructProperty* StructProperty = FindFProperty<FStructProperty>(Object->GetClass(), PropertyName))
		{
			Object->Modify();
			void* Destination = StructProperty->ContainerPtrToValuePtr<void>(Object);
			StructProperty->CopyCompleteValue(Destination, &Value);
		}
		else
		{
			UE_LOG(LogClassSelectWBPSetup, Warning, TEXT("ClassSelectWBPSetup: missing linear color property %s on %s"), PropertyName, *Object->GetName());
		}
	}

	FSlateBrush MakeImageBrush(UTexture2D* Texture, const FLinearColor& Tint = FLinearColor::White)
	{
		FSlateBrush Brush;
		if (Texture != nullptr)
		{
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Brush.ImageSize = FVector2D(static_cast<float>(Texture->GetSizeX()), static_cast<float>(Texture->GetSizeY()));
			Brush.SetResourceObject(Texture);
		}
		else
		{
			Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
		}
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}

	FSlateBrush MakeNoDrawBrush()
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
		Brush.TintColor = FSlateColor(FLinearColor::Transparent);
		return Brush;
	}

	void ApplyButtonArt(UButton* Button, UTexture2D* Texture)
	{
		if (Button == nullptr)
		{
			return;
		}

		Button->Modify();
		FButtonStyle Style = Button->GetStyle();
		Style.SetNormal(MakeImageBrush(Texture));
		Style.SetHovered(MakeImageBrush(Texture, FLinearColor(1.08f, 1.06f, 1.0f, 1.0f)));
		Style.SetPressed(MakeImageBrush(Texture, FLinearColor(0.70f, 0.78f, 0.78f, 1.0f)));
		Style.SetDisabled(MakeImageBrush(Texture, FLinearColor(0.42f, 0.44f, 0.46f, 0.58f)));
		Style.SetNormalPadding(FMargin(0.0f));
		Style.SetPressedPadding(FMargin(1.0f, 2.0f, 0.0f, 0.0f));
		Button->SetStyle(Style);
	}

	void ApplyTransparentButton(UButton* Button)
	{
		if (Button == nullptr)
		{
			return;
		}

		Button->Modify();
		FButtonStyle Style = Button->GetStyle();
		const FSlateBrush NoDrawBrush = MakeNoDrawBrush();
		Style.SetNormal(NoDrawBrush);
		Style.SetHovered(NoDrawBrush);
		Style.SetPressed(NoDrawBrush);
		Style.SetDisabled(NoDrawBrush);
		Style.SetNormalPadding(FMargin(0.0f));
		Style.SetPressedPadding(FMargin(0.0f));
		Button->SetStyle(Style);
	}

	void ApplyButtonText(UTextBlock* TextBlock, const FText& Text)
	{
		if (TextBlock == nullptr)
		{
			return;
		}

		TextBlock->Modify();
		TextBlock->SetText(Text);
		FSlateFontInfo FontInfo = TextBlock->GetFont();
		FontInfo.Size = 24;
		TextBlock->SetFont(FontInfo);
		TextBlock->SetJustification(ETextJustify::Center);
		TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.84f, 0.57f, 1.0f)));
		TextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.86f));
		TextBlock->SetShadowOffset(FVector2D(1.0f, 2.0f));
		TextBlock->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	void ApplyCanvasLayout(UWidget* Widget, const FVector2D& Anchor, const FVector2D& Position, const FVector2D& Size, const FVector2D& Alignment, int32 ZOrder)
	{
		if (UCanvasPanelSlot* CanvasSlot = Widget != nullptr ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr)
		{
			CanvasSlot->Modify();
			CanvasSlot->SetAnchors(FAnchors(Anchor.X, Anchor.Y, Anchor.X, Anchor.Y));
			CanvasSlot->SetAlignment(Alignment);
			CanvasSlot->SetPosition(Position);
			CanvasSlot->SetSize(Size);
			CanvasSlot->SetZOrder(ZOrder);
		}
	}

	void MakeWidgetTransparent(UWidget* Widget)
	{
		if (UBorder* Border = Cast<UBorder>(Widget))
		{
			Border->Modify();
			Border->SetBrushColor(FLinearColor::Transparent);
			Border->SetVisibility(ESlateVisibility::HitTestInvisible);
			return;
		}

		if (UImage* Image = Cast<UImage>(Widget))
		{
			Image->Modify();
			Image->SetColorAndOpacity(FLinearColor::Transparent);
			Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	void RemoveWidgetVariableGuid(UWidgetBlueprint* WidgetBP, const FName& WidgetName)
	{
		if (WidgetBP != nullptr && WidgetBP->WidgetVariableNameToGuidMap.Contains(WidgetName))
		{
			WidgetBP->OnVariableRemoved(WidgetName);
			WidgetBP->WidgetVariableNameToGuidMap.Remove(WidgetName);
		}
	}

	void AddWidgetVariableGuid(UWidgetBlueprint* WidgetBP, const FName& WidgetName)
	{
		if (WidgetBP != nullptr && !WidgetBP->WidgetVariableNameToGuidMap.Contains(WidgetName))
		{
			WidgetBP->OnVariableAdded(WidgetName);
		}
	}

	UWidgetBlueprint* LoadOrCreateCardVariantBlueprint(UWidgetBlueprint* BaseCardBP, const FClassSelectCardWBPVariantSpec& Spec)
	{
		UWidgetBlueprint* VariantBP = LoadObject<UWidgetBlueprint>(nullptr, *GetWidgetBlueprintObjectPath(Spec.AssetName));
		if (VariantBP != nullptr)
		{
			return VariantBP;
		}

		if (BaseCardBP == nullptr)
		{
			return nullptr;
		}

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* DuplicatedAsset = AssetToolsModule.Get().DuplicateAsset(
			Spec.AssetName,
			CharacterCardAssetFolder,
			BaseCardBP);

		VariantBP = Cast<UWidgetBlueprint>(DuplicatedAsset);
		if (VariantBP == nullptr)
		{
			UE_LOG(LogClassSelectWBPSetup, Warning, TEXT("ClassSelectWBPSetup: failed to create %s."), Spec.AssetName);
		}
		return VariantBP;
	}

	void ApplyCharacterCardWBPVisuals(UWidgetBlueprint* CardBP, UTexture2D* IconTexture)
	{
		if (CardBP == nullptr || CardBP->WidgetTree == nullptr)
		{
			return;
		}

		CardBP->Modify();
		CardBP->WidgetTree->Modify();

		USizeBox* RootSizeBox = Cast<USizeBox>(CardBP->WidgetTree->FindWidget(TEXT("MobileCardRootSize")));
		if (RootSizeBox == nullptr)
		{
			RootSizeBox = CardBP->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MobileCardRootSize"));
		}

		UOverlay* CardOverlay = Cast<UOverlay>(CardBP->WidgetTree->FindWidget(TEXT("MobileCardOverlay")));
		if (CardOverlay == nullptr)
		{
			CardOverlay = CardBP->WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MobileCardOverlay"));
		}

		UImage* IconImage = Cast<UImage>(CardBP->WidgetTree->FindWidget(TEXT("IconImage")));
		if (IconImage == nullptr)
		{
			IconImage = CardBP->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IconImage"));
		}

		UButton* CardButton = Cast<UButton>(CardBP->WidgetTree->FindWidget(TEXT("CardButton")));
		if (CardButton == nullptr)
		{
			CardButton = CardBP->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CardButton"));
		}

		if (RootSizeBox == nullptr || CardOverlay == nullptr || IconImage == nullptr || CardButton == nullptr)
		{
			UE_LOG(LogClassSelectWBPSetup, Warning, TEXT("ClassSelectWBPSetup: failed to build card visual tree for %s."), *CardBP->GetName());
			return;
		}

		RootSizeBox->Modify();
		CardOverlay->Modify();
		IconImage->Modify();
		CardButton->Modify();

		if (CardBP->WidgetTree->RootWidget != RootSizeBox)
		{
			RootSizeBox->RemoveFromParent();
			CardBP->WidgetTree->RootWidget = RootSizeBox;
		}

		CardOverlay->RemoveFromParent();
		IconImage->RemoveFromParent();
		CardButton->RemoveFromParent();

		RootSizeBox->ClearChildren();
		RootSizeBox->AddChild(CardOverlay);
		CardOverlay->ClearChildren();
		if (UOverlaySlot* IconSlot = CardOverlay->AddChildToOverlay(IconImage))
		{
			IconSlot->Modify();
			IconSlot->SetHorizontalAlignment(HAlign_Fill);
			IconSlot->SetVerticalAlignment(VAlign_Fill);
			IconSlot->SetPadding(FMargin(0.0f));
		}
		if (UOverlaySlot* ButtonSlot = CardOverlay->AddChildToOverlay(CardButton))
		{
			ButtonSlot->Modify();
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
			ButtonSlot->SetVerticalAlignment(VAlign_Fill);
			ButtonSlot->SetPadding(FMargin(0.0f));
		}

		if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(CardOverlay->Slot))
		{
			OverlaySlot->Modify();
			OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(VAlign_Fill);
			OverlaySlot->SetPadding(FMargin(0.0f));
		}

		if (RootSizeBox != nullptr)
		{
			RootSizeBox->SetWidthOverride(154.0f);
			RootSizeBox->SetHeightOverride(77.0f);
			RootSizeBox->SetVisibility(ESlateVisibility::Visible);
			RootSizeBox->SetRenderOpacity(1.0f);
			RootSizeBox->SetClipping(EWidgetClipping::Inherit);
		}

		{
			IconImage->SetBrushFromTexture(IconTexture, true);
			IconImage->SetColorAndOpacity(FLinearColor::White);
			IconImage->SetRenderOpacity(1.0f);
			IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			IconImage->SetClipping(EWidgetClipping::Inherit);
		}

		MakeWidgetTransparent(CardBP->WidgetTree->FindWidget(TEXT("IconBackground")));
		MakeWidgetTransparent(CardBP->WidgetTree->FindWidget(TEXT("MobileCardBody")));
		MakeWidgetTransparent(CardBP->WidgetTree->FindWidget(TEXT("MobileCardFrame")));
		{
			CardButton->SetVisibility(ESlateVisibility::Visible);
			CardButton->SetRenderOpacity(1.0f);
			ApplyTransparentButton(CardButton);
		}

		const FName LegacyTextNames[] = {
			TEXT("MobileCardFrame"),
			TEXT("MobileCardBody"),
			TEXT("IconBackground"),
			TEXT("NameText"),
			TEXT("RoleText"),
			TEXT("StatText"),
			TEXT("DescriptionText"),
			TEXT("StateText"),
		};

		for (const FName& TextName : LegacyTextNames)
		{
			if (UTextBlock* TextBlock = Cast<UTextBlock>(CardBP->WidgetTree->FindWidget(TextName)))
			{
				TextBlock->Modify();
				TextBlock->SetText(FText::GetEmpty());
				TextBlock->SetVisibility(ESlateVisibility::Collapsed);
			}
			RemoveWidgetVariableGuid(CardBP, TextName);
		}

		CardBP->WidgetVariableNameToGuidMap.Empty();
		CardBP->WidgetTree->ForEachWidget([CardBP](UWidget* Widget)
		{
			if (Widget != nullptr)
			{
				AddWidgetVariableGuid(CardBP, Widget->GetFName());
			}
		});

		FBlueprintEditorUtils::MarkBlueprintAsModified(CardBP);
		FKismetEditorUtilities::CompileBlueprint(CardBP);
		SaveAssetObject(CardBP);
	}

	void EnsureClassCardVariantBlueprints(UWidgetBlueprint* BaseCardBP)
	{
		for (const FClassSelectCardWBPVariantSpec& Spec : CardVariantSpecs)
		{
			UWidgetBlueprint* VariantBP = LoadOrCreateCardVariantBlueprint(BaseCardBP, Spec);
			ApplyCharacterCardWBPVisuals(VariantBP, LoadTextureAsset(Spec.IconTextureName));
		}
	}

	struct FCardSlotSnapshot
	{
		bool bHasHorizontalSlot = false;
		FMargin Padding = FMargin(7.0f, 0.0f);
		EHorizontalAlignment HorizontalAlignment = HAlign_Center;
		EVerticalAlignment VerticalAlignment = VAlign_Center;
	};

	FCardSlotSnapshot CaptureCardSlotSnapshot(UWidget* Widget)
	{
		FCardSlotSnapshot Snapshot;
		if (UHorizontalBoxSlot* HorizontalSlot = Widget != nullptr ? Cast<UHorizontalBoxSlot>(Widget->Slot) : nullptr)
		{
			Snapshot.bHasHorizontalSlot = true;
			Snapshot.Padding = HorizontalSlot->GetPadding();
			Snapshot.HorizontalAlignment = HorizontalSlot->GetHorizontalAlignment();
			Snapshot.VerticalAlignment = HorizontalSlot->GetVerticalAlignment();
		}
		return Snapshot;
	}

	void ApplyCardSlotSnapshot(UPanelSlot* PanelSlot, const FCardSlotSnapshot& Snapshot)
	{
		if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(PanelSlot))
		{
			HorizontalSlot->Modify();
			HorizontalSlot->SetPadding(Snapshot.Padding);
			HorizontalSlot->SetHorizontalAlignment(Snapshot.HorizontalAlignment);
			HorizontalSlot->SetVerticalAlignment(Snapshot.VerticalAlignment);
		}
	}

	void MoveWidgetOutOfWidgetTree(UWidgetBlueprint* WidgetBP, UWidget* Widget, const FString& BaseName)
	{
		if (WidgetBP == nullptr || WidgetBP->WidgetTree == nullptr || Widget == nullptr)
		{
			return;
		}

		WidgetBP->WidgetTree->RemoveWidget(Widget);
		RemoveWidgetVariableGuid(WidgetBP, Widget->GetFName());

		const FName TransientName = MakeUniqueObjectName(
			GetTransientPackage(),
			Widget->GetClass(),
			*FString::Printf(TEXT("%s_Deprecated"), *BaseName));
		Widget->ClearFlags(RF_Public | RF_Standalone);
		Widget->SetFlags(RF_Transient);
		Widget->Rename(*TransientName.ToString(), GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
	}

	void RemoveDeprecatedClassSelectCards(UWidgetBlueprint* SelectBP)
	{
		if (SelectBP == nullptr || SelectBP->WidgetTree == nullptr)
		{
			return;
		}

		TArray<UWidget*> DeprecatedWidgets;
		SelectBP->WidgetTree->ForEachWidget([&DeprecatedWidgets](UWidget* Widget)
		{
			if (Widget == nullptr)
			{
				return;
			}

			const FString WidgetName = Widget->GetName();
			if (WidgetName.StartsWith(TEXT("ClassSelectCard_")) && WidgetName.Contains(TEXT("Deprecated")))
			{
				DeprecatedWidgets.Add(Widget);
			}
		});

		for (UWidget* DeprecatedWidget : DeprecatedWidgets)
		{
			MoveWidgetOutOfWidgetTree(SelectBP, DeprecatedWidget, DeprecatedWidget->GetName());
		}

		TArray<FName> DeprecatedVariableNames;
		for (const TPair<FName, FGuid>& VariableGuidPair : SelectBP->WidgetVariableNameToGuidMap)
		{
			const FString VariableName = VariableGuidPair.Key.ToString();
			if (VariableName.StartsWith(TEXT("ClassSelectCard_")) && VariableName.Contains(TEXT("Deprecated")))
			{
				DeprecatedVariableNames.Add(VariableGuidPair.Key);
			}
		}

		for (const FName& DeprecatedVariableName : DeprecatedVariableNames)
		{
			RemoveWidgetVariableGuid(SelectBP, DeprecatedVariableName);
		}
	}

	void CopyCanvasSlotLayout(UWidget* SourceWidget, UWidget* TargetWidget, int32 ZOrderOffset)
	{
		UCanvasPanelSlot* SourceSlot = SourceWidget != nullptr ? Cast<UCanvasPanelSlot>(SourceWidget->Slot) : nullptr;
		UCanvasPanelSlot* TargetSlot = TargetWidget != nullptr ? Cast<UCanvasPanelSlot>(TargetWidget->Slot) : nullptr;
		if (SourceSlot == nullptr || TargetSlot == nullptr)
		{
			return;
		}

		TargetSlot->Modify();
		TargetSlot->SetAnchors(SourceSlot->GetAnchors());
		TargetSlot->SetAlignment(SourceSlot->GetAlignment());
		TargetSlot->SetPosition(SourceSlot->GetPosition());
		TargetSlot->SetSize(SourceSlot->GetSize());
		TargetSlot->SetZOrder(SourceSlot->GetZOrder() + ZOrderOffset);
	}

	void ApplyActionImageWBPVisual(UImage* Image, UTexture2D* Texture, bool bVisible)
	{
		if (Image == nullptr)
		{
			return;
		}

		Image->Modify();
		Image->SetBrushFromTexture(Texture, true);
		Image->SetColorAndOpacity(FLinearColor::White);
		Image->SetRenderOpacity(1.0f);
		Image->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		Image->SetClipping(EWidgetClipping::Inherit);
	}

	void EnsureClassActionImages(UWidgetBlueprint* SelectBP)
	{
		if (SelectBP == nullptr || SelectBP->WidgetTree == nullptr)
		{
			return;
		}

		UImage* TemplateImage = Cast<UImage>(SelectBP->WidgetTree->FindWidget(TEXT("mSelectedCharacterPortraitImage")));
		UPanelWidget* ParentPanel = TemplateImage != nullptr ? TemplateImage->GetParent() : Cast<UPanelWidget>(SelectBP->WidgetTree->RootWidget);
		if (ParentPanel == nullptr)
		{
			UE_LOG(LogClassSelectWBPSetup, Warning, TEXT("ClassSelectWBPSetup: action image parent panel not found."));
			return;
		}

		SelectBP->Modify();
		SelectBP->WidgetTree->Modify();
		ParentPanel->Modify();

		if (TemplateImage != nullptr)
		{
			TemplateImage->Modify();
			TemplateImage->SetVisibility(ESlateVisibility::Collapsed);
		}

		for (int32 ImageIndex = 0; ImageIndex < UE_ARRAY_COUNT(ActionImageSpecs); ++ImageIndex)
		{
			const FClassSelectActionImageSpec& Spec = ActionImageSpecs[ImageIndex];
			const FName ActionImageName(Spec.WidgetName);
			UImage* ActionImage = Cast<UImage>(SelectBP->WidgetTree->FindWidget(ActionImageName));
			if (ActionImage == nullptr)
			{
				ActionImage = SelectBP->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), ActionImageName);
				if (ActionImage == nullptr)
				{
					continue;
				}

				ParentPanel->AddChild(ActionImage);
				AddWidgetVariableGuid(SelectBP, ActionImageName);
			}

			CopyCanvasSlotLayout(TemplateImage, ActionImage, ImageIndex);
			ApplyActionImageWBPVisual(ActionImage, LoadTextureAsset(Spec.TextureName), Spec.JobType == EPlayerJobType::Knight);
		}

		FBlueprintEditorUtils::MarkBlueprintAsModified(SelectBP);
	}

	void DumpClassSelectCardContainer(UWidgetBlueprint* SelectBP)
	{
		if (SelectBP == nullptr || SelectBP->WidgetTree == nullptr)
		{
			return;
		}

		UPanelWidget* CardContainer = Cast<UPanelWidget>(SelectBP->WidgetTree->FindWidget(TEXT("mCharacterCardContainer")));
		if (CardContainer == nullptr)
		{
			UE_LOG(LogClassSelectWBPSetup, Warning, TEXT("ClassSelectWBPSetup: dump failed, mCharacterCardContainer not found."));
			return;
		}

		UE_LOG(LogClassSelectWBPSetup, Display, TEXT("ClassSelectWBPSetup: mCharacterCardContainer class=%s children=%d visibility=%d opacity=%.2f"),
			*CardContainer->GetClass()->GetName(),
			CardContainer->GetChildrenCount(),
			static_cast<int32>(CardContainer->GetVisibility()),
			CardContainer->GetRenderOpacity());

		for (UWidget* ParentWidget = CardContainer; ParentWidget != nullptr; ParentWidget = ParentWidget->GetParent())
		{
			FString LayoutText = TEXT("slot=none");
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ParentWidget->Slot))
			{
				const FVector2D Position = CanvasSlot->GetPosition();
				const FVector2D Size = CanvasSlot->GetSize();
				const FVector2D Alignment = CanvasSlot->GetAlignment();
				const FAnchors Anchors = CanvasSlot->GetAnchors();
				LayoutText = FString::Printf(
					TEXT("slot=Canvas pos=(%.1f,%.1f) size=(%.1f,%.1f) align=(%.1f,%.1f) anchors=(%.2f,%.2f,%.2f,%.2f) z=%d"),
					Position.X,
					Position.Y,
					Size.X,
					Size.Y,
					Alignment.X,
					Alignment.Y,
					Anchors.Minimum.X,
					Anchors.Minimum.Y,
					Anchors.Maximum.X,
					Anchors.Maximum.Y,
					CanvasSlot->GetZOrder());
			}
			else if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(ParentWidget->Slot))
			{
				const FMargin Padding = HorizontalSlot->GetPadding();
				LayoutText = FString::Printf(
					TEXT("slot=Horizontal padding=(%.1f,%.1f,%.1f,%.1f) h=%d v=%d"),
					Padding.Left,
					Padding.Top,
					Padding.Right,
					Padding.Bottom,
					static_cast<int32>(HorizontalSlot->GetHorizontalAlignment()),
					static_cast<int32>(HorizontalSlot->GetVerticalAlignment()));
			}

			UE_LOG(LogClassSelectWBPSetup, Display, TEXT("ClassSelectWBPSetup: parent name=%s class=%s visibility=%d opacity=%.2f %s"),
				*ParentWidget->GetName(),
				*ParentWidget->GetClass()->GetName(),
				static_cast<int32>(ParentWidget->GetVisibility()),
				ParentWidget->GetRenderOpacity(),
				*LayoutText);
		}

		for (int32 ChildIndex = 0; ChildIndex < CardContainer->GetChildrenCount(); ++ChildIndex)
		{
			UWidget* ChildWidget = CardContainer->GetChildAt(ChildIndex);
			if (ChildWidget == nullptr)
			{
				continue;
			}

			FString SlotText = TEXT("slot=none");
			if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(ChildWidget->Slot))
			{
				const FMargin Padding = HorizontalSlot->GetPadding();
				SlotText = FString::Printf(
					TEXT("slot=Horizontal padding=(%.1f,%.1f,%.1f,%.1f) h=%d v=%d"),
					Padding.Left,
					Padding.Top,
					Padding.Right,
					Padding.Bottom,
					static_cast<int32>(HorizontalSlot->GetHorizontalAlignment()),
					static_cast<int32>(HorizontalSlot->GetVerticalAlignment()));
			}

			UE_LOG(LogClassSelectWBPSetup, Display, TEXT("ClassSelectWBPSetup: card[%d] name=%s class=%s visibility=%d opacity=%.2f %s"),
				ChildIndex,
				*ChildWidget->GetName(),
				*ChildWidget->GetClass()->GetPathName(),
				static_cast<int32>(ChildWidget->GetVisibility()),
				ChildWidget->GetRenderOpacity(),
				*SlotText);
		}
	}

	void SetupCharacterCardBlueprint(UWidgetBlueprint* CardBP)
	{
		if (CardBP == nullptr || CardBP->WidgetTree == nullptr)
		{
			return;
		}

		CardBP->Modify();
		CardBP->WidgetTree->Modify();

		ApplyCharacterCardWBPVisuals(CardBP, LoadTextureAsset(TEXT("classselect_knight_symbol_button_1024x512")));
		EnsureClassCardVariantBlueprints(CardBP);
	}

	void AddDesignerCardsToSelectBlueprint(UWidgetBlueprint* SelectBP)
	{
		if (SelectBP == nullptr || SelectBP->WidgetTree == nullptr)
		{
			return;
		}

		UPanelWidget* CardContainer = Cast<UPanelWidget>(SelectBP->WidgetTree->FindWidget(TEXT("mCharacterCardContainer")));
		if (CardContainer == nullptr)
		{
			UE_LOG(LogClassSelectWBPSetup, Warning, TEXT("ClassSelectWBPSetup: mCharacterCardContainer not found."));
			return;
		}

		SelectBP->Modify();
		SelectBP->WidgetTree->Modify();
		CardContainer->Modify();
		RemoveDeprecatedClassSelectCards(SelectBP);

		TMap<FName, FCardSlotSnapshot> PreviousSlotSnapshots;
		for (const FClassSelectCardWBPVariantSpec& Spec : CardVariantSpecs)
		{
			const FName CardName(Spec.WidgetName);
			if (UWidget* ExistingWidget = SelectBP->WidgetTree->FindWidget(CardName))
			{
				PreviousSlotSnapshots.Add(CardName, CaptureCardSlotSnapshot(ExistingWidget));
			}
		}

		for (const FClassSelectCardWBPVariantSpec& Spec : CardVariantSpecs)
		{
			const FName CardName(Spec.WidgetName);
			UWidget* OldWidget = SelectBP->WidgetTree->FindWidget(CardName);
			if (OldWidget == nullptr)
			{
				OldWidget = FindObject<UWidget>(SelectBP->WidgetTree, *CardName.ToString());
			}
			if (OldWidget != nullptr)
			{
				MoveWidgetOutOfWidgetTree(SelectBP, OldWidget, CardName.ToString());
			}
			RemoveWidgetVariableGuid(SelectBP, CardName);
		}

		CardContainer->ClearChildren();

		for (const FClassSelectCardWBPVariantSpec& Spec : CardVariantSpecs)
		{
			const FName CardName(Spec.WidgetName);
			UClass* CardClass = LoadObject<UClass>(nullptr, *GetWidgetBlueprintGeneratedClassPath(Spec.AssetName));
			if (CardClass == nullptr)
			{
				UE_LOG(LogClassSelectWBPSetup, Warning, TEXT("ClassSelectWBPSetup: card class not found: %s"), Spec.AssetName);
				CardClass = LoadObject<UClass>(nullptr, CharacterCardClassPath);
			}
			if (CardClass == nullptr)
			{
				continue;
			}

			UCharacterCardWidget* CardWidget = SelectBP->WidgetTree->ConstructWidget<UCharacterCardWidget>(CardClass, CardName);
			if (CardWidget == nullptr)
			{
				continue;
			}

			CardWidget->Modify();
			CardWidget->SetVisibility(ESlateVisibility::Visible);
			CardWidget->SetRenderOpacity(1.0f);
			CardWidget->SetClipping(EWidgetClipping::Inherit);
			AddWidgetVariableGuid(SelectBP, CardName);

			UPanelSlot* PanelSlot = CardContainer->AddChild(CardWidget);
			const FCardSlotSnapshot* PreviousSnapshot = PreviousSlotSnapshots.Find(CardName);
			ApplyCardSlotSnapshot(PanelSlot, PreviousSnapshot != nullptr ? *PreviousSnapshot : FCardSlotSnapshot());
		}
	}

	void SetupCharacterSelectBlueprint(UWidgetBlueprint* SelectBP)
	{
		if (SelectBP == nullptr || SelectBP->WidgetTree == nullptr)
		{
			return;
		}

		UTexture2D* NavigationTexture = LoadTextureAsset(TEXT("classselect_navigation_button_darkfantasy_base"));
		ApplyButtonArt(Cast<UButton>(SelectBP->WidgetTree->FindWidget(TEXT("mBackToMainButton"))), NavigationTexture);
		ApplyButtonArt(Cast<UButton>(SelectBP->WidgetTree->FindWidget(TEXT("mConfirmButton"))), NavigationTexture);
		ApplyButtonText(Cast<UTextBlock>(SelectBP->WidgetTree->FindWidget(TEXT("mBackToMainButtonText"))), NSLOCTEXT("ClassSelectWBPSetup", "BackText", "뒤로가기"));
		ApplyButtonText(Cast<UTextBlock>(SelectBP->WidgetTree->FindWidget(TEXT("mConfirmButtonText"))), NSLOCTEXT("ClassSelectWBPSetup", "ConfirmText", "확인"));

		ApplyCanvasLayout(SelectBP->WidgetTree->FindWidget(TEXT("mSelectedCharacterPortraitImage")), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector, FVector2D(1920.0f, 1080.0f), FVector2D(0.5f, 0.5f), 1);
		ApplyCanvasLayout(SelectBP->WidgetTree->FindWidget(TEXT("mCharacterCardContainer")), FVector2D(0.5f, 1.0f), FVector2D(0.0f, -28.0f), FVector2D(504.0f, 80.0f), FVector2D(0.5f, 1.0f), 30);
		ApplyCanvasLayout(SelectBP->WidgetTree->FindWidget(TEXT("mBackToMainButton")), FVector2D(0.0f, 1.0f), FVector2D(64.0f, -42.0f), FVector2D(250.0f, 82.0f), FVector2D(0.0f, 1.0f), 31);
		ApplyCanvasLayout(SelectBP->WidgetTree->FindWidget(TEXT("mConfirmButton")), FVector2D(1.0f, 1.0f), FVector2D(-64.0f, -42.0f), FVector2D(250.0f, 82.0f), FVector2D(1.0f, 1.0f), 31);

		if (UWidget* CardContainer = SelectBP->WidgetTree->FindWidget(TEXT("mCharacterCardContainer")))
		{
			CardContainer->Modify();
			CardContainer->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			CardContainer->SetRenderTranslation(FVector2D::ZeroVector);
		}

		EnsureClassActionImages(SelectBP);
		AddDesignerCardsToSelectBlueprint(SelectBP);

		FBlueprintEditorUtils::MarkBlueprintAsModified(SelectBP);
		FKismetEditorUtilities::CompileBlueprint(SelectBP);
		SaveAssetObject(SelectBP);
	}

	void SetWidgetClippingToInherit(UWidget* Widget)
	{
		if (Widget == nullptr)
		{
			return;
		}

		Widget->Modify();
		Widget->SetClipping(EWidgetClipping::Inherit);
	}

	void SetWidgetRenderOpacity(UWidget* Widget, float Opacity)
	{
		if (Widget == nullptr)
		{
			return;
		}

		Widget->Modify();
		Widget->SetRenderOpacity(Opacity);
	}

	void SetWidgetVisibility(UWidget* Widget, ESlateVisibility Visibility)
	{
		if (Widget == nullptr)
		{
			return;
		}

		Widget->Modify();
		Widget->SetVisibility(Visibility);
	}

	void SetWidgetAndParentsVisible(UWidget* Widget)
	{
		UWidget* CurrentWidget = Widget;
		while (CurrentWidget != nullptr)
		{
			CurrentWidget->Modify();
			CurrentWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			CurrentWidget->SetRenderOpacity(1.0f);
			CurrentWidget->SetClipping(EWidgetClipping::Inherit);
			CurrentWidget = CurrentWidget->GetParent();
		}
	}

	void RaiseCanvasSlotZOrder(UWidget* Widget, int32 ZOrder)
	{
		if (Widget == nullptr)
		{
			return;
		}

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
		{
			CanvasSlot->Modify();
			CanvasSlot->SetZOrder(ZOrder);
		}
	}

	void FixClassSelectIconLayering(UWidgetBlueprint* SelectBP)
	{
		if (SelectBP == nullptr || SelectBP->WidgetTree == nullptr)
		{
			return;
		}

		SelectBP->Modify();
		SelectBP->WidgetTree->Modify();

		UWidget* CardsSizer = SelectBP->WidgetTree->FindWidget(TEXT("MobileCardsSizer"));
		UWidget* CardContainer = SelectBP->WidgetTree->FindWidget(TEXT("mCharacterCardContainer"));
		UWidget* DetailPanel = SelectBP->WidgetTree->FindWidget(TEXT("MobileDetailPanel"));
		UWidget* BackButton = SelectBP->WidgetTree->FindWidget(TEXT("mBackToMainButton"));
		UWidget* ConfirmButton = SelectBP->WidgetTree->FindWidget(TEXT("mConfirmButton"));

		/*
		 * 카드 위치는 디자이너가 잡은 값을 그대로 둔다.
		 * 전사 아이콘이 왼쪽 상세 패널과 겹칠 때도 보이도록 카드 묶음만 상세 패널보다 위 레이어로 올린다.
		 */
		RaiseCanvasSlotZOrder(DetailPanel, 20);
		RaiseCanvasSlotZOrder(CardsSizer, 45);
		RaiseCanvasSlotZOrder(CardContainer, 45);
		RaiseCanvasSlotZOrder(BackButton, 50);
		RaiseCanvasSlotZOrder(ConfirmButton, 50);

		ApplyCanvasLayout(CardsSizer, FVector2D(0.5f, 1.0f), FVector2D(0.0f, -118.0f), FVector2D(540.0f, 96.0f), FVector2D(0.5f, 1.0f), 45);
		if (USizeBox* CardsSizeBox = Cast<USizeBox>(CardsSizer))
		{
			CardsSizeBox->Modify();
			CardsSizeBox->SetWidthOverride(540.0f);
			CardsSizeBox->SetHeightOverride(96.0f);
		}

		SetWidgetAndParentsVisible(CardContainer);
		SetWidgetVisibility(CardsSizer, ESlateVisibility::SelfHitTestInvisible);
		SetWidgetVisibility(CardContainer, ESlateVisibility::SelfHitTestInvisible);

		const FName ClipSafeWidgetNames[] = {
			TEXT("MobileCardsSizer"),
			TEXT("mCharacterCardContainer"),
			TEXT("ClassSelectCard_Knight"),
			TEXT("ClassSelectCard_Rogue"),
			TEXT("ClassSelectCard_Mage"),
		};

		for (const FName& WidgetName : ClipSafeWidgetNames)
		{
			SetWidgetClippingToInherit(SelectBP->WidgetTree->FindWidget(WidgetName));
			SetWidgetRenderOpacity(SelectBP->WidgetTree->FindWidget(WidgetName), 1.0f);
		}

		FBlueprintEditorUtils::MarkBlueprintAsModified(SelectBP);
		FKismetEditorUtilities::CompileBlueprint(SelectBP);
		SaveAssetObject(SelectBP);
	}

	void ApplyCharacterCardIconDefaults(UWidgetBlueprint* CardBP)
	{
		if (CardBP == nullptr || CardBP->WidgetTree == nullptr)
		{
			return;
		}

		ApplyCharacterCardWBPVisuals(CardBP, LoadTextureAsset(TEXT("classselect_knight_symbol_button_1024x512")));
		EnsureClassCardVariantBlueprints(CardBP);
	}
#endif
}

UClassSelectWBPSetupCommandlet::UClassSelectWBPSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UClassSelectWBPSetupCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	ImportClassSelectTextures();

	UWidgetBlueprint* CardBP = LoadObject<UWidgetBlueprint>(nullptr, CharacterCardAssetPath);
	UWidgetBlueprint* SelectBP = LoadObject<UWidgetBlueprint>(nullptr, ClassSelectAssetPath);
	if (CardBP == nullptr || SelectBP == nullptr)
	{
		UE_LOG(LogClassSelectWBPSetup, Error, TEXT("ClassSelectWBPSetup: failed to load class select widget blueprints."));
		return 1;
	}

	SetupCharacterCardBlueprint(CardBP);
	SetupCharacterSelectBlueprint(SelectBP);
	UE_LOG(LogClassSelectWBPSetup, Display, TEXT("ClassSelectWBPSetup: WBP class select setup complete."));
	return 0;
#else
	UE_LOG(LogClassSelectWBPSetup, Error, TEXT("ClassSelectWBPSetup can only run in editor builds."));
	return 1;
#endif
}

UClassSelectIconVisibilityFixCommandlet::UClassSelectIconVisibilityFixCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UClassSelectIconVisibilityFixCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	UWidgetBlueprint* CardBP = LoadObject<UWidgetBlueprint>(nullptr, CharacterCardAssetPath);
	UWidgetBlueprint* SelectBP = LoadObject<UWidgetBlueprint>(nullptr, ClassSelectAssetPath);
	if (CardBP == nullptr || SelectBP == nullptr)
	{
		UE_LOG(LogClassSelectWBPSetup, Error, TEXT("ClassSelectIconVisibilityFix: failed to load class select widget blueprints."));
		return 1;
	}

	ApplyCharacterCardIconDefaults(CardBP);
	EnsureClassActionImages(SelectBP);
	AddDesignerCardsToSelectBlueprint(SelectBP);
	FixClassSelectIconLayering(SelectBP);
	DumpClassSelectCardContainer(SelectBP);
	UE_LOG(LogClassSelectWBPSetup, Display, TEXT("ClassSelectIconVisibilityFix: class select visuals moved to WBP variants."));
	return 0;
#else
	UE_LOG(LogClassSelectWBPSetup, Error, TEXT("ClassSelectIconVisibilityFix can only run in editor builds."));
	return 1;
#endif
}
