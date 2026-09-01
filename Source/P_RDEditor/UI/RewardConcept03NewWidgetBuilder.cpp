#include "UI/RewardConcept03NewWidgetBuilder.h"

#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/Paths.h"
#include "UI/UIFont.h"
#include "UI/Reward/RewardConcept03Widget.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditorUtils.h"
#include "WidgetBlueprintFactory.h"

namespace RewardConcept03NewWidgetBuilder
{
	constexpr TCHAR PackagePath[] = TEXT("/Game/UI/RewardConcept03New");
	constexpr TCHAR ArtPackagePath[] = TEXT("/Game/SVN/OutSideAsset/AICreation/UI/RewardConcept03New");
	constexpr TCHAR FourStepAssetName[] = TEXT("WBP_RewardConcept03_New");
	constexpr TCHAR FourStepAssetPath[] =
		TEXT("/Game/UI/RewardConcept03New/WBP_RewardConcept03_New.WBP_RewardConcept03_New");
	constexpr TCHAR ThreeStepAssetName[] = TEXT("WBP_RewardConcept03_New_NoArtifact");
	constexpr TCHAR ThreeStepAssetPath[] =
		TEXT("/Game/UI/RewardConcept03New/WBP_RewardConcept03_New_NoArtifact.WBP_RewardConcept03_New_NoArtifact");
	constexpr TCHAR FramelessAssetName[] = TEXT("WBP_RewardConcept03_Frameless");
	constexpr TCHAR FramelessAssetPath[] =
		TEXT("/Game/UI/RewardConcept03New/WBP_RewardConcept03_Frameless.WBP_RewardConcept03_Frameless");
	constexpr TCHAR FramelessThreeStepAssetName[] =
		TEXT("WBP_RewardConcept03_Frameless_NoArtifact");
	constexpr TCHAR FramelessThreeStepAssetPath[] =
		TEXT("/Game/UI/RewardConcept03New/WBP_RewardConcept03_Frameless_NoArtifact.WBP_RewardConcept03_Frameless_NoArtifact");
	constexpr TCHAR PortraitCellTexturePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Normal.T_KitA_Cell_Normal");
	TUniquePtr<FAutoConsoleCommand> BuildCommand;
	TUniquePtr<FAutoConsoleCommand> VerifyCommand;
	constexpr int32 ChestTripleBurstFrameCount = 33;

	const TCHAR* TextureNames[] = {
		TEXT("T_RCN_Header"), TEXT("T_RCN_MainFrame"), TEXT("T_RCN_Tab"),
		TEXT("T_RCN_Button"), TEXT("T_RCN_ParchmentLarge"),
		TEXT("T_RCN_ParchmentMedium"), TEXT("T_RCN_ChoiceCard"),
		TEXT("T_RCN_ProgressTrack"),
		TEXT("T_RCN_ProgressFilledLayerV3"),
		TEXT("T_RCN_RewardBackground"),
		TEXT("T_RCN_StepCircle"), TEXT("T_RCN_StepActiveRing"),
		TEXT("T_RCN_SelectionOutline"), TEXT("T_RCN_ChestClosed"),
		TEXT("T_RCN_ChestOpen25"), TEXT("T_RCN_ChestOpen50"),
		TEXT("T_RCN_ChestOpen75"), TEXT("T_RCN_ChestOpen"),
		TEXT("T_RCN_GoldCoin"), TEXT("T_RCN_ChestTripleBurst_Atlas")
	};
	const TCHAR* EffectTextureNames[] = {
		TEXT("T_RCN_GoldBurstGlow"), TEXT("T_RCN_GoldBurstRing"),
		TEXT("T_RCN_GoldBurstRays"), TEXT("T_RCN_GoldBurstSpark")
	};

	FString ChestTripleBurstTextureName(const int32 Index)
	{
		return FString::Printf(TEXT("T_RCN_ChestTripleBurst_%02d"), Index);
	}

	FString SourceDirectory()
	{
		return FPaths::Combine(FPaths::ProjectDir(), TEXT("SourceArt"),
			TEXT("UI"), TEXT("RewardConcept03New"));
	}

	FString TextureObjectPath(const TCHAR* Name)
	{
		return FString::Printf(TEXT("%s/%s.%s"), ArtPackagePath, Name, Name);
	}

	void SaveObject(UObject* Object)
	{
		check(Object != nullptr);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		checkf(UPackage::SavePackage(Object->GetPackage(), Object, *Filename,
			FSavePackageArgs()), TEXT("Could not save %s"), *Object->GetPathName());
	}

	void EnsureTextures()
	{
		TArray<FString> SourceFiles;
		TSet<FString> ImportedNames;
		auto QueueTextureImport = [&SourceFiles, &ImportedNames](
			const FString& Name, const FString& SourceFile)
		{
			const FString PackageName = FString::Printf(
				TEXT("%s/%s"), ArtPackagePath, *Name);
			const FString PackageFile = FPackageName::LongPackageNameToFilename(
				PackageName, FPackageName::GetAssetPackageExtension());
			IFileManager& FileManager = IFileManager::Get();
			const bool bMissingAsset = !FileManager.FileExists(*PackageFile);
			const bool bHasSource = FPaths::FileExists(SourceFile);
			// 커밋된 uasset만 있는 기존 그림은 그대로 쓸 수 있다. 새 그림을
			// 추가하거나 uasset이 없을 때만 원본 PNG를 필수로 요구한다.
			checkf(bHasSource || !bMissingAsset,
				TEXT("Missing RewardConcept03 texture source and asset: %s"),
				*SourceFile);
			if (!bHasSource)
			{
				return;
			}
			const bool bSourceIsNewer = !bMissingAsset
				&& FileManager.GetTimeStamp(*SourceFile)
					> FileManager.GetTimeStamp(*PackageFile);
			if (bMissingAsset || bSourceIsNewer)
			{
				SourceFiles.Add(SourceFile);
				ImportedNames.Add(Name);
			}
		};
		for (const TCHAR* Name : TextureNames)
		{
			const FString SourceFile = FPaths::Combine(
				SourceDirectory(), FString(Name) + TEXT(".png"));
			QueueTextureImport(Name, SourceFile);
		}
		// 런타임은 아틀라스를 재생하고 WBP에는 마지막 버스트 프레임 한 장만
		// 정지 미리보기로 굽는다. 삭제된 00..31 원본을 다시 요구하지 않는다.
		{
			const FString Name = ChestTripleBurstTextureName(
				ChestTripleBurstFrameCount - 1);
			const FString SourceFile = FPaths::Combine(SourceDirectory(),
				TEXT("ChestTripleBurstFrames"), Name + TEXT(".png"));
			QueueTextureImport(Name, SourceFile);
		}
		for (const TCHAR* Name : EffectTextureNames)
		{
			const FString SourceFile = FPaths::Combine(SourceDirectory(),
				TEXT("Effects"), FString(Name) + TEXT(".png"));
			QueueTextureImport(Name, SourceFile);
		}

		auto ImportTextures = [](const TArray<FString>& Files,
			const TCHAR* DestinationPath)
		{
			if (Files.IsEmpty())
			{
				return;
			}
			UE_LOG(LogTemp, Display,
				TEXT("RD_REWARD_CONCEPT03_TEXTURE_IMPORT begin count=%d"),
				Files.Num());
			UAutomatedAssetImportData* ImportData =
				NewObject<UAutomatedAssetImportData>();
			ImportData->DestinationPath = DestinationPath;
			ImportData->Filenames = Files;
			ImportData->bReplaceExisting = true;
			ImportData->bSkipReadOnly = true;
			FAssetToolsModule& AssetTools =
				FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			AssetTools.Get().ImportAssetsAutomated(ImportData);
			UE_LOG(LogTemp, Display,
				TEXT("RD_REWARD_CONCEPT03_TEXTURE_IMPORT complete count=%d"),
				Files.Num());
		};
		ImportTextures(SourceFiles, ArtPackagePath);

		auto ConfigureTexture = [&ImportedNames](const FString& Name)
		{
			UTexture2D* Texture = LoadObject<UTexture2D>(
				nullptr, *TextureObjectPath(*Name));
			checkf(Texture != nullptr,
				TEXT("Failed to import RewardConcept03 texture %s"), *Name);
			const bool bSettingsChanged = Texture->LODGroup != TEXTUREGROUP_UI
				|| Texture->MipGenSettings != TMGS_NoMipmaps
				|| Texture->CompressionSettings != TC_BC7
				|| !Texture->NeverStream || !Texture->SRGB;
			if (bSettingsChanged)
			{
				Texture->Modify();
				Texture->LODGroup = TEXTUREGROUP_UI;
				Texture->MipGenSettings = TMGS_NoMipmaps;
				Texture->CompressionSettings = TC_BC7;
				Texture->NeverStream = true;
				Texture->SRGB = true;
				Texture->PostEditChange();
			}
			if (bSettingsChanged || ImportedNames.Contains(Name))
			{
				SaveObject(Texture);
			}
		};
		for (const TCHAR* Name : TextureNames)
		{
			ConfigureTexture(Name);
		}
		ConfigureTexture(ChestTripleBurstTextureName(
			ChestTripleBurstFrameCount - 1));
		for (const TCHAR* Name : EffectTextureNames)
		{
			ConfigureTexture(Name);
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_REWARD_CONCEPT03_TEXTURES_READY imported=%d"),
			ImportedNames.Num());
	}

	UTexture2D* Texture(const TCHAR* Name)
	{
		UTexture2D* Result = LoadObject<UTexture2D>(
			nullptr, *TextureObjectPath(Name));
		checkf(Result != nullptr, TEXT("Missing new RewardConcept03 texture: %s"), Name);
		return Result;
	}

	FSlateBrush TextureBrush(UTexture2D* Source)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Source);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		const FIntPoint ImportedSize = Source->GetImportedSize();
		Brush.ImageSize = FVector2D(ImportedSize.X, ImportedSize.Y);
		return Brush;
	}

	FSlateBrush TextureRegionBrush(UTexture2D* Source,
		const FBox2f& UVRegion)
	{
		FSlateBrush Brush = TextureBrush(Source);
		Brush.SetUVRegion(UVRegion);
		return Brush;
	}

	FSlateBrush ProgressFillBrush(UTexture2D* Source)
	{
		// 생성된 채움 이미지의 금색 외곽 장식은 트랙 프레임과 중복된다.
		// 중앙 청색 영역만 UV로 잘라 트랙 내부에 맞추고, 퍼센트는 별도
		// Clip Canvas 폭으로 표현한다.
		return TextureRegionBrush(Source, FBox2f(
			FVector2f(.012f, .20f), FVector2f(.988f, .82f)));
	}

	FSlateBrush TextureBoxBrush(UTexture2D* Source,
		const FMargin Margin = FMargin(.12f))
	{
		FSlateBrush Brush = TextureBrush(Source);
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = Margin;
		return Brush;
	}

	void Place(UCanvasPanel* Parent, UWidget* Child, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder)
	{
		UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Child);
		Slot->SetAnchors(FAnchors(0.f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
	}

	void Anchor(UCanvasPanel* Parent, UWidget* Child, const FAnchors Anchors,
		const FMargin Offsets, const int32 ZOrder)
	{
		UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Child);
		Slot->SetAnchors(Anchors);
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetOffsets(Offsets);
		Slot->SetZOrder(ZOrder);
	}

	void StyleText(UTextBlock* Text, const int32 FontSize,
		const FLinearColor Color = FLinearColor::White)
	{
		FSlateFontInfo Font = UIFont::MakeProjectExact(Text->GetFont(), FontSize);
		Font.OutlineSettings.OutlineSize = FontSize >= 24 ? 1 : 0;
		Font.OutlineSettings.OutlineColor = FLinearColor::Black;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(1.5f, 1.5f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .75f));
		Text->SetJustification(ETextJustify::Center);
		Text->SetAutoWrapText(false);
		Text->SetMinDesiredWidth(0.f);
		Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	UCanvasPanel* AddCanvasPanel(UWidgetBlueprint* Blueprint,
		UCanvasPanel* Parent, const FName Name, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder)
	{
		UCanvasPanel* Panel = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), Name);
		Panel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Panel, Position, Size, ZOrder);
		return Panel;
	}

	UOverlay* AddOverlayPanel(UWidgetBlueprint* Blueprint,
		UCanvasPanel* Parent, const FName Name, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder)
	{
		UOverlay* Panel = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), Name);
		Panel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Panel, Position, Size, ZOrder);
		return Panel;
	}

	UOverlaySlot* AddOverlayChild(UOverlay* Parent, UWidget* Child,
		const FMargin Padding = FMargin(0.f),
		const EHorizontalAlignment HorizontalAlignment = HAlign_Fill,
		const EVerticalAlignment VerticalAlignment = VAlign_Fill)
	{
		UOverlaySlot* Slot = Parent->AddChildToOverlay(Child);
		Slot->SetPadding(Padding);
		Slot->SetHorizontalAlignment(HorizontalAlignment);
		Slot->SetVerticalAlignment(VerticalAlignment);
		return Slot;
	}

	UImage* AddOverlayImage(UWidgetBlueprint* Blueprint, UOverlay* Parent,
		const FName Name, UTexture2D* Source,
		const FLinearColor Tint = FLinearColor::White)
	{
		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), Name);
		Image->SetBrush(TextureBrush(Source));
		Image->SetColorAndOpacity(Tint);
		Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		AddOverlayChild(Parent, Image);
		return Image;
	}

	UImage* AddOverlayBoxImage(UWidgetBlueprint* Blueprint, UOverlay* Parent,
		const FName Name, UTexture2D* Source, const FMargin Margin,
		const FLinearColor Tint = FLinearColor::White)
	{
		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), Name);
		Image->SetBrush(TextureBoxBrush(Source, Margin));
		Image->SetColorAndOpacity(Tint);
		Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		AddOverlayChild(Parent, Image);
		return Image;
	}

	UBorder* AddOverlayBorder(UWidgetBlueprint* Blueprint, UOverlay* Parent,
		const FName Name, const FLinearColor Color)
	{
		UBorder* Border = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), Name);
		Border->SetBrushColor(Color);
		Border->SetPadding(FMargin(0.f));
		Border->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		AddOverlayChild(Parent, Border);
		return Border;
	}

	UTextBlock* AddOverlayText(UWidgetBlueprint* Blueprint, UOverlay* Parent,
		const FName Name, const FText& Value, const int32 FontSize,
		const FMargin Padding = FMargin(0.f),
		const FLinearColor Color = FLinearColor::White)
	{
		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), Name);
		Text->SetText(Value);
		StyleText(Text, FontSize, Color);
		AddOverlayChild(Parent, Text, Padding, HAlign_Fill, VAlign_Center);
		return Text;
	}

	UTextBlock* AddOverlayFittedText(UWidgetBlueprint* Blueprint,
		UOverlay* Parent, const FName Name, const FText& Value,
		const int32 FontSize, const FMargin Padding = FMargin(0.f),
		const FLinearColor Color = FLinearColor::White)
	{
		UScaleBox* Fit = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(),
			FName(*(Name.ToString() + TEXT("_Fit"))));
		Fit->SetStretch(EStretch::ScaleToFit);
		Fit->SetStretchDirection(EStretchDirection::DownOnly);
		Fit->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Fit->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		AddOverlayChild(Parent, Fit, Padding, HAlign_Fill, VAlign_Fill);

		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), Name);
		Text->SetText(Value);
		StyleText(Text, FontSize, Color);
		Fit->AddChild(Text);
		UScaleBoxSlot* TextSlot = CastChecked<UScaleBoxSlot>(Text->Slot);
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		return Text;
	}

	UImage* AddOverlayFittedImage(UWidgetBlueprint* Blueprint,
		UOverlay* Parent, const FName Name, UTexture2D* Source,
		const FMargin Padding = FMargin(0.f))
	{
		UScaleBox* Fit = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(),
			FName(*(Name.ToString() + TEXT("_Fit"))));
		Fit->SetStretch(EStretch::ScaleToFit);
		Fit->SetStretchDirection(EStretchDirection::DownOnly);
		Fit->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Fit->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		AddOverlayChild(Parent, Fit, Padding, HAlign_Fill, VAlign_Fill);

		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), Name);
		Image->SetBrush(TextureBrush(Source));
		Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Fit->AddChild(Image);
		UScaleBoxSlot* ImageSlot = CastChecked<UScaleBoxSlot>(Image->Slot);
		ImageSlot->SetHorizontalAlignment(HAlign_Center);
		ImageSlot->SetVerticalAlignment(VAlign_Center);
		return Image;
	}

	UImage* AddImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, UTexture2D* Source, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder,
		const FLinearColor Tint = FLinearColor::White)
	{
		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), Name);
		Image->SetBrush(TextureBrush(Source));
		Image->SetColorAndOpacity(Tint);
		Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Image, Position, Size, ZOrder);
		return Image;
	}

	UImage* AddBoxImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, UTexture2D* Source, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder, const FMargin Margin,
		const FLinearColor Tint = FLinearColor::White)
	{
		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), Name);
		Image->SetBrush(TextureBoxBrush(Source, Margin));
		Image->SetColorAndOpacity(Tint);
		Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Image, Position, Size, ZOrder);
		return Image;
	}

	UTextBlock* AddText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, const FText& Value, const int32 FontSize,
		const FVector2D Position, const FVector2D Size, const int32 ZOrder,
		const FLinearColor Color = FLinearColor::White)
	{
		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), Name);
		Text->SetText(Value);
		StyleText(Text, FontSize, Color);
		Place(Parent, Text, Position, Size, ZOrder);
		return Text;
	}

	UBorder* AddBorder(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, const FLinearColor Color, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder)
	{
		UBorder* Border = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), Name);
		Border->SetBrushColor(Color);
		Border->SetPadding(FMargin(0.f));
		Border->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Border, Position, Size, ZOrder);
		return Border;
	}

	UButton* AddInvisibleButton(UWidgetBlueprint* Blueprint,
		UCanvasPanel* Parent, const FName Name, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder)
	{
		UButton* Button = Blueprint->WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), Name);
		FSlateBrush InvisibleBrush;
		InvisibleBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
		FButtonStyle Style;
		Style.SetNormal(InvisibleBrush);
		Style.SetHovered(InvisibleBrush);
		Style.SetPressed(InvisibleBrush);
		Style.SetDisabled(InvisibleBrush);
		Button->SetStyle(Style);
		Button->SetBackgroundColor(FLinearColor::Transparent);
		Button->SetColorAndOpacity(FLinearColor::White);
		Place(Parent, Button, Position, Size, ZOrder);
		return Button;
	}

	UButton* AddOverlayInvisibleButton(UWidgetBlueprint* Blueprint,
		UOverlay* Parent, const FName Name)
	{
		UButton* Button = Blueprint->WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), Name);
		FSlateBrush InvisibleBrush;
		InvisibleBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
		FButtonStyle Style;
		Style.SetNormal(InvisibleBrush);
		Style.SetHovered(InvisibleBrush);
		Style.SetPressed(InvisibleBrush);
		Style.SetDisabled(InvisibleBrush);
		Button->SetStyle(Style);
		Button->SetBackgroundColor(FLinearColor::Transparent);
		Button->SetColorAndOpacity(FLinearColor::White);
		AddOverlayChild(Parent, Button);
		return Button;
	}

	UWidgetBlueprint* FindOrCreateBlueprint(const TCHAR* TargetAssetPath,
		const TCHAR* TargetAssetName, UClass* ParentClass)
	{
		if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(
			nullptr, TargetAssetPath))
		{
			return Existing;
		}
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->ParentClass = ParentClass;
		FAssetToolsModule& AssetTools =
			FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		return Cast<UWidgetBlueprint>(AssetTools.Get().CreateAsset(
			TargetAssetName, PackagePath, UWidgetBlueprint::StaticClass(), Factory));
	}

	UCanvasPanel* AddSwitcherCanvas(UWidgetBlueprint* Blueprint,
		UWidgetSwitcher* Switcher, const FName Name)
	{
		UCanvasPanel* Canvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), Name);
		Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Switcher->AddChild(Canvas);
		return Canvas;
	}

	UOverlay* AddSwitcherOverlay(UWidgetBlueprint* Blueprint,
		UWidgetSwitcher* Switcher, const FName Name)
	{
		UOverlay* Overlay = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), Name);
		Overlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Switcher->AddChild(Overlay);
		return Overlay;
	}

	void AddExperienceStep(UWidgetBlueprint* Blueprint, UCanvasPanel* Step,
		UTexture2D* Track, UTexture2D* Fill, UTexture2D* SummaryPanel,
		UTexture2D* PortraitCell,
		const TArray<UTexture2D*>& PreviewPortraits)
	{
		UCanvasPanel* Layout = AddCanvasPanel(Blueprint, Step,
			TEXT("NewExperienceLayout"), FVector2D::ZeroVector,
			FVector2D(1160.f, 390.f), 1);
		UCanvasPanel* List = AddCanvasPanel(Blueprint, Layout,
			TEXT("NewExperienceList"), FVector2D(20.f, 12.f),
			FVector2D(650.f, 360.f), 1);
		const FText Labels[] = {
			NSLOCTEXT("RewardConcept03", "MercPreview1", "용병 1"),
			NSLOCTEXT("RewardConcept03", "MercPreview2", "용병 2"),
			NSLOCTEXT("RewardConcept03", "MercPreview3", "용병 3") };
		const TCHAR* Progress[] = { TEXT("75 / 250"), TEXT("150 / 250"), TEXT("225 / 250") };
		const float PreviewPercents[] = { .30f, .60f, .90f };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			check(PreviewPortraits.IsValidIndex(Index)
				&& PreviewPortraits[Index] != nullptr && PortraitCell != nullptr);
			const float Y = 8.f + 112.f * Index;
			UCanvasPanel* Row = AddCanvasPanel(Blueprint, List,
				*FString::Printf(TEXT("NewExperienceRow_%d"), Index),
				FVector2D(0.f, Y), FVector2D(650.f, 94.f), 1);
			UOverlay* Portrait = AddOverlayPanel(Blueprint, Row,
				*FString::Printf(TEXT("NewPortraitZone_%d"), Index),
				FVector2D(10.f, 8.f), FVector2D(74.f, 74.f), 1);
			AddOverlayImage(Blueprint, Portrait,
				*FString::Printf(TEXT("NewPortraitCell_%d"), Index),
				PortraitCell);
			UOverlay* PortraitInner = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				*FString::Printf(TEXT("NewPortraitInner_%d"), Index));
			PortraitInner->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			PortraitInner->SetClipping(EWidgetClipping::ClipToBoundsAlways);
			AddOverlayChild(Portrait, PortraitInner, FMargin(7.f));

			UImage* PortraitImage = Blueprint->WidgetTree->ConstructWidget<UImage>(
				UImage::StaticClass(),
				*FString::Printf(TEXT("NewPortraitImage_%d"), Index));
			PortraitImage->SetBrush(TextureBrush(PreviewPortraits[Index]));
			PortraitImage->SetClipping(EWidgetClipping::ClipToBoundsAlways);
			PortraitImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			AddOverlayChild(PortraitInner, PortraitImage);

			UTextBlock* PortraitFallback = AddOverlayText(Blueprint, PortraitInner,
				*FString::Printf(TEXT("NewPortraitLabel_%d"), Index),
				Labels[Index], 16, FMargin(3.f),
				FLinearColor(.93f, .84f, .64f, 1.f));
			PortraitFallback->SetVisibility(ESlateVisibility::Collapsed);

			UOverlay* LevelZone = AddOverlayPanel(Blueprint, Row,
				*FString::Printf(TEXT("NewLevelZone_%d"), Index),
				FVector2D(92.f, 23.f), FVector2D(70.f, 44.f), 1);
			AddOverlayFittedText(Blueprint, LevelZone,
				*FString::Printf(TEXT("NewLevel_%d"), Index),
				FText::FromString(FString::Printf(TEXT("Lv.%d"), Index + 5)),
				18, FMargin(3.f), FLinearColor::White);

			UOverlay* ProgressZone = AddOverlayPanel(Blueprint, Row,
				*FString::Printf(TEXT("NewProgressZone_%d"), Index),
				FVector2D(156.f, 18.f), FVector2D(484.f, 56.f), 1);
			ProgressZone->SetClipping(EWidgetClipping::ClipToBounds);
			UCanvasPanel* BarLayer = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(),
				*FString::Printf(TEXT("NewExperienceBarLayer_%d"), Index));
			BarLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			AddOverlayChild(ProgressZone, BarLayer);
			constexpr float FullFillWidth = 484.f;
			constexpr float FillHeight = 36.f;
			constexpr float BarY = 10.f;
			AddImage(Blueprint, BarLayer,
				*FString::Printf(TEXT("NewTrack_%d"), Index), Track,
				FVector2D(0.f, BarY), FVector2D(FullFillWidth, FillHeight), 1);
			UCanvasPanel* FillClip = AddCanvasPanel(Blueprint, BarLayer,
				*FString::Printf(TEXT("NewFillClip_%d"), Index),
				FVector2D(0.f, BarY),
				FVector2D(FullFillWidth * PreviewPercents[Index],
					FillHeight), 2);
			FillClip->SetClipping(EWidgetClipping::ClipToBoundsAlways);
			AddImage(Blueprint, FillClip,
				*FString::Printf(TEXT("NewFill_%d"), Index), Fill,
				FVector2D::ZeroVector,
				FVector2D(FullFillWidth, FillHeight), 0);
			AddOverlayText(Blueprint, ProgressZone,
				*FString::Printf(TEXT("NewProgress_%d"), Index),
				FText::FromString(Progress[Index]), 19, FMargin(18.f, 4.f));

			UOverlay* LevelUpZone = AddOverlayPanel(Blueprint, Row,
				*FString::Printf(TEXT("NewLevelUpZone_%d"), Index),
				FVector2D(500.f, 64.f), FVector2D(136.f, 28.f), 3);
			UTextBlock* LevelUp = AddOverlayFittedText(Blueprint, LevelUpZone,
				*FString::Printf(TEXT("NewLevelUp_%d"), Index),
				NSLOCTEXT("RewardConcept03", "LevelUp", "레벨 업!"),
				18, FMargin(4.f, 1.f), FLinearColor(1.f, .78f, .18f, 1.f));
			LevelUp->SetVisibility(ESlateVisibility::Collapsed);
		}

		UOverlay* Summary = AddOverlayPanel(Blueprint, Layout,
			TEXT("NewExperienceSummaryPanel"), FVector2D(760.f, 48.f),
			FVector2D(330.f, 278.f), 2);
		AddOverlayImage(Blueprint, Summary,
			TEXT("NewExperienceSummaryArt"), SummaryPanel);
		UCanvasPanel* SummaryContent = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("NewExperienceSummaryContent"));
		SummaryContent->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		AddOverlayChild(Summary, SummaryContent, FMargin(30.f, 24.f));
		UOverlay* HeadingZone = AddOverlayPanel(Blueprint, SummaryContent,
			TEXT("NewExperienceSummaryHeadingZone"), FVector2D(0.f, 8.f),
			FVector2D(270.f, 50.f), 1);
		AddOverlayText(Blueprint, HeadingZone,
			TEXT("NewExperienceSummaryHeading"),
			NSLOCTEXT("RewardConcept03", "ExpHeading", "경험치 획득"), 22, FMargin(4.f),
			FLinearColor::White);
		UOverlay* RewardZone = AddOverlayPanel(Blueprint, SummaryContent,
			TEXT("NewExperienceRewardZone"), FVector2D(0.f, 92.f),
			FVector2D(270.f, 96.f), 2);
		AddOverlayText(Blueprint, RewardZone, TEXT("NewExperienceRewardText"),
			FText::FromString(TEXT("+50 XP")), 38, FMargin(4.f),
			FLinearColor::White);
	}

	void AddCenteredInfoStep(UWidgetBlueprint* Blueprint, UCanvasPanel* Step,
		UTexture2D* Panel, const FText& Heading, const FText& MainText,
		const FText& Hint, const FName Prefix,
		UTexture2D* ChestClosed = nullptr, UTexture2D* ChestOpen25 = nullptr,
		UTexture2D* ChestOpen50 = nullptr, UTexture2D* ChestOpen75 = nullptr,
		UTexture2D* ChestOpen = nullptr, UTexture2D* GoldCoin = nullptr,
		const bool bFramelessVisual = false)
	{
		const bool bIsChest = Prefix == TEXT("NewChest");
		UCanvasPanel* Layout = AddCanvasPanel(Blueprint, Step,
			FName(*(Prefix.ToString() + TEXT("Layout"))),
			FVector2D::ZeroVector, FVector2D(1160.f, 390.f), 1);

		// 시각 에셋, 상태 전환, 입력 영역을 분리해 런타임 교체와 연출을 허용한다.
		UOverlay* VisualPanel = AddOverlayPanel(Blueprint, Layout,
			FName(*(Prefix.ToString() + TEXT("VisualPanel"))),
			FVector2D(105.f, 55.f), FVector2D(360.f, 280.f), 1);
		if (!bFramelessVisual)
		{
			AddOverlayBorder(Blueprint, VisualPanel,
				FName(*(Prefix.ToString() + TEXT("VisualOuterRim"))),
				FLinearColor(.34f, .22f, .075f, 1.f));
			UOverlay* VisualInner = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				FName(*(Prefix.ToString() + TEXT("VisualInner"))));
			VisualInner->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			AddOverlayChild(VisualPanel, VisualInner, FMargin(4.f));
			AddOverlayBorder(Blueprint, VisualInner,
				FName(*(Prefix.ToString() + TEXT("VisualInnerFrame"))),
				FLinearColor(.075f, .065f, .045f, 1.f));
			UOverlay* VisualWell = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				FName(*(Prefix.ToString() + TEXT("VisualWell"))));
			VisualWell->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			AddOverlayChild(VisualInner, VisualWell, FMargin(12.f));
			AddOverlayBorder(Blueprint, VisualWell,
				FName(*(Prefix.ToString() + TEXT("VisualBackground"))),
				FLinearColor(.022f, .028f, .036f, 1.f));
			UCanvasPanel* VisualDecor = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(),
				FName(*(Prefix.ToString() + TEXT("VisualDecor"))));
			VisualDecor->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			AddOverlayChild(VisualPanel, VisualDecor, FMargin(18.f));
			AddBorder(Blueprint, VisualDecor,
				FName(*(Prefix.ToString() + TEXT("VisualTopHighlight"))),
				FLinearColor(.82f, .58f, .20f, .45f), FVector2D(0.f, 0.f),
				FVector2D(324.f, 2.f), 1);
			AddBorder(Blueprint, VisualDecor,
				FName(*(Prefix.ToString() + TEXT("VisualBottomShade"))),
				FLinearColor(0.f, 0.f, 0.f, .7f), FVector2D(0.f, 242.f),
				FVector2D(324.f, 2.f), 1);
		}
		if (bIsChest)
		{
			check(ChestClosed != nullptr && ChestOpen25 != nullptr
				&& ChestOpen50 != nullptr && ChestOpen75 != nullptr
				&& ChestOpen != nullptr);
			UWidgetSwitcher* ChestVisualSwitcher =
				Blueprint->WidgetTree->ConstructWidget<UWidgetSwitcher>(
					UWidgetSwitcher::StaticClass(), TEXT("NewChestVisualSwitcher"));
			ChestVisualSwitcher->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			AddOverlayChild(VisualPanel, ChestVisualSwitcher, FMargin(14.f, 10.f));
			UOverlay* ClosedState = AddSwitcherOverlay(Blueprint,
				ChestVisualSwitcher, TEXT("NewChestClosedState"));
			AddOverlayFittedImage(Blueprint, ClosedState,
				TEXT("NewChestClosedImage"), ChestClosed, FMargin(4.f));
			UOverlay* Open25State = AddSwitcherOverlay(Blueprint,
				ChestVisualSwitcher, TEXT("NewChestOpen25State"));
			AddOverlayFittedImage(Blueprint, Open25State,
				TEXT("NewChestOpen25Image"), ChestOpen25, FMargin(3.f));
			UOverlay* Open50State = AddSwitcherOverlay(Blueprint,
				ChestVisualSwitcher, TEXT("NewChestOpen50State"));
			AddOverlayFittedImage(Blueprint, Open50State,
				TEXT("NewChestOpen50Image"), ChestOpen50, FMargin(2.f));
			UOverlay* Open75State = AddSwitcherOverlay(Blueprint,
				ChestVisualSwitcher, TEXT("NewChestOpen75State"));
			AddOverlayFittedImage(Blueprint, Open75State,
				TEXT("NewChestOpen75Image"), ChestOpen75, FMargin(2.f));
			UOverlay* OpenState = AddSwitcherOverlay(Blueprint,
				ChestVisualSwitcher, TEXT("NewChestOpenState"));
			AddOverlayFittedImage(Blueprint, OpenState,
				TEXT("NewChestOpenImage"), ChestOpen, FMargin(2.f));
			ChestVisualSwitcher->SetActiveWidgetIndex(0);
			AddOverlayInvisibleButton(Blueprint, VisualPanel,
				TEXT("NewChestOpenButton"));
		}
		else
		{
			check(GoldCoin != nullptr);
			AddOverlayFittedImage(Blueprint, VisualPanel,
				TEXT("NewGoldCoinImage"), GoldCoin, FMargin(38.f));
		}

		UOverlay* InfoPanel = AddOverlayPanel(Blueprint, Layout,
			FName(*(Prefix.ToString() + TEXT("Panel"))),
			FVector2D(690.f, 55.f), FVector2D(360.f, 280.f), 2);
		AddOverlayImage(Blueprint, InfoPanel,
			FName(*(Prefix.ToString() + TEXT("PanelArt"))), Panel);
		UCanvasPanel* Content = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(),
			FName(*(Prefix.ToString() + TEXT("Content"))));
		Content->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		AddOverlayChild(InfoPanel, Content, FMargin(32.f, 24.f, 32.f, 28.f));

		UOverlay* HeadingZone = AddOverlayPanel(Blueprint, Content,
			FName(*(Prefix.ToString() + TEXT("HeadingZone"))),
			FVector2D(0.f, 6.f), FVector2D(296.f, 52.f), 1);
		AddOverlayText(Blueprint, HeadingZone,
			FName(*(Prefix.ToString() + TEXT("Heading"))),
			Heading, 25, FMargin(6.f),
			FLinearColor(.24f, .14f, .07f, 1.f));
		UOverlay* MainZone = AddOverlayPanel(Blueprint, Content,
			FName(*(Prefix.ToString() + TEXT("MainZone"))),
			FVector2D(0.f, 68.f), FVector2D(296.f, 82.f), 2);
		AddOverlayText(Blueprint, MainZone,
			FName(*(Prefix.ToString() + TEXT("Main"))),
			MainText, bIsChest ? 34 : 44, FMargin(4.f),
			FLinearColor(.16f, .095f, .045f, 1.f));
		UOverlay* HintZone = AddOverlayPanel(Blueprint, Content,
			FName(*(Prefix.ToString() + TEXT("HintZone"))),
			FVector2D(0.f, 166.f), FVector2D(296.f, 54.f), 3);
		UTextBlock* HintText = AddOverlayFittedText(Blueprint, HintZone,
			FName(*(Prefix.ToString() + TEXT("Hint"))),
			Hint, bIsChest ? 23 : 20, FMargin(8.f),
			FLinearColor(.28f, .18f, .10f, 1.f));
		HintText->SetAutoWrapText(false);
	}

	void AddFramelessChestStep(UWidgetBlueprint* Blueprint, UCanvasPanel* Step,
		UTexture2D* BurstAtlas, UTexture2D* GoldCoin,
		UTexture2D* BurstGlow, UTexture2D* BurstRing,
		UTexture2D* BurstRays, UTexture2D* BurstSpark)
	{
		check(BurstAtlas != nullptr);
		check(GoldCoin != nullptr && BurstGlow != nullptr && BurstRing != nullptr
			&& BurstRays != nullptr && BurstSpark != nullptr);
		UCanvasPanel* Layout = AddCanvasPanel(Blueprint, Step,
			TEXT("NewChestLayout"), FVector2D::ZeroVector,
			FVector2D(1160.f, 390.f), 1);
		// 33개 프레임을 한 장의 아틀라스에 상주시킨다. 모바일에서 개별
		// 텍스처 리소스를 교체하지 않고 UV만 이동해 검은 프레임/깜빡임을 막는다.
		UOverlay* VisualPanel = AddOverlayPanel(Blueprint, Layout,
			TEXT("NewChestVisualPanel"), FVector2D(250.f, 0.f),
			FVector2D(660.f, 390.f), 1);
		VisualPanel->SetClipping(EWidgetClipping::Inherit);
		auto AddAtlasLayer = [Blueprint, VisualPanel, BurstAtlas](
			const FName Name, const float Opacity)
		{
			UImage* Image = AddOverlayFittedImage(Blueprint, VisualPanel, Name,
				BurstAtlas);
			Image->SetClipping(EWidgetClipping::Inherit);
			if (UWidget* Fit = Image->GetParent())
			{
				Fit->SetClipping(EWidgetClipping::Inherit);
			}
			FSlateBrush Brush = Image->GetBrush();
			Brush.ImageSize = FVector2D(682.f, 455.f);
			Brush.SetUVRegion(FBox2f(FVector2f(0.f, 0.f),
				FVector2f(1.f / 6.f, 1.f / 6.f)));
			Image->SetBrush(Brush);
			Image->SetRenderOpacity(Opacity);
			Image->SetRenderTransformPivot(FVector2D(.5f, .5f));
			return Image;
		};
		AddAtlasLayer(TEXT("NewChestSequenceImage"), 1.f);
		AddAtlasLayer(TEXT("NewChestSequenceBlendImage"), 0.f);

		// 영상 위에 투명 UMG 이펙트를 겹쳐 사각 배경 없이 세 번의
		// 충격파가 독립적으로 확장되게 한다.
		for (int32 Wave = 0; Wave < 3; ++Wave)
		{
			const int32 BaseZ = 2 + Wave * 4;
			UImage* Glow = AddImage(Blueprint, Layout,
				*FString::Printf(TEXT("NewChestBurstGlow_%d"), Wave), BurstGlow,
				FVector2D(340.f, -70.f), FVector2D(480.f, 480.f), BaseZ);
			UImage* Ring = AddImage(Blueprint, Layout,
				*FString::Printf(TEXT("NewChestBurstRing_%d"), Wave), BurstRing,
				FVector2D(390.f, -20.f), FVector2D(380.f, 380.f), BaseZ + 1);
			UImage* Rays = AddImage(Blueprint, Layout,
				*FString::Printf(TEXT("NewChestBurstRays_%d"), Wave), BurstRays,
				FVector2D(370.f, -40.f), FVector2D(420.f, 420.f), BaseZ + 2);
			UImage* Spark = AddImage(Blueprint, Layout,
				*FString::Printf(TEXT("NewChestBurstSpark_%d"), Wave), BurstSpark,
				FVector2D(500.f, 90.f), FVector2D(160.f, 160.f), BaseZ + 3);
			for (UImage* Effect : { Glow, Ring, Rays, Spark })
			{
				Effect->SetRenderOpacity(0.f);
				Effect->SetVisibility(ESlateVisibility::Collapsed);
				Effect->SetRenderTransformPivot(FVector2D(.5f, .5f));
			}
		}

		// 네 개씩 세 웨이브. 영상 속 코인과 별개로 UI 전경에서 크게
		// 확대되므로 실제 화면 쪽으로 튀어나오는 깊이감을 만든다.
		for (int32 Index = 0; Index < 12; ++Index)
		{
			UImage* Coin = AddImage(Blueprint, Layout,
				*FString::Printf(TEXT("NewChestBurstForegroundCoin_%02d"), Index),
				GoldCoin, FVector2D(552.f, 178.f), FVector2D(56.f, 56.f),
				20 + Index);
			Coin->SetRenderOpacity(0.f);
			Coin->SetVisibility(ESlateVisibility::Collapsed);
			Coin->SetRenderTransformPivot(FVector2D(.5f, .5f));
		}
		AddOverlayInvisibleButton(Blueprint, VisualPanel,
			TEXT("NewChestOpenButton"));
	}

	void AddFramelessGoldStep(UWidgetBlueprint* Blueprint, UCanvasPanel* Step,
		UTexture2D* OpenChestWithGold, UTexture2D* GoldCoin)
	{
		check(OpenChestWithGold != nullptr && GoldCoin != nullptr);
		UCanvasPanel* Layout = AddCanvasPanel(Blueprint, Step,
			TEXT("NewGoldLayout"), FVector2D::ZeroVector,
			FVector2D(1160.f, 390.f), 1);

		// 마지막 개봉 프레임을 지우지 않고 화면 중앙의 배경으로 유지한다.
		// 다음 레이어인 BackgroundBlur가 이 상자만 흐리게 만든다.
		UOverlay* BackgroundPanel = AddOverlayPanel(Blueprint, Layout,
			TEXT("NewGoldBackgroundChestPanel"), FVector2D(250.f, 0.f),
			FVector2D(660.f, 390.f), 1);
		UImage* BackgroundChest = AddOverlayFittedImage(Blueprint,
			BackgroundPanel, TEXT("NewGoldBackgroundChestImage"),
			OpenChestWithGold);
		BackgroundChest->SetColorAndOpacity(
			FLinearColor(1.f, .94f, .80f, .88f));
		BackgroundChest->SetClipping(EWidgetClipping::ClipToBoundsAlways);

		UBackgroundBlur* ChestBlur =
			Blueprint->WidgetTree->ConstructWidget<UBackgroundBlur>(
				UBackgroundBlur::StaticClass(), TEXT("NewGoldChestBlur"));
		ChestBlur->SetBlurStrength(0.f);
		ChestBlur->SetVisibility(ESlateVisibility::Collapsed);
		Place(Layout, ChestBlur, FVector2D(250.f, 0.f),
			FVector2D(660.f, 390.f), 2);

		// 설명판 대신 보상 아이콘과 금액만 중앙 클러스터로 직접 노출한다.
		UOverlay* RewardCluster = AddOverlayPanel(Blueprint, Layout,
			TEXT("NewGoldVisualPanel"), FVector2D(420.f, 42.f),
			FVector2D(320.f, 306.f), 5);
		UCanvasPanel* RewardContent =
			Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(), TEXT("NewGoldRewardContent"));
		RewardContent->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		AddOverlayChild(RewardCluster, RewardContent);
		UOverlay* CoinZone = AddOverlayPanel(Blueprint, RewardContent,
			TEXT("NewGoldCoinZone"), FVector2D(74.f, 0.f),
			FVector2D(172.f, 172.f), 1);
		AddOverlayFittedImage(Blueprint, CoinZone,
			TEXT("NewGoldCoinImage"), GoldCoin, FMargin(5.f));
		UOverlay* AmountZone = AddOverlayPanel(Blueprint, RewardContent,
			TEXT("NewGoldMainZone"), FVector2D(0.f, 190.f),
			FVector2D(320.f, 92.f), 2);
		AddOverlayText(Blueprint, AmountZone, TEXT("NewGoldMain"),
			FText::FromString(TEXT("+350 G")), 52, FMargin(8.f, 2.f),
			FLinearColor(1.f, .82f, .22f, 1.f));
	}

	void AddChoiceStep(UWidgetBlueprint* Blueprint, UCanvasPanel* Step,
		UTexture2D* Card, UTexture2D* Selection)
	{
		UCanvasPanel* ChoiceList = AddCanvasPanel(Blueprint, Step,
			TEXT("NewArtifactChoiceList"), FVector2D::ZeroVector,
			FVector2D(1160.f, 390.f), 1);
		const FText Names[] = {
			NSLOCTEXT("RewardConcept03", "ArtifactPreview1", "피의 성배"),
			NSLOCTEXT("RewardConcept03", "ArtifactPreview2", "야수의 송곳니"),
			NSLOCTEXT("RewardConcept03", "ArtifactPreview3", "행운의 주화") };
		const float Xs[] = { 120.f, 450.f, 780.f };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			UOverlay* ChoicePanel = AddOverlayPanel(Blueprint, ChoiceList,
				*FString::Printf(TEXT("NewArtifactChoicePanel_%d"), Index),
				FVector2D(Xs[Index], 35.f), FVector2D(260.f, 315.f), 1);
			AddOverlayImage(Blueprint, ChoicePanel,
				*FString::Printf(TEXT("NewChoiceCard_%d"), Index), Card,
				FLinearColor::White);
			UCanvasPanel* ChoiceContent = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(),
				*FString::Printf(TEXT("NewChoiceContent_%d"), Index));
			ChoiceContent->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			AddOverlayChild(ChoicePanel, ChoiceContent, FMargin(14.f, 18.f));
			UOverlay* NameZone = AddOverlayPanel(Blueprint, ChoiceContent,
				*FString::Printf(TEXT("NewChoiceNameZone_%d"), Index),
				FVector2D(0.f, 18.f), FVector2D(232.f, 58.f), 1);
			AddOverlayFittedText(Blueprint, NameZone,
				*FString::Printf(TEXT("NewChoiceName_%d"), Index),
				Names[Index], 21, FMargin(5.f),
				FLinearColor(.21f, .12f, .06f, 1.f));
			UOverlay* TypeZone = AddOverlayPanel(Blueprint, ChoiceContent,
				*FString::Printf(TEXT("NewChoiceTypeZone_%d"), Index),
				FVector2D(16.f, 225.f), FVector2D(200.f, 40.f), 2);
			AddOverlayText(Blueprint, TypeZone,
				*FString::Printf(TEXT("NewChoiceType_%d"), Index),
				NSLOCTEXT("RewardConcept03", "TypeArtifact", "아티팩트"), 17, FMargin(3.f),
				FLinearColor(.35f, .21f, .10f, 1.f));
			AddOverlayInvisibleButton(Blueprint, ChoicePanel,
				*FString::Printf(TEXT("NewArtifactChoiceButton_%d"), Index));
		}
		AddImage(Blueprint, ChoiceList, TEXT("NewChoiceSelection"), Selection,
			FVector2D(425.f, 12.f), FVector2D(310.f, 360.f), 4);
	}

	void Build()
	{
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_CONCEPT03_NEW_BUILD begin"));
		EnsureTextures();
		UE_LOG(LogTemp, Display,
			TEXT("RD_REWARD_CONCEPT03_NEW_BUILD loading texture references"));
		UTexture2D* Header = Texture(TEXT("T_RCN_Header"));
		UTexture2D* MainFrame = Texture(TEXT("T_RCN_MainFrame"));
		UTexture2D* Tab = Texture(TEXT("T_RCN_Tab"));
		UTexture2D* Button = Texture(TEXT("T_RCN_Button"));
		UTexture2D* ParchmentLarge = Texture(TEXT("T_RCN_ParchmentLarge"));
		UTexture2D* ParchmentMedium = Texture(TEXT("T_RCN_ParchmentMedium"));
		UTexture2D* ChoiceCard = Texture(TEXT("T_RCN_ChoiceCard"));
		UTexture2D* StepTrack = Texture(TEXT("T_RCN_ProgressTrack"));
		UTexture2D* StepFill = Texture(TEXT("T_RCN_ProgressFilledLayerV3"));
		UTexture2D* RewardBackground = Texture(TEXT("T_RCN_RewardBackground"));
		UTexture2D* StepCircle = Texture(TEXT("T_RCN_StepCircle"));
		UTexture2D* ActiveRing = Texture(TEXT("T_RCN_StepActiveRing"));
		UTexture2D* Selection = Texture(TEXT("T_RCN_SelectionOutline"));
		UTexture2D* ChestClosed = Texture(TEXT("T_RCN_ChestClosed"));
		UTexture2D* ChestOpen25 = Texture(TEXT("T_RCN_ChestOpen25"));
		UTexture2D* ChestOpen50 = Texture(TEXT("T_RCN_ChestOpen50"));
		UTexture2D* ChestOpen75 = Texture(TEXT("T_RCN_ChestOpen75"));
		UTexture2D* ChestOpen = Texture(TEXT("T_RCN_ChestOpen"));
		UTexture2D* GoldCoin = Texture(TEXT("T_RCN_GoldCoin"));
		UTexture2D* GoldBurstGlow = Texture(TEXT("T_RCN_GoldBurstGlow"));
		UTexture2D* GoldBurstRing = Texture(TEXT("T_RCN_GoldBurstRing"));
		UTexture2D* GoldBurstRays = Texture(TEXT("T_RCN_GoldBurstRays"));
		UTexture2D* GoldBurstSpark = Texture(TEXT("T_RCN_GoldBurstSpark"));
		UTexture2D* ChestTripleBurstAtlas = Texture(
			TEXT("T_RCN_ChestTripleBurst_Atlas"));
		UTexture2D* PortraitCell = LoadObject<UTexture2D>(
			nullptr, PortraitCellTexturePath);
		const TCHAR* PreviewPortraitPaths[] = {
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Ranger.T_MB_HireIcon_Ranger"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage")
		};
		TArray<UTexture2D*> PreviewPortraits;
		PreviewPortraits.Reserve(UE_ARRAY_COUNT(PreviewPortraitPaths));
		for (const TCHAR* PreviewPortraitPath : PreviewPortraitPaths)
		{
			PreviewPortraits.Add(LoadObject<UTexture2D>(nullptr,
				PreviewPortraitPath));
		}
		checkf(PortraitCell != nullptr
			&& !PreviewPortraits.Contains(nullptr),
			TEXT("Missing shared KitA portrait cell or preview portraits"));
		TArray<UTexture2D*> ChestTripleBurstFrames;
		ChestTripleBurstFrames.Add(Texture(*ChestTripleBurstTextureName(
			ChestTripleBurstFrameCount - 1)));
		UE_LOG(LogTemp, Display,
			TEXT("RD_REWARD_CONCEPT03_NEW_BUILD texture references ready"));

		auto BuildVariant = [&](const TCHAR* TargetAssetPath,
			const TCHAR* TargetAssetName, UClass* ParentClass,
			const bool bHasArtifact, const bool bFrameless)
		{
		const int32 RewardStepCount = bHasArtifact ? 4 : 3;
		UWidgetBlueprint* Blueprint = FindOrCreateBlueprint(
			TargetAssetPath, TargetAssetName, ParentClass);
		check(Blueprint != nullptr && Blueprint->WidgetTree != nullptr);
		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		if (UWidget* PreviousRoot = Blueprint->WidgetTree->RootWidget)
		{
			TSet<UWidget*> PreviousWidgets;
			PreviousWidgets.Add(PreviousRoot);
			FWidgetBlueprintEditorUtils::DeleteWidgets(Blueprint,
				MoveTemp(PreviousWidgets),
				FWidgetBlueprintEditorUtils::EDeleteWidgetWarningType::DeleteSilently);
		}
		Blueprint->ParentClass = ParentClass;

		UOverlay* Root = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("NewRewardConceptRoot"));
		Blueprint->WidgetTree->RootWidget = Root;
		UCanvasPanel* Viewport = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("NewRewardViewportCanvas"));
		Root->AddChildToOverlay(Viewport);
		CastChecked<UOverlaySlot>(Viewport->Slot)->SetHorizontalAlignment(HAlign_Fill);
		CastChecked<UOverlaySlot>(Viewport->Slot)->SetVerticalAlignment(VAlign_Fill);
		UImage* RewardBackgroundImage =
			Blueprint->WidgetTree->ConstructWidget<UImage>(
				UImage::StaticClass(), TEXT("NewRewardBackgroundImage"));
		RewardBackgroundImage->SetBrush(TextureBrush(RewardBackground));
		RewardBackgroundImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Anchor(Viewport, RewardBackgroundImage, FAnchors(0.f, 0.f, 1.f, 1.f),
			FMargin(0.f), 0);

		UBorder* ModalScrim = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("NewRewardModalScrim"));
		ModalScrim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, .58f));
		ModalScrim->SetPadding(FMargin(0.f));
		ModalScrim->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Anchor(Viewport, ModalScrim, FAnchors(0.f, 0.f, 1.f, 1.f),
			FMargin(0.f), 1);

		UScaleBox* MasterScale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("NewRewardMasterScale"));
		MasterScale->SetStretch(EStretch::ScaleToFit);
		MasterScale->SetStretchDirection(EStretchDirection::Both);
		MasterScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Anchor(Viewport, MasterScale, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f), 2);

		USizeBox* DesignSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("NewRewardDesignSize"));
		DesignSize->SetWidthOverride(1600.f);
		DesignSize->SetHeightOverride(900.f);
		MasterScale->AddChild(DesignSize);
		UCanvasPanel* Design = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("NewRewardDesignCanvas"));
		DesignSize->SetContent(Design);

		// CombatHUD와 같은 기능 단위 계층: 패널 안에서 아트는 Fill,
		// 텍스트는 패딩된 중앙 영역, 입력은 가장 위의 투명 Button으로 둔다.
		UOverlay* HeaderPanel = AddOverlayPanel(Blueprint, Design,
			TEXT("NewRewardHeaderPanel"), FVector2D(510.f, 20.f),
			FVector2D(580.f, 116.f), 20);
		AddOverlayImage(Blueprint, HeaderPanel, TEXT("NewHeaderArt"), Header);
		AddOverlayText(Blueprint, HeaderPanel, TEXT("NewTitleText"),
			NSLOCTEXT("RewardConcept03", "Title", "전투 보상"), 43,
			FMargin(82.f, 13.f, 82.f, 19.f),
			FLinearColor(.96f, .90f, .74f, 1.f));

		UCanvasPanel* ProgressPanel = AddCanvasPanel(Blueprint, Design,
			TEXT("NewRewardProgressPanel"), FVector2D(350.f, 115.f),
			FVector2D(900.f, 92.f), 18);
		AddImage(Blueprint, ProgressPanel, TEXT("NewProgressTrackArt"), StepTrack,
			FVector2D(0.f, 18.f), FVector2D(900.f, 54.f), 1);
		// 실제 렌더 캡처에서 양끝 금속 마감과 단계 원이 충돌하지 않도록
		// 원의 중심을 안쪽으로 모은다.
		const TArray<float> StepCenters = bHasArtifact
			? TArray<float>{ 120.f, 340.f, 560.f, 780.f }
			: TArray<float>{ 166.f, 450.f, 734.f };

		UWidgetSwitcher* ProgressSwitcher =
			Blueprint->WidgetTree->ConstructWidget<UWidgetSwitcher>(
				UWidgetSwitcher::StaticClass(), TEXT("NewRewardProgressSwitcher"));
		Place(ProgressPanel, ProgressSwitcher, FVector2D::ZeroVector,
			FVector2D(900.f, 92.f), 2);
		AddSwitcherCanvas(Blueprint, ProgressSwitcher, TEXT("NewProgressWarmupState"));
		for (int32 Index = 0; Index < RewardStepCount; ++Index)
		{
			UCanvasPanel* State = AddSwitcherCanvas(Blueprint, ProgressSwitcher,
				*FString::Printf(TEXT("NewProgressState_%d"), Index + 1));
			// 빈 바와 완전히 채운 바는 동일한 원본 크기와 슬롯을 공유한다.
			// 채운 바 전체를 가로로 잘라 단계 원 아래에서 끝내므로, 별도 UV
			// 보정이나 추정 패딩 없이 두 프레임이 픽셀 단위로 겹친다.
			constexpr float ProgressFillStartX = 0.f;
			constexpr float ProgressFillY = 18.f;
			constexpr float ProgressFillHeight = 54.f;
			constexpr float FullProgressFillWidth = 900.f;
			const float VisibleProgressFillWidth = Index == RewardStepCount - 1
				? FullProgressFillWidth : StepCenters[Index];
			UCanvasPanel* ProgressFillClip = AddCanvasPanel(Blueprint, State,
				*FString::Printf(TEXT("NewCompletedProgressClip_%d"), Index + 1),
				FVector2D(ProgressFillStartX, ProgressFillY),
				FVector2D(VisibleProgressFillWidth, ProgressFillHeight), 0);
			ProgressFillClip->SetClipping(EWidgetClipping::ClipToBoundsAlways);
			AddImage(Blueprint, ProgressFillClip,
				*FString::Printf(TEXT("NewCompletedProgress_%d"), Index + 1),
				StepFill, FVector2D::ZeroVector,
				FVector2D(FullProgressFillWidth, ProgressFillHeight), 0);
			AddImage(Blueprint, State,
				*FString::Printf(TEXT("NewActiveRing_%d"), Index + 1),
				ActiveRing, FVector2D(StepCenters[Index] - 46.f, 0.f),
				FVector2D(92.f, 92.f), 1);
		}
		ProgressSwitcher->SetActiveWidgetIndex(1);

		for (int32 Index = 0; Index < RewardStepCount; ++Index)
		{
			AddImage(Blueprint, ProgressPanel,
				*FString::Printf(TEXT("NewStepCircle_%d"), Index + 1),
				StepCircle, FVector2D(StepCenters[Index] - 36.f, 10.f),
				FVector2D(72.f, 72.f), 3);
			UOverlay* NumberZone = AddOverlayPanel(Blueprint, ProgressPanel,
				*FString::Printf(TEXT("NewStepNumberZone_%d"), Index + 1),
				FVector2D(StepCenters[Index] - 36.f, 10.f),
				FVector2D(72.f, 72.f), 4);
			AddOverlayText(Blueprint, NumberZone,
				*FString::Printf(TEXT("NewStepNumber_%d"), Index + 1),
				FText::AsNumber(Index + 1), 27, FMargin(0.f, 3.f, 0.f, 8.f));
		}

		UCanvasPanel* FramePanel = AddCanvasPanel(Blueprint, Design,
			TEXT("NewRewardFramePanel"), FVector2D(140.f, 198.f),
			FVector2D(1320.f, 600.f), 9);
		if (!bFrameless)
		{
			// 프레임 내부 배경은 프레임 패널 자체를 0-offset으로 완전히 채운다.
			AddBorder(Blueprint, FramePanel, TEXT("NewPanelSurfaceBase"),
				FLinearColor(.018f, .022f, .028f, 1.f), FVector2D::ZeroVector,
				FVector2D(1320.f, 600.f), 0);
			AddImage(Blueprint, FramePanel, TEXT("NewPanelParchment"), ParchmentLarge,
				FVector2D::ZeroVector, FVector2D(1320.f, 600.f), 1,
				FLinearColor(.22f, .24f, .27f, .34f));
			AddBorder(Blueprint, FramePanel, TEXT("NewPanelSurfaceWash"),
				FLinearColor(.012f, .016f, .021f, .24f), FVector2D::ZeroVector,
				FVector2D(1320.f, 600.f), 2);
			AddImage(Blueprint, FramePanel, TEXT("NewMainFrameArt"), MainFrame,
				FVector2D::ZeroVector, FVector2D(1320.f, 600.f), 10);
		}

		if (!bFrameless)
		{
			UOverlay* TabPanel = AddOverlayPanel(Blueprint, FramePanel,
				TEXT("NewRewardTabPanel"), FVector2D(-6.f, 0.f),
				FVector2D(200.f, 76.f), 20);
			AddOverlayImage(Blueprint, TabPanel, TEXT("NewTabArt"), Tab);
			UWidgetSwitcher* TabSwitcher =
				Blueprint->WidgetTree->ConstructWidget<UWidgetSwitcher>(
					UWidgetSwitcher::StaticClass(), TEXT("NewRewardTabSwitcher"));
			AddOverlayChild(TabPanel, TabSwitcher, FMargin(14.f, 11.f, 14.f, 17.f));
			AddSwitcherOverlay(Blueprint, TabSwitcher, TEXT("NewTabWarmupState"));
			const FText TabNames[] = {
				NSLOCTEXT("RewardConcept03", "TabExp", "경험치"),
				NSLOCTEXT("RewardConcept03", "TabChest", "상자"),
				NSLOCTEXT("RewardConcept03", "TabGold", "골드"),
				NSLOCTEXT("RewardConcept03", "TabArtifact", "아티팩트") };
			for (int32 Index = 0; Index < RewardStepCount; ++Index)
			{
				UOverlay* State = AddSwitcherOverlay(Blueprint, TabSwitcher,
					*FString::Printf(TEXT("NewTabState_%d"), Index + 1));
				AddOverlayFittedText(Blueprint, State,
					*FString::Printf(TEXT("NewTabText_%d"), Index + 1),
					TabNames[Index], 22, FMargin(3.f),
					FLinearColor(.96f, .90f, .74f, 1.f));
			}
			TabSwitcher->SetActiveWidgetIndex(1);
		}

		UWidgetSwitcher* StepSwitcher =
			Blueprint->WidgetTree->ConstructWidget<UWidgetSwitcher>(
				UWidgetSwitcher::StaticClass(), TEXT("NewRewardStepSwitcher"));
		Place(FramePanel, StepSwitcher, FVector2D(80.f, 88.f),
			FVector2D(1160.f, 390.f), 5);
		AddSwitcherCanvas(Blueprint, StepSwitcher, TEXT("NewStepWarmupState"));
		UCanvasPanel* Step1 = AddSwitcherCanvas(Blueprint, StepSwitcher,
			TEXT("NewExperienceStep"));
		UCanvasPanel* Step2 = AddSwitcherCanvas(Blueprint, StepSwitcher,
			TEXT("NewChestStep"));
		UCanvasPanel* Step3 = AddSwitcherCanvas(Blueprint, StepSwitcher,
			TEXT("NewGoldStep"));
		AddExperienceStep(Blueprint, Step1, StepTrack, StepFill, ParchmentMedium,
			PortraitCell, PreviewPortraits);
		if (bFrameless)
		{
			AddFramelessChestStep(Blueprint, Step2, ChestTripleBurstAtlas,
				GoldCoin, GoldBurstGlow, GoldBurstRing,
				GoldBurstRays, GoldBurstSpark);
		}
		else
		{
			AddCenteredInfoStep(Blueprint, Step2, ParchmentMedium,
				NSLOCTEXT("RewardConcept03", "ChestHeading", "보상 상자"),
				NSLOCTEXT("RewardConcept03", "ChestOpenLabel", "상자 열기"),
				NSLOCTEXT("RewardConcept03", "ChestHint", "상자를 눌러 여세요"),
				TEXT("NewChest"), ChestClosed, ChestOpen25, ChestOpen50,
				ChestOpen75, ChestOpen);
		}
		if (bFrameless)
		{
			AddFramelessGoldStep(Blueprint, Step3,
				ChestTripleBurstFrames.Last(), GoldCoin);
		}
		else
		{
			AddCenteredInfoStep(Blueprint, Step3, ParchmentMedium,
				NSLOCTEXT("RewardConcept03", "GoldHeading", "획득 골드"),
				FText::FromString(TEXT("+350 G")),
				NSLOCTEXT("RewardConcept03", "GoldGrantedHint", "보상이 지급되었습니다"),
				TEXT("NewGold"), nullptr, nullptr, nullptr, nullptr, nullptr,
				GoldCoin, false);
		}
		if (bHasArtifact)
		{
			UCanvasPanel* Step4 = AddSwitcherCanvas(Blueprint, StepSwitcher,
				TEXT("NewArtifactStep"));
			AddChoiceStep(Blueprint, Step4, ChoiceCard, Selection);
		}
		StepSwitcher->SetActiveWidgetIndex(1);

		UBorder* PresentationFlash = AddBorder(Blueprint, FramePanel,
			TEXT("NewRewardPresentationFlash"),
			FLinearColor(1.f, .63f, .12f, .75f), FVector2D(80.f, 88.f),
			FVector2D(1160.f, 390.f), 25);
		PresentationFlash->SetRenderOpacity(0.f);

		UOverlay* BottomButtonPanel = AddOverlayPanel(Blueprint, FramePanel,
			TEXT("NewBottomButtonPanel"), FVector2D(474.f, 530.f),
			FVector2D(372.f, 132.f), 30);
		AddOverlayImage(Blueprint, BottomButtonPanel,
			TEXT("NewBottomButtonArt"), Button);
		UWidgetSwitcher* ButtonSwitcher =
			Blueprint->WidgetTree->ConstructWidget<UWidgetSwitcher>(
				UWidgetSwitcher::StaticClass(), TEXT("NewRewardButtonSwitcher"));
		AddOverlayChild(BottomButtonPanel, ButtonSwitcher,
			FMargin(36.f, 27.f, 36.f, 40.f));
		AddSwitcherOverlay(Blueprint, ButtonSwitcher, TEXT("NewButtonWarmupState"));
		for (int32 Index = 0; Index < RewardStepCount; ++Index)
		{
			UOverlay* State = AddSwitcherOverlay(Blueprint, ButtonSwitcher,
				*FString::Printf(TEXT("NewButtonState_%d"), Index + 1));
			AddOverlayFittedText(Blueprint, State,
				*FString::Printf(TEXT("NewButtonText_%d"), Index + 1),
				Index == RewardStepCount - 1
					? NSLOCTEXT("RewardConcept03", "Confirm", "확정")
					: NSLOCTEXT("RewardConcept03", "Next", "다음"), 31,
				FMargin(4.f), FLinearColor(.96f, .90f, .74f, 1.f));
		}
		ButtonSwitcher->SetActiveWidgetIndex(1);
		AddOverlayInvisibleButton(Blueprint, BottomButtonPanel,
			TEXT("NewBottomActionButton"));

		Blueprint->WidgetTree->ForEachWidget([Blueprint](UWidget* Widget)
		{
			if (Widget != nullptr
				&& Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()) == false)
			{
				Blueprint->OnVariableAdded(Widget->GetFName());
			}
		});
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		SaveObject(Blueprint);
		UE_LOG(LogTemp, Display,
			TEXT("RD_REWARD_CONCEPT03_NEW_BUILD success asset=%s steps=%d frameless=%s background=none portrait_cell=T_KitA_Cell_Normal"),
			TargetAssetPath, RewardStepCount, bFrameless ? TEXT("true") : TEXT("false"));
		};

		BuildVariant(FourStepAssetPath, FourStepAssetName,
			URewardConcept03Widget::StaticClass(), true, false);
		BuildVariant(ThreeStepAssetPath, ThreeStepAssetName,
			URewardConcept03NoArtifactWidget::StaticClass(), false, false);
		BuildVariant(FramelessAssetPath, FramelessAssetName,
			URewardConcept03Widget::StaticClass(), true, true);
		BuildVariant(FramelessThreeStepAssetPath, FramelessThreeStepAssetName,
			URewardConcept03NoArtifactWidget::StaticClass(), false, true);
	}

	void VerifyVariant(const TCHAR* TargetAssetPath, UClass* ParentClass,
		const bool bHasArtifact)
	{
		const int32 RewardStepCount = bHasArtifact ? 4 : 3;
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(
			nullptr, TargetAssetPath);
		checkf(Blueprint != nullptr && Blueprint->WidgetTree != nullptr,
			TEXT("New RewardConcept03 WBP is missing"));
		checkf(Blueprint->ParentClass == ParentClass,
			TEXT("New RewardConcept03 WBP must use its independent runtime class"));
		for (const TCHAR* Name : {
			TEXT("NewRewardConceptRoot"), TEXT("NewRewardMasterScale"),
			TEXT("NewRewardHeaderPanel"), TEXT("NewRewardProgressPanel"),
			TEXT("NewRewardFramePanel"), TEXT("NewRewardTabPanel"),
			TEXT("NewBottomButtonPanel"),
			TEXT("NewRewardStepSwitcher"), TEXT("NewRewardProgressSwitcher"),
			TEXT("NewRewardTabSwitcher"), TEXT("NewRewardButtonSwitcher"),
			TEXT("NewBottomActionButton"), TEXT("NewChestOpenButton"),
			TEXT("NewRewardPresentationFlash"),
			TEXT("NewChestVisualPanel"), TEXT("NewChestPanel"),
			TEXT("NewChestVisualSwitcher"),
			TEXT("NewChestClosedState"), TEXT("NewChestOpen25State"),
			TEXT("NewChestOpen50State"), TEXT("NewChestOpen75State"),
			TEXT("NewChestOpenState"), TEXT("NewGoldVisualPanel"),
			TEXT("NewGoldCoinImage"), TEXT("NewGoldPanel"),
			TEXT("NewPortraitCell_0"), TEXT("NewPortraitInner_0"),
			TEXT("NewPortraitImage_0"), TEXT("NewLevelUp_0"),
			TEXT("NewExperienceStep"), TEXT("NewChestStep"),
			TEXT("NewGoldStep") })
		{
			checkf(Blueprint->WidgetTree->FindWidget(FName(Name)) != nullptr,
				TEXT("New RewardConcept03 WBP missing widget %s"), Name);
		}
		for (const TCHAR* Name : {
			TEXT("NewArtifactChoiceButton_0"),
			TEXT("NewArtifactChoiceButton_1"),
			TEXT("NewArtifactChoiceButton_2"), TEXT("NewArtifactStep") })
		{
			checkf((Blueprint->WidgetTree->FindWidget(FName(Name)) != nullptr)
				== bHasArtifact,
				TEXT("Artifact widget presence mismatch for %s in %s"),
				Name, TargetAssetPath);
		}
		auto CheckParent = [Blueprint](const TCHAR* ChildName,
			const TCHAR* ParentName)
		{
			UWidget* Child = Blueprint->WidgetTree->FindWidget(FName(ChildName));
			checkf(Child != nullptr && Child->GetParent() != nullptr
				&& Child->GetParent()->GetFName() == FName(ParentName),
				TEXT("Reward widget %s must be contained by functional panel %s"),
				ChildName, ParentName);
		};
		CheckParent(TEXT("NewTitleText"), TEXT("NewRewardHeaderPanel"));
		CheckParent(TEXT("NewRewardProgressSwitcher"), TEXT("NewRewardProgressPanel"));
		CheckParent(TEXT("NewPanelParchment"), TEXT("NewRewardFramePanel"));
		CheckParent(TEXT("NewRewardStepSwitcher"), TEXT("NewRewardFramePanel"));
		CheckParent(TEXT("NewRewardTabSwitcher"), TEXT("NewRewardTabPanel"));
		CheckParent(TEXT("NewBottomActionButton"), TEXT("NewBottomButtonPanel"));
		CheckParent(TEXT("NewProgress_0"), TEXT("NewProgressZone_0"));
		CheckParent(TEXT("NewLevelUp_0"), TEXT("NewLevelUp_0_Fit"));
		CheckParent(TEXT("NewPortraitImage_0"), TEXT("NewPortraitInner_0"));
		CheckParent(TEXT("NewExperienceRewardText"),
			TEXT("NewExperienceRewardZone"));
		CheckParent(TEXT("NewChestMain"), TEXT("NewChestMainZone"));
		CheckParent(TEXT("NewChestOpenButton"), TEXT("NewChestVisualPanel"));
		CheckParent(TEXT("NewChestVisualSwitcher"), TEXT("NewChestVisualPanel"));
		if (bHasArtifact)
		{
			CheckParent(TEXT("NewChoiceName_0"), TEXT("NewChoiceName_0_Fit"));
		}
		auto CheckCenteredOverlayText = [Blueprint](const TCHAR* TextName)
		{
			UTextBlock* Text = Cast<UTextBlock>(
				Blueprint->WidgetTree->FindWidget(FName(TextName)));
			UOverlaySlot* OverlaySlot = Text != nullptr
				? Cast<UOverlaySlot>(Text->Slot) : nullptr;
			UScaleBoxSlot* ScaleSlot = Text != nullptr
				? Cast<UScaleBoxSlot>(Text->Slot) : nullptr;
			const bool bOverlayCentered = OverlaySlot != nullptr
				&& OverlaySlot->GetHorizontalAlignment() == HAlign_Fill
				&& OverlaySlot->GetVerticalAlignment() == VAlign_Center;
			const bool bScaleCentered = ScaleSlot != nullptr
				&& ScaleSlot->GetHorizontalAlignment() == HAlign_Center
				&& ScaleSlot->GetVerticalAlignment() == VAlign_Center;
			checkf(bOverlayCentered || bScaleCentered,
				TEXT("Reward text %s must use a fill/center Overlay slot"), TextName);
		};
		for (const TCHAR* TextName : {
			TEXT("NewTitleText"), TEXT("NewTabText_1"),
			TEXT("NewButtonText_1"), TEXT("NewProgress_0"),
			TEXT("NewExperienceRewardText"), TEXT("NewChestMain"),
			TEXT("NewChestHint"), TEXT("NewGoldMain") })
		{
			CheckCenteredOverlayText(TextName);
		}
		if (bHasArtifact)
		{
			CheckCenteredOverlayText(TEXT("NewChoiceName_1"));
		}
		else
		{
			CheckCenteredOverlayText(TEXT("NewButtonText_3"));
		}
		UWidgetSwitcher* Steps = CastChecked<UWidgetSwitcher>(
			Blueprint->WidgetTree->FindWidget(TEXT("NewRewardStepSwitcher")));
		checkf(Steps->GetNumWidgets() == RewardStepCount + 1,
			TEXT("New RewardConcept03 WBP has wrong step count"));
		UWidgetSwitcher* ChestStates = CastChecked<UWidgetSwitcher>(
			Blueprint->WidgetTree->FindWidget(TEXT("NewChestVisualSwitcher")));
		checkf(ChestStates->GetNumWidgets() == 5,
			TEXT("New RewardConcept03 WBP must contain five chest reveal frames"));
		UE_LOG(LogTemp, Display,
			TEXT("RD_REWARD_CONCEPT03_NEW_VERIFY success asset=%s steps=%d"),
			TargetAssetPath, Steps->GetNumWidgets() - 1);
	}

	void Verify()
	{
		VerifyVariant(FourStepAssetPath,
			URewardConcept03Widget::StaticClass(), true);
		VerifyVariant(ThreeStepAssetPath,
			URewardConcept03NoArtifactWidget::StaticClass(), false);
	}
}

void RegisterRewardConcept03NewWidgetBuilderCommands()
{
	using namespace RewardConcept03NewWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildRewardConcept03New"),
		TEXT("Build the new independent four-step RewardConcept03 WBP."),
		FConsoleCommandDelegate::CreateStatic(&Build));
	VerifyCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.VerifyRewardConcept03New"),
		TEXT("Verify the new independent four-step RewardConcept03 WBP."),
		FConsoleCommandDelegate::CreateStatic(&Verify));
}

void UnregisterRewardConcept03NewWidgetBuilderCommands()
{
	RewardConcept03NewWidgetBuilder::BuildCommand.Reset();
	RewardConcept03NewWidgetBuilder::VerifyCommand.Reset();
}
