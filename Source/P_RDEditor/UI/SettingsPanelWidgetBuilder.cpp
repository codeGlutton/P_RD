#include "UI/SettingsPanelWidgetBuilder.h"
#include "UI/UIFont.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CheckBox.h"
#include "Components/ContentWidget.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace SettingsPanelWidgetBuilder
{
	TUniquePtr<FAutoConsoleCommand> BuildCommand;
	TUniquePtr<FAutoConsoleCommand> TextDefaultsCommand;

	constexpr const TCHAR* SettingsLedgerAssetRoot =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/SettingsLedger");
	constexpr const TCHAR* SettingsConfirmPanelTexturePath =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Common/"
			"T_MB_GenericDetailPanel.T_MB_GenericDetailPanel");

	UTexture2D* LoadSettingsLedgerTexture(const TCHAR* PartName)
	{
		const FString AssetName = FString::Printf(
			TEXT("T_MB_SettingsLedger_%s"), PartName);
		const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"),
			SettingsLedgerAssetRoot, *AssetName, *AssetName);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		if (Texture == nullptr)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("RD_SETTINGS_LEDGER missing texture %s"), *ObjectPath);
		}
		return Texture;
	}

	UTexture2D* LoadSettingsConfirmPanelTexture()
	{
		UTexture2D* Texture = LoadObject<UTexture2D>(
			nullptr, SettingsConfirmPanelTexturePath);
		if (Texture == nullptr)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("RD_SETTINGS_LEDGER missing confirm panel texture %s"),
				SettingsConfirmPanelTexturePath);
		}
		return Texture;
	}

	struct FSettingsLedgerTextures
	{
		UTexture2D* BookBase = LoadSettingsLedgerTexture(TEXT("BookBase"));
		UTexture2D* SectionRibbon = LoadSettingsLedgerTexture(TEXT("SectionRibbon"));
		UTexture2D* RowLabel = LoadSettingsLedgerTexture(TEXT("RowLabel"));
		UTexture2D* ChoiceButton = LoadSettingsLedgerTexture(TEXT("ChoiceButton"));
		UTexture2D* ChoiceButtonSelected =
			LoadSettingsLedgerTexture(TEXT("ChoiceButton_Selected"));
		UTexture2D* ActionButton = LoadSettingsLedgerTexture(TEXT("ActionButton"));
		UTexture2D* ActionButtonDanger =
			LoadSettingsLedgerTexture(TEXT("ActionButton_Danger"));
		UTexture2D* ToggleOff = LoadSettingsLedgerTexture(TEXT("ToggleOff"));
		UTexture2D* ToggleOn = LoadSettingsLedgerTexture(TEXT("ToggleOn"));
		UTexture2D* SliderThumb = LoadSettingsLedgerTexture(TEXT("SliderThumb"));
		UTexture2D* SliderTrack = LoadSettingsLedgerTexture(TEXT("SliderTrack"));
		UTexture2D* SliderFill = LoadSettingsLedgerTexture(TEXT("SliderFill"));
		UTexture2D* ConfirmPanel = LoadSettingsConfirmPanelTexture();
	};

	/** 원본 PNG에서 장식/투명 여백을 제외하고 실제 글자가 들어갈 면. */
	struct FTextureContentRect
	{
		float Left;
		float Top;
		float Right;
		float Bottom;
	};
	constexpr FTextureContentRect RowLabelContent{ .06f, .18f, .94f, .82f };
	constexpr FTextureContentRect RibbonContent{ .14f, .18f, .86f, .76f };
	constexpr FTextureContentRect ChoiceContent{ .10f, .20f, .90f, .78f };
	constexpr FTextureContentRect ActionContent{ .19f, .12f, .81f, .58f };

	void LogPhase(const TCHAR* Phase)
	{
		const FString MarkerPath = FPaths::ProjectSavedDir()
			/ TEXT("SettingsLayoutPhase.txt");
		FFileHelper::SaveStringToFile(FString::Printf(TEXT("%s\n"), Phase),
			*MarkerPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
			&IFileManager::Get(), FILEWRITE_Append);
		UE_LOG(LogTemp, Display, TEXT("RD_SETTINGS_LAYOUT_PHASE %s"), Phase);
	}

	FVector2D TextureNativeSize(const UTexture2D* Texture)
	{
		if (Texture == nullptr)
		{
			return FVector2D::ZeroVector;
		}

		const FIntPoint ImportedSize = Texture->GetImportedSize();
		if (ImportedSize.X > 0 && ImportedSize.Y > 0)
		{
			return FVector2D(ImportedSize.X, ImportedSize.Y);
		}
		return FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
	}

	FSlateBrush MakeTextureBrush(UTexture2D* Texture, const FVector2D ImageSize,
		const FLinearColor Tint = FLinearColor::White)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = Texture != nullptr
			? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;
		Brush.Margin = FMargin(0.f);
		Brush.SetImageSize(ImageSize);
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}

	void PlaceWidgetInSettingsCanvas(UCanvasPanel* ContentCanvas, UWidget* Widget,
		const FVector2D Position, const FVector2D Size,
		int32 ZOrder, ESlateVisibility Visibility);

	void ApplyTextureToImage(UWidgetBlueprint* Blueprint, const TCHAR* ImageName,
		UTexture2D* Texture)
	{
		UImage* Image = Cast<UImage>(
			Blueprint->WidgetTree->FindWidget(FName(ImageName)));
		if (Image == nullptr || Texture == nullptr)
		{
			return;
		}

		Image->SetBrush(MakeTextureBrush(Texture, TextureNativeSize(Texture)));
		Image->SetColorAndOpacity(FLinearColor::White);
	}

	/** The Marchbound detail panel is a complete authored modal, not a 9-slice. */
	void ApplyConfirmPanelTexture(UWidgetBlueprint* Blueprint, UTexture2D* Texture)
	{
		UImage* Image = Cast<UImage>(
			Blueprint->WidgetTree->FindWidget(TEXT("Set_confirm_plate")));
		if (Image == nullptr || Texture == nullptr)
		{
			return;
		}

		FSlateBrush Brush = Image->GetBrush();
		Brush.SetResourceObject(Texture);
		Brush.SetImageSize(TextureNativeSize(Texture));
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.Margin = FMargin(0.f);
		Image->SetBrush(Brush);
		Image->SetColorAndOpacity(FLinearColor::White);
	}

	UTextBlock* FindFirstTextBlockIn(UWidget* Root)
	{
		if (Root == nullptr)
		{
			return nullptr;
		}
		if (UTextBlock* Text = Cast<UTextBlock>(Root))
		{
			return Text;
		}
		if (UContentWidget* Content = Cast<UContentWidget>(Root))
		{
			if (UTextBlock* Text = FindFirstTextBlockIn(Content->GetContent()))
			{
				return Text;
			}
		}
		if (UPanelWidget* Panel = Cast<UPanelWidget>(Root))
		{
			for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
			{
				if (UTextBlock* Text = FindFirstTextBlockIn(Panel->GetChildAt(Index)))
				{
					return Text;
				}
			}
		}
		return nullptr;
	}

	/**
	 * 런타임 USettingsPanelWidget::SyncText와 같은 localization identity를
	 * WBP 기본값에도 굽는다. 디자이너와 게임이 서로 다른 문구 사전을 갖지
	 * 않도록 namespace/key/source를 C++ 쪽과 정확히 공유한다.
	 */
	void ApplyRuntimeTextDefaults(UWidgetBlueprint* Blueprint)
	{
		check(Blueprint != nullptr && Blueprint->WidgetTree != nullptr);
		struct FTextDefault
		{
			const TCHAR* Widget;
			FText Text;
		};
		const FText SettingsLedger = NSLOCTEXT(
			"SettingsPanelWidget_Text", "SettingsLedger", "SETTINGS LEDGER");
		const FText Back = NSLOCTEXT("SettingsPanelWidget_Text", "Back", "Back");
		const FText SaveAndExit = NSLOCTEXT(
			"SettingsPanelWidget_Text", "SaveAndExit", "Save and Exit");
		const FText AbandonRun = NSLOCTEXT(
			"SettingsPanelWidget_Text", "AbandonRun", "Abandon Run");
		const FText Reset = NSLOCTEXT("SettingsPanelWidget_Text", "Reset", "Reset");
		const FText Graphics = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Graphics", "Graphics");
		const FText Volume = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Volume", "Volume");
		const FText Gameplay = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Gameplay", "Gameplay");
		const FText Fps = NSLOCTEXT("SettingsPanelWidget_Text", "FPS", "FPS");
		const FText Quality = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Quality", "Quality");
		const FText Low = NSLOCTEXT("SettingsPanelWidget_Text", "Low", "LOW");
		const FText Mid = NSLOCTEXT("SettingsPanelWidget_Text", "Mid", "MID");
		const FText High = NSLOCTEXT("SettingsPanelWidget_Text", "High", "HIGH");
		const FText ScreenShake = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Screen Shake", "Screen Shake");
		const FText Effects = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Effects", "Effects");
		const FText Language = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Language", "Language");
		const FText Master = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Master", "Master");
		const FText Bgm = NSLOCTEXT("SettingsPanelWidget_Text", "BGM", "BGM");
		const FText Sfx = NSLOCTEXT("SettingsPanelWidget_Text", "SFX", "SFX");
		const FText Ui = NSLOCTEXT("SettingsPanelWidget_Text", "UI", "UI");
		const FText Audio = NSLOCTEXT("SettingsPanelWidget_Text", "Audio", "Audio");
		const FText Display = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Display", "Display");
		const FText Brightness = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Brightness", "Brightness");
		const FText Vibration = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Vibration", "Vibration");
		const FText FastMode = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Fast Mode", "Fast Mode");
		const FText SkipAnimation = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Skip Animation", "Skip Animation");
		const FText AutoEndTurn = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Auto End Turn", "Auto End Turn");
		const FText Info = NSLOCTEXT("SettingsPanelWidget_Text", "Info", "Info");
		const FText Credits = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Credits", "Credits");
		const FText License = NSLOCTEXT(
			"SettingsPanelWidget_Text", "License", "License");
		const FText Open = NSLOCTEXT("SettingsPanelWidget_Text", "Open", "Open");
		const FText AbandonTitle = NSLOCTEXT(
			"SettingsPanelWidget_Text", "AbandonConfirmTitle", "Abandon this run?");
		const FText AbandonBody = NSLOCTEXT("SettingsPanelWidget_Text",
			"AbandonConfirmBody",
			"Current progress will be deleted and you will return to the title.");
		const FText Abandon = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Abandon", "Abandon");
		const FText Cancel = NSLOCTEXT(
			"SettingsPanelWidget_Text", "Cancel", "Cancel");

		const FTextDefault Defaults[] = {
			{ TEXT("SettingsTitleText"), SettingsLedger },
			{ TEXT("Set_sec_graphics_text"), Graphics },
			{ TEXT("Set_sec_display_text"), Graphics },
			{ TEXT("Set_sec_audio_text"), Volume },
			{ TEXT("Set_sec_volume_text"), Volume },
			{ TEXT("Set_sec_gameplay_text"), Gameplay },
			{ TEXT("Set_row_fps_label"), Fps },
			{ TEXT("FpsRow_Label"), Fps },
			{ TEXT("FpsThirtyButton"), FText::AsNumber(30) },
			{ TEXT("FpsSixtyButton"), FText::AsNumber(60) },
			{ TEXT("FpsThirtyButtonText"), FText::AsNumber(30) },
			{ TEXT("FpsSixtyButtonText"), FText::AsNumber(60) },
			{ TEXT("Set_row_quality_label"), Quality },
			{ TEXT("QualityRow_Label"), Quality },
			{ TEXT("QualityLowButton"), Low },
			{ TEXT("QualityMidButton"), Mid },
			{ TEXT("QualityHighButton"), High },
			{ TEXT("LowQualityButton"), Low },
			{ TEXT("MediumQualityButton"), Mid },
			{ TEXT("HighQualityButton"), High },
			{ TEXT("LowQualityButtonText"), Low },
			{ TEXT("MediumQualityButtonText"), Mid },
			{ TEXT("HighQualityButtonText"), High },
			{ TEXT("Set_row_screen_shake_label"), ScreenShake },
			{ TEXT("ScreenShakeRow_Label"), ScreenShake },
			{ TEXT("Set_row_effects_label"), Effects },
			{ TEXT("Set_row_effects_text"), Effects },
			{ TEXT("EffectsRow_Label"), Effects },
			{ TEXT("Set_row_language_label"), Language },
			{ TEXT("LanguageRow_Label"), Language },
			{ TEXT("LanguageKoreanButton"), FText::FromString(TEXT("한국어")) },
			{ TEXT("LanguageEnglishButton"), FText::FromString(TEXT("English")) },
			{ TEXT("LanguageKoreanButtonText"), FText::FromString(TEXT("한국어")) },
			{ TEXT("LanguageEnglishButtonText"), FText::FromString(TEXT("English")) },
			{ TEXT("Set_row_master_label"), Master },
			{ TEXT("MasterVolumeRow_Label"), Master },
			{ TEXT("Set_row_bgm_label"), Bgm },
			{ TEXT("BGMVolumeRow_Label"), Bgm },
			{ TEXT("Set_row_sfx_label"), Sfx },
			{ TEXT("SFXVolumeRow_Label"), Sfx },
			{ TEXT("Set_row_ui_label"), Ui },
			{ TEXT("UIVolumeRow_Label"), Ui },
			{ TEXT("BackButton"), Back },
			{ TEXT("BackButtonText"), Back },
			{ TEXT("SaveAndExitButton"), SaveAndExit },
			{ TEXT("SaveAndExitButtonText"), SaveAndExit },
			{ TEXT("AbandonRunButton"), AbandonRun },
			{ TEXT("AbandonRunButtonText"), AbandonRun },
			{ TEXT("ResetButton"), Reset },
			{ TEXT("ResetButtonText"), Reset },
			{ TEXT("AudioSectionHeader"), Audio },
			{ TEXT("DisplaySectionHeader"), Display },
			{ TEXT("BrightnessRow_Label"), Brightness },
			{ TEXT("VibrationRow_Label"), Vibration },
			{ TEXT("GameplaySectionHeader"), Gameplay },
			{ TEXT("FastModeRow_Label"), FastMode },
			{ TEXT("SkipAnimationRow_Label"), SkipAnimation },
			{ TEXT("AutoEndTurnRow_Label"), AutoEndTurn },
			{ TEXT("InfoSectionHeader"), Info },
			{ TEXT("CreditsRow_Label"), Credits },
			{ TEXT("LicenseRow_Label"), License },
			{ TEXT("CreditsOpenButtonText"), Open },
			{ TEXT("LicenseOpenButtonText"), Open },
			{ TEXT("AbandonConfirmTitleText"), AbandonTitle },
			{ TEXT("AbandonConfirmBodyText"), AbandonBody },
			{ TEXT("ConfirmAbandonButtonText"), Abandon },
			{ TEXT("CancelAbandonButtonText"), Cancel },
			{ TEXT("RunConfirmHeaderText"), NSLOCTEXT(
				"SettingsPanelWidget", "AbandonRunHeader", "ABANDON RUN") },
		};

		for (const FTextDefault& Default : Defaults)
		{
			if (UTextBlock* Text = FindFirstTextBlockIn(
				Blueprint->WidgetTree->FindWidget(FName(Default.Widget))))
			{
				Text->SetText(Default.Text);
			}
		}
	}

	UImage* EnsureLedgerImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Canvas,
		const FName Name, UTexture2D* Texture, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder)
	{
		if (Canvas == nullptr || Texture == nullptr)
		{
			return nullptr;
		}

		UImage* Image = Cast<UImage>(Blueprint->WidgetTree->FindWidget(Name));
		if (Image == nullptr)
		{
			Image = Blueprint->WidgetTree->ConstructWidget<UImage>(
				UImage::StaticClass(), Name);
		}
		Image->SetBrush(MakeTextureBrush(Texture, TextureNativeSize(Texture)));
		Image->SetColorAndOpacity(FLinearColor::White);
		PlaceWidgetInSettingsCanvas(Canvas, Image, Position, Size, ZOrder,
			ESlateVisibility::SelfHitTestInvisible);
		return Image;
	}

	void ApplyLedgerCheckBoxStyle(UWidgetBlueprint* Blueprint,
		const TCHAR* CheckBoxName, UTexture2D* ToggleOff, UTexture2D* ToggleOn)
	{
		UCheckBox* CheckBox = Cast<UCheckBox>(
			Blueprint->WidgetTree->FindWidget(FName(CheckBoxName)));
		if (CheckBox == nullptr || ToggleOff == nullptr || ToggleOn == nullptr)
		{
			return;
		}

		const FVector2D ToggleSize(120.f, 60.f);
		const FSlateBrush Off = MakeTextureBrush(ToggleOff, ToggleSize);
		const FSlateBrush OffHovered = MakeTextureBrush(ToggleOff, ToggleSize,
			FLinearColor(1.18f, 1.18f, 1.18f, 1.f));
		const FSlateBrush OffPressed = MakeTextureBrush(ToggleOff, ToggleSize,
			FLinearColor(.82f, .82f, .82f, 1.f));
		const FSlateBrush On = MakeTextureBrush(ToggleOn, ToggleSize);
		const FSlateBrush OnHovered = MakeTextureBrush(ToggleOn, ToggleSize,
			FLinearColor(1.18f, 1.18f, 1.18f, 1.f));
		const FSlateBrush OnPressed = MakeTextureBrush(ToggleOn, ToggleSize,
			FLinearColor(.82f, .82f, .82f, 1.f));
		FSlateBrush NoDraw;
		NoDraw.DrawAs = ESlateBrushDrawType::NoDrawType;

		FCheckBoxStyle Style = CheckBox->GetWidgetStyle();
		Style.SetCheckBoxType(ESlateCheckBoxType::CheckBox)
			.SetUncheckedImage(Off)
			.SetUncheckedHoveredImage(OffHovered)
			.SetUncheckedPressedImage(OffPressed)
			.SetCheckedImage(On)
			.SetCheckedHoveredImage(OnHovered)
			.SetCheckedPressedImage(OnPressed)
			.SetUndeterminedImage(Off)
			.SetUndeterminedHoveredImage(OffHovered)
			.SetUndeterminedPressedImage(OffPressed)
			.SetPadding(FMargin(0.f))
			.SetBackgroundImage(NoDraw)
			.SetBackgroundHoveredImage(NoDraw)
			.SetBackgroundPressedImage(NoDraw);
		CheckBox->SetWidgetStyle(Style);
	}

	void ApplyLedgerSliderStyle(UWidgetBlueprint* Blueprint,
		const TCHAR* SliderName, UTexture2D* SliderThumb)
	{
		USlider* Slider = Cast<USlider>(
			Blueprint->WidgetTree->FindWidget(FName(SliderName)));
		if (Slider == nullptr || SliderThumb == nullptr)
		{
			return;
		}

		FSlateBrush NoDraw;
		NoDraw.DrawAs = ESlateBrushDrawType::NoDrawType;
		const FVector2D ThumbSize(62.f, 62.f);
		const FSlateBrush Thumb = MakeTextureBrush(SliderThumb, ThumbSize);
		const FSlateBrush ThumbHovered = MakeTextureBrush(SliderThumb, ThumbSize,
			FLinearColor(1.18f, 1.18f, 1.18f, 1.f));
		const FSlateBrush ThumbDisabled = MakeTextureBrush(SliderThumb, ThumbSize,
			FLinearColor(.48f, .48f, .48f, .72f));

		FSliderStyle Style = Slider->GetWidgetStyle();
		Style.SetNormalBarImage(NoDraw)
			.SetHoveredBarImage(NoDraw)
			.SetDisabledBarImage(NoDraw)
			.SetNormalThumbImage(Thumb)
			.SetHoveredThumbImage(ThumbHovered)
			.SetDisabledThumbImage(ThumbDisabled)
			.SetBarThickness(1.f);
		Slider->SetWidgetStyle(Style);
		Slider->SetIndentHandle(false);
	}

	FVector2D AspectFitSize(const FVector2D NativeSize, const FVector2D Bounds)
	{
		if (NativeSize.X <= 0.0 || NativeSize.Y <= 0.0
			|| Bounds.X <= 0.0 || Bounds.Y <= 0.0)
		{
			return Bounds;
		}

		const double UniformScale = FMath::Min(
			Bounds.X / NativeSize.X, Bounds.Y / NativeSize.Y);
		return NativeSize * UniformScale;
	}

	FBox2D TextureContentBox(const FVector2D& Position, const FVector2D& Size,
		const FTextureContentRect& Content)
	{
		return FBox2D(
			Position + FVector2D(Size.X * Content.Left, Size.Y * Content.Top),
			Position + FVector2D(Size.X * Content.Right, Size.Y * Content.Bottom));
	}

	/** AspectFit 된 실제 그림 면을 기준으로 Overlay 안쪽 여백을 계산한다. */
	FMargin TextureContentPadding(const UTexture2D* Texture,
		const FVector2D& Bounds, const FTextureContentRect& Content)
	{
		const FVector2D Fitted = AspectFitSize(TextureNativeSize(Texture), Bounds);
		const FVector2D Offset = (Bounds - Fitted) * .5f;
		return FMargin(
			Offset.X + Fitted.X * Content.Left,
			Offset.Y + Fitted.Y * Content.Top,
			Offset.X + Fitted.X * (1.f - Content.Right),
			Offset.Y + Fitted.Y * (1.f - Content.Bottom));
	}

	FVector2D CanvasMountSize(UWidgetBlueprint* Blueprint, const TCHAR* MountName)
	{
		UWidget* Mount = Blueprint->WidgetTree->FindWidget(FName(MountName));
		const UCanvasPanelSlot* Slot = Mount != nullptr
			? Cast<UCanvasPanelSlot>(Mount->Slot) : nullptr;
		return Slot != nullptr ? Slot->GetSize() : FVector2D::ZeroVector;
	}

	/**
	 * The transparent button continues to fill its mount. Only the visible plate is
	 * centered at one uniform texture scale, so widening a hit target cannot squash art.
	 */
	void AspectFitButtonPlate(UWidgetBlueprint* Blueprint, const TCHAR* PlateName,
		const FVector2D Bounds)
	{
		UImage* Plate = Cast<UImage>(Blueprint->WidgetTree->FindWidget(FName(PlateName)));
		if (Plate == nullptr)
		{
			return;
		}

		FSlateBrush Brush = Plate->GetBrush();
		const UTexture2D* Texture = Cast<UTexture2D>(Brush.GetResourceObject());
		const FVector2D FittedSize = AspectFitSize(TextureNativeSize(Texture), Bounds);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.Margin = FMargin(0.0f);
		Brush.SetImageSize(FittedSize);
		Plate->SetBrush(Brush);

		if (UOverlaySlot* Slot = Cast<UOverlaySlot>(Plate->Slot))
		{
			Slot->SetPadding(FMargin(0.0f));
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetVerticalAlignment(VAlign_Center);
		}
	}

	/** Keep genuinely resizable backplates as 9-slice and restore their native basis. */
	void NormalizeNineSlice(UWidgetBlueprint* Blueprint, const TCHAR* ImageName)
	{
		UImage* Image = Cast<UImage>(Blueprint->WidgetTree->FindWidget(FName(ImageName)));
		if (Image == nullptr)
		{
			return;
		}

		FSlateBrush Brush = Image->GetBrush();
		const UTexture2D* Texture = Cast<UTexture2D>(Brush.GetResourceObject());
		const FVector2D NativeSize = TextureNativeSize(Texture);
		if (NativeSize.X > 0.0 && NativeSize.Y > 0.0)
		{
			Brush.SetImageSize(NativeSize);
		}
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Image->SetBrush(Brush);
	}

	void SetCanvasWidgetSizeCentered(UWidgetBlueprint* Blueprint, const TCHAR* Name,
		const FVector2D ExactSize)
	{
		UWidget* Widget = Blueprint->WidgetTree->FindWidget(FName(Name));
		UCanvasPanelSlot* Slot = Widget != nullptr
			? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
		if (Slot == nullptr)
		{
			return;
		}
		const FVector2D OldSize = Slot->GetSize();
		Slot->SetPosition(Slot->GetPosition() + (OldSize - ExactSize) * .5f);
		Slot->SetSize(ExactSize);
	}

	void ConfigureCanvasSlot(UCanvasPanelSlot* Slot, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder)
	{
		if (Slot == nullptr)
		{
			return;
		}

		Slot->SetAnchors(FAnchors(0.f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
	}

	void SetExistingCanvasRect(UWidgetBlueprint* Blueprint, const TCHAR* Name,
		const FVector2D Position, const FVector2D Size, const int32 ZOrder)
	{
		UWidget* Widget = Blueprint->WidgetTree->FindWidget(FName(Name));
		ConfigureCanvasSlot(Widget != nullptr ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr,
			Position, Size, ZOrder);
	}

	UCanvasPanel* EnsureSettingsContentCanvas(UWidgetBlueprint* Blueprint)
	{
		UOverlay* PanelMount = Cast<UOverlay>(
			Blueprint->WidgetTree->FindWidget(TEXT("Set_panel_bodyMount")));
		if (PanelMount == nullptr)
		{
			return nullptr;
		}

		UCanvasPanel* ContentCanvas = Cast<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("SettingsContentCanvas")));
		if (ContentCanvas == nullptr)
		{
			ContentCanvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(), TEXT("SettingsContentCanvas"));
		}

		if (ContentCanvas->GetParent() != PanelMount)
		{
			if (UPanelWidget* PreviousParent = ContentCanvas->GetParent())
			{
				PreviousParent->RemoveChild(ContentCanvas);
			}
			PanelMount->AddChildToOverlay(ContentCanvas);
		}

		if (UOverlaySlot* Slot = Cast<UOverlaySlot>(ContentCanvas->Slot))
		{
			Slot->SetPadding(FMargin(0.f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
		ContentCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ContentCanvas->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		return ContentCanvas;
	}

	UCanvasPanel* EnsureSettingsDesignCanvas(UWidgetBlueprint* Blueprint)
	{
		UCanvasPanel* Root = Cast<UCanvasPanel>(Blueprint->WidgetTree->RootWidget);
		if (Root == nullptr)
		{
			return nullptr;
		}

		UScaleBox* Scale = Cast<UScaleBox>(
			Blueprint->WidgetTree->FindWidget(TEXT("SettingsScaleBox")));
		if (Scale == nullptr)
		{
			Scale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
				UScaleBox::StaticClass(), TEXT("SettingsScaleBox"));
		}
		if (Scale->GetParent() != Root)
		{
			if (UPanelWidget* PreviousParent = Scale->GetParent())
			{
				PreviousParent->RemoveChild(Scale);
			}
			Root->AddChildToCanvas(Scale);
		}
		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Scale->Slot))
		{
			Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			Slot->SetAlignment(FVector2D::ZeroVector);
			Slot->SetAutoSize(false);
			Slot->SetOffsets(FMargin(0.f));
			Slot->SetZOrder(10);
		}
		Scale->SetStretch(EStretch::ScaleToFit);
		Scale->SetStretchDirection(EStretchDirection::Both);
		Scale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Scale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		USizeBox* DesignSize = Cast<USizeBox>(
			Blueprint->WidgetTree->FindWidget(TEXT("SettingsSizeBox")));
		if (DesignSize == nullptr)
		{
			DesignSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(), TEXT("SettingsSizeBox"));
		}
		if (DesignSize->GetParent() != Scale)
		{
			if (UPanelWidget* PreviousParent = DesignSize->GetParent())
			{
				PreviousParent->RemoveChild(DesignSize);
			}
			Scale->SetContent(DesignSize);
		}
		DesignSize->SetWidthOverride(1920.f);
		DesignSize->SetHeightOverride(1080.f);

		UCanvasPanel* DesignCanvas = Cast<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("SettingsModalCanvas")));
		if (DesignCanvas == nullptr)
		{
			DesignCanvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(), TEXT("SettingsModalCanvas"));
		}
		if (DesignCanvas->GetParent() != DesignSize)
		{
			if (UPanelWidget* PreviousParent = DesignCanvas->GetParent())
			{
				PreviousParent->RemoveChild(DesignCanvas);
			}
			DesignSize->SetContent(DesignCanvas);
		}
		DesignCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		return DesignCanvas;
	}

	/**
	 * Keep the dimmer in viewport space and only aspect-fit the authored dialog.
	 * A 1920x1080 dimmer below SettingsScaleBox can never cover the letterbox bands
	 * of a fold-sized viewport, which is why the old modal left bright strips visible.
	 */
	UCanvasPanel* EnsureRunConfirmViewportLayer(UWidgetBlueprint* Blueprint,
		UCanvasPanel* Root, UWidget* ConfirmPanel)
	{
		if (Blueprint == nullptr || Root == nullptr || ConfirmPanel == nullptr)
		{
			return nullptr;
		}

		UOverlay* Layer = Cast<UOverlay>(
			Blueprint->WidgetTree->FindWidget(TEXT("RunConfirmViewportLayer")));
		if (Layer == nullptr)
		{
			Layer = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(), TEXT("RunConfirmViewportLayer"));
		}
		if (Layer->GetParent() != Root)
		{
			if (UPanelWidget* PreviousParent = Layer->GetParent())
			{
				PreviousParent->RemoveChild(Layer);
			}
			Root->AddChildToCanvas(Layer);
		}
		if (UCanvasPanelSlot* Slot = CastChecked<UCanvasPanelSlot>(Layer->Slot))
		{
			Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			Slot->SetAlignment(FVector2D::ZeroVector);
			Slot->SetAutoSize(false);
			Slot->SetOffsets(FMargin(0.f));
			Slot->SetZOrder(1000);
		}
		Layer->SetClipping(EWidgetClipping::Inherit);
		Layer->SetVisibility(ESlateVisibility::Collapsed);

		UBorder* Dim = Cast<UBorder>(
			Blueprint->WidgetTree->FindWidget(TEXT("RunConfirmViewportDim")));
		if (Dim == nullptr)
		{
			Dim = Blueprint->WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(), TEXT("RunConfirmViewportDim"));
		}
		if (Dim->GetParent() != Layer)
		{
			if (UPanelWidget* PreviousParent = Dim->GetParent())
			{
				PreviousParent->RemoveChild(Dim);
			}
			Layer->AddChildToOverlay(Dim);
		}
		Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, .78f));
		Dim->SetVisibility(ESlateVisibility::Visible);
		if (UOverlaySlot* Slot = CastChecked<UOverlaySlot>(Dim->Slot))
		{
			Slot->SetPadding(FMargin(0.f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}

		UScaleBox* Scale = Cast<UScaleBox>(
			Blueprint->WidgetTree->FindWidget(TEXT("RunConfirmScale")));
		if (Scale == nullptr)
		{
			Scale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
				UScaleBox::StaticClass(), TEXT("RunConfirmScale"));
		}
		if (Scale->GetParent() != Layer)
		{
			if (UPanelWidget* PreviousParent = Scale->GetParent())
			{
				PreviousParent->RemoveChild(Scale);
			}
			Layer->AddChildToOverlay(Scale);
		}
		Scale->SetStretch(EStretch::ScaleToFit);
		Scale->SetStretchDirection(EStretchDirection::Both);
		Scale->SetClipping(EWidgetClipping::Inherit);
		Scale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UOverlaySlot* Slot = CastChecked<UOverlaySlot>(Scale->Slot))
		{
			Slot->SetPadding(FMargin(0.f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}

		USizeBox* DesignSize = Cast<USizeBox>(
			Blueprint->WidgetTree->FindWidget(TEXT("RunConfirmDesignSize")));
		if (DesignSize == nullptr)
		{
			DesignSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(), TEXT("RunConfirmDesignSize"));
		}
		if (DesignSize->GetParent() != Scale)
		{
			if (UPanelWidget* PreviousParent = DesignSize->GetParent())
			{
				PreviousParent->RemoveChild(DesignSize);
			}
			Scale->SetContent(DesignSize);
		}
		DesignSize->SetWidthOverride(1920.f);
		DesignSize->SetHeightOverride(1080.f);
		DesignSize->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		if (ConfirmPanel->GetParent() != DesignSize)
		{
			if (UPanelWidget* PreviousParent = ConfirmPanel->GetParent())
			{
				PreviousParent->RemoveChild(ConfirmPanel);
			}
			DesignSize->SetContent(ConfirmPanel);
		}
		ConfirmPanel->SetVisibility(ESlateVisibility::Visible);
		ConfirmPanel->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		return Root;
	}

	void PlaceWidgetInSettingsCanvas(UCanvasPanel* ContentCanvas, UWidget* Widget,
		const FVector2D Position, const FVector2D Size,
		const int32 ZOrder, const ESlateVisibility Visibility)
	{
		if (ContentCanvas == nullptr || Widget == nullptr)
		{
			return;
		}

		UCanvasPanelSlot* Slot = nullptr;
		if (Widget->GetParent() == ContentCanvas)
		{
			Slot = Cast<UCanvasPanelSlot>(Widget->Slot);
		}
		else
		{
			if (UPanelWidget* PreviousParent = Widget->GetParent())
			{
				PreviousParent->RemoveChild(Widget);
			}
			Slot = ContentCanvas->AddChildToCanvas(Widget);
		}

		ConfigureCanvasSlot(Slot, Position, Size, ZOrder);
		Widget->SetVisibility(Visibility);
	}

	void PlaceInSettingsCanvas(UWidgetBlueprint* Blueprint, UCanvasPanel* ContentCanvas,
		const TCHAR* Name, const FVector2D Position, const FVector2D Size,
		const int32 ZOrder, const ESlateVisibility Visibility)
	{
		PlaceWidgetInSettingsCanvas(ContentCanvas,
			Blueprint->WidgetTree->FindWidget(FName(Name)), Position, Size,
			ZOrder, Visibility);
	}

	void PlaceInTextureContent(UWidgetBlueprint* Blueprint,
		UCanvasPanel* ContentCanvas, const TCHAR* Name,
		const FVector2D& TexturePosition, const FVector2D& TextureSize,
		const FTextureContentRect& Content, const int32 ZOrder)
	{
		const FBox2D Box = TextureContentBox(TexturePosition, TextureSize, Content);
		PlaceInSettingsCanvas(Blueprint, ContentCanvas, Name, Box.Min, Box.GetSize(),
			ZOrder, ESlateVisibility::SelfHitTestInvisible);
	}

	void CollapseNamed(UWidgetBlueprint* Blueprint, const TCHAR* Name)
	{
		if (UWidget* Widget = Blueprint->WidgetTree->FindWidget(FName(Name)))
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	void EnsureRowSurface(UWidgetBlueprint* Blueprint, UCanvasPanel* Canvas,
		const FName Name, const FVector2D Position, const FVector2D Size)
	{
		UBorder* Surface = Cast<UBorder>(Blueprint->WidgetTree->FindWidget(Name));
		if (Surface == nullptr)
		{
			Surface = Blueprint->WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(), Name);
		}
		Surface->SetPadding(FMargin(0.f));
		// A very light umber wash separates each control group without covering the
		// parchment grain that is already painted into the fixed book illustration.
		Surface->SetBrushColor(FLinearColor(0.20f, 0.075f, 0.018f, 0.065f));
		PlaceWidgetInSettingsCanvas(Canvas, Surface, Position, Size, 0,
			ESlateVisibility::SelfHitTestInvisible);
	}

	void PlaceScaledControl(UWidgetBlueprint* Blueprint, UCanvasPanel* Canvas,
		const TCHAR* ControlName, const FVector2D Position, const FVector2D Size,
		const int32 ZOrder)
	{
		UWidget* Control = Blueprint->WidgetTree->FindWidget(FName(ControlName));
		if (Control == nullptr)
		{
			return;
		}

		const FName ScaleName(*(FString(ControlName) + TEXT("_FitScale")));
		UScaleBox* Scale = Cast<UScaleBox>(Blueprint->WidgetTree->FindWidget(ScaleName));
		if (Scale == nullptr)
		{
			Scale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
				UScaleBox::StaticClass(), ScaleName);
		}
		if (Scale->GetContent() != Control)
		{
			if (UPanelWidget* PreviousParent = Control->GetParent())
			{
				PreviousParent->RemoveChild(Control);
			}
			Scale->SetContent(Control);
		}
		Scale->SetStretch(EStretch::ScaleToFit);
		Scale->SetStretchDirection(EStretchDirection::DownOnly);
		Scale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		PlaceWidgetInSettingsCanvas(Canvas, Scale,
			Position, Size, ZOrder, ESlateVisibility::SelfHitTestInvisible);
		if (UScaleBoxSlot* ControlSlot = Cast<UScaleBoxSlot>(Control->Slot))
		{
			ControlSlot->SetHorizontalAlignment(HAlign_Center);
			ControlSlot->SetVerticalAlignment(VAlign_Center);
		}
		Control->SetVisibility(ESlateVisibility::Visible);
	}

	void NormalizeFixedImage(UWidgetBlueprint* Blueprint, const TCHAR* ImageName)
	{
		UImage* Image = Cast<UImage>(Blueprint->WidgetTree->FindWidget(FName(ImageName)));
		if (Image == nullptr)
		{
			return;
		}
		FSlateBrush Brush = Image->GetBrush();
		const UTexture2D* Texture = Cast<UTexture2D>(Brush.GetResourceObject());
		const FVector2D NativeSize = TextureNativeSize(Texture);
		if (NativeSize.X > 0.f && NativeSize.Y > 0.f)
		{
			Brush.SetImageSize(NativeSize);
		}
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.Margin = FMargin(0.f);
		Image->SetBrush(Brush);
	}

	UProgressBar* EnsureSliderFill(UWidgetBlueprint* Blueprint, UCanvasPanel* Canvas,
		const TCHAR* Name, UTexture2D* FillTexture, const FVector2D Position,
		const FVector2D Size)
	{
		UProgressBar* Fill = Cast<UProgressBar>(
			Blueprint->WidgetTree->FindWidget(FName(Name)));
		if (Fill == nullptr)
		{
			Fill = Blueprint->WidgetTree->ConstructWidget<UProgressBar>(
				UProgressBar::StaticClass(), FName(Name));
		}

		FSlateBrush EmptyBrush;
		EmptyBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
		FSlateBrush FillBrush;
		FillBrush.SetResourceObject(FillTexture);
		FillBrush.DrawAs = ESlateBrushDrawType::Image;
		FillBrush.SetImageSize(TextureNativeSize(FillTexture));
		FProgressBarStyle Style;
		Style.SetBackgroundImage(EmptyBrush);
		Style.SetFillImage(FillBrush);
		Style.SetMarqueeImage(EmptyBrush);
		Fill->SetWidgetStyle(Style);
		Fill->SetBarFillType(EProgressBarFillType::LeftToRight);
		Fill->SetPercent(0.5f);
		Fill->SetIsMarquee(false);
		PlaceWidgetInSettingsCanvas(Canvas, Fill, Position, Size, 2,
			ESlateVisibility::SelfHitTestInvisible);
		return Fill;
	}

	void EnsureButtonTextFit(UWidgetBlueprint* Blueprint, const TCHAR* TextName,
		const TCHAR* ContainerName, const FMargin Padding = FMargin(10.f, 4.f))
	{
		UTextBlock* Text = Cast<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(FName(TextName)));
		UOverlay* Container = Cast<UOverlay>(
			Blueprint->WidgetTree->FindWidget(FName(ContainerName)));
		if (Text == nullptr || Container == nullptr)
		{
			return;
		}

		const FName ScaleName(*(FString(TextName) + TEXT("_FitScale")));
		UScaleBox* Scale = Cast<UScaleBox>(Blueprint->WidgetTree->FindWidget(ScaleName));
		if (Scale == nullptr)
		{
			Scale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
				UScaleBox::StaticClass(), ScaleName);
		}

		if (Scale->GetParent() != Container)
		{
			if (UPanelWidget* PreviousParent = Scale->GetParent())
			{
				PreviousParent->RemoveChild(Scale);
			}
			Container->AddChildToOverlay(Scale);
		}
		if (Scale->GetContent() != Text)
		{
			if (UPanelWidget* PreviousParent = Text->GetParent())
			{
				PreviousParent->RemoveChild(Text);
			}
			Scale->SetContent(Text);
		}

		Scale->SetStretch(EStretch::ScaleToFit);
		Scale->SetStretchDirection(EStretchDirection::DownOnly);
		Scale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Scale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Container->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		if (UOverlaySlot* ContainerSlot = Cast<UOverlaySlot>(Container->Slot))
		{
			ContainerSlot->SetPadding(FMargin(0.f));
			ContainerSlot->SetHorizontalAlignment(HAlign_Fill);
			ContainerSlot->SetVerticalAlignment(VAlign_Fill);
		}
		if (UOverlaySlot* ScaleSlot = Cast<UOverlaySlot>(Scale->Slot))
		{
			ScaleSlot->SetPadding(Padding);
			ScaleSlot->SetHorizontalAlignment(HAlign_Fill);
			ScaleSlot->SetVerticalAlignment(VAlign_Fill);
		}
		if (UScaleBoxSlot* TextSlot = Cast<UScaleBoxSlot>(Text->Slot))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Center);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
		// The legacy mockup stores per-label render offsets and text margins. Those
		// survive reparenting and can make a mathematically centered ScaleBox still
		// draw its glyphs off-center. Normalize the label itself as part of the button
		// contract; the caller's padding is the sole source of optical adjustment.
		Text->SetJustification(ETextJustify::Center);
		Text->SetMargin(FMargin(0.f));
		Text->SetRenderTransform(FWidgetTransform());
		Text->SetRenderTransformPivot(FVector2D(.5f, .5f));
		Text->SetAutoWrapText(false);
		Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void LayoutSettingsContent(UWidgetBlueprint* Blueprint)
	{
		const FSettingsLedgerTextures Art;
		UCanvasPanel* DesignCanvas = EnsureSettingsDesignCanvas(Blueprint);
		if (DesignCanvas == nullptr)
		{
			return;
		}

		// The book is a fixed composition: page edges, title plaque and center spine
		// must all keep the authored proportions. It nearly fills the 16:9 design
		// canvas and leaves a small amount of the in-game scene visible around it.
		ApplyTextureToImage(Blueprint, TEXT("Set_panel_body"), Art.BookBase);
		if (UImage* BookImage = Cast<UImage>(
			Blueprint->WidgetTree->FindWidget(TEXT("Set_panel_body"))))
		{
			BookImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			if (UOverlaySlot* BookSlot = Cast<UOverlaySlot>(BookImage->Slot))
			{
				BookSlot->SetPadding(FMargin(0.f));
				BookSlot->SetHorizontalAlignment(HAlign_Fill);
				BookSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
		PlaceInSettingsCanvas(Blueprint, DesignCanvas, TEXT("Set_panel_bodyMount"),
			FVector2D(175.f, 28.f), FVector2D(1570.f, 1001.f), 10,
			ESlateVisibility::SelfHitTestInvisible);
		UWidget* ConfirmPanel = Blueprint->WidgetTree->FindWidget(
			TEXT("AbandonConfirmPanel"));
		UCanvasPanel* Root = Cast<UCanvasPanel>(Blueprint->WidgetTree->RootWidget);
		if (EnsureRunConfirmViewportLayer(Blueprint, Root, ConfirmPanel) == nullptr)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_SETTINGS_READABILITY_BUILD failed to create viewport confirm layer"));
			return;
		}
		// Keep all modal children inside the 1920x1080 confirmation canvas and give
		// the authored panel enough breathing room for localized copy.
		// Legacy dims live inside the aspect-fitted dialog and therefore cannot cover
		// non-16:9 letterbox bands. The root-level RunConfirmViewportDim owns dimming.
		CollapseNamed(Blueprint, TEXT("AbandonConfirmDim"));
		SetExistingCanvasRect(Blueprint, TEXT("Set_confirm_dim"),
			FVector2D::ZeroVector, FVector2D(1920.f, 1080.f), 0);
		CollapseNamed(Blueprint, TEXT("Set_confirm_dim"));
		SetExistingCanvasRect(Blueprint, TEXT("Set_confirm_plateMount"),
			FVector2D(420.f, 180.f), FVector2D(1080.f, 720.f), 1);
		SetExistingCanvasRect(Blueprint, TEXT("AbandonConfirmSize"),
			FVector2D(420.f, 180.f), FVector2D(1080.f, 720.f), 2);
		SetExistingCanvasRect(Blueprint, TEXT("Set_confirm_canvas"),
			FVector2D(420.f, 180.f), FVector2D(1080.f, 720.f), 2);
		UTextBlock* Header = Cast<UTextBlock>(Blueprint->WidgetTree->FindWidget(
			TEXT("RunConfirmHeaderText")));
		if (Header == nullptr)
		{
			Header = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(), TEXT("RunConfirmHeaderText"));
		}
		FSlateFontInfo HeaderFont = UIFont::MakeProjectExact(Header->GetFont(), 34);
		HeaderFont.OutlineSettings.OutlineSize = 1;
		HeaderFont.OutlineSettings.OutlineColor = FLinearColor(.025f, .012f, .004f, 1.f);
		Header->SetFont(HeaderFont);
		Header->SetText(FText::FromString(TEXT("런 포기")));
		Header->SetJustification(ETextJustify::Center);
		Header->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, .90f, .68f, 1.f)));
		Header->SetShadowOffset(FVector2D(1.5f, 1.5f));
		Header->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .68f));
		Header->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		UCanvasPanel* ConfirmCanvas = Cast<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("Set_confirm_canvas")));
		PlaceWidgetInSettingsCanvas(ConfirmCanvas, Header,
			FVector2D(670.f, 210.f), FVector2D(580.f, 76.f), 103,
			ESlateVisibility::SelfHitTestInvisible);

		UCanvasPanel* Canvas = EnsureSettingsContentCanvas(Blueprint);
		if (Canvas == nullptr)
		{
			return;
		}

		// The previous WBP still contains some first-pass grid and segment art. Keep
		// those widgets (old captures and diagnostics reference their names), but stop
		// drawing them under the new self-contained ledger composition.
		for (const TCHAR* Name : {
			TEXT("Set_panel_frame"), TEXT("Set_grid_base"), TEXT("Set_grid_fold"),
			TEXT("Set_sec_graphics_banner"), TEXT("Set_sec_display_banner"),
			TEXT("Set_sec_audio_banner"), TEXT("Set_sec_volume_banner"),
			TEXT("Set_sec_gameplay_banner"), TEXT("Set_seg_fps"),
			TEXT("Set_seg_quality"), TEXT("Set_seg_language"), TEXT("Set_btn_close"),
			TEXT("Set_sec_graphics_text"), TEXT("Set_sec_display_text"),
			TEXT("Set_sec_audio_text"), TEXT("Set_sec_volume_text"),
			TEXT("Set_sec_gameplay_text"), TEXT("Set_row_fps_label"),
			TEXT("Set_row_quality_label"), TEXT("Set_row_language_label"),
			TEXT("Set_row_master_label"), TEXT("Set_row_bgm_label"),
			TEXT("Set_row_sfx_label"), TEXT("Set_row_ui_label"),
			TEXT("Set_row_screen_shake_label"), TEXT("Set_row_effects_label") })
		{
			CollapseNamed(Blueprint, Name);
		}

		EnsureLedgerImage(Blueprint, Canvas, TEXT("SettingsAudioRibbonArt"),
			Art.SectionRibbon, FVector2D(188.f, 150.f), FVector2D(450.f, 112.f), 1);
		EnsureLedgerImage(Blueprint, Canvas, TEXT("SettingsDisplayRibbonArt"),
			Art.SectionRibbon, FVector2D(928.f, 145.f), FVector2D(450.f, 112.f), 1);
		EnsureLedgerImage(Blueprint, Canvas, TEXT("SettingsGameplayRibbonArt"),
			Art.SectionRibbon, FVector2D(928.f, 602.f), FVector2D(450.f, 112.f), 1);
		// Keep a serialized hard reference so the selected-state texture is gathered
		// for cooked builds; runtime swaps the existing plate brush to this resource.
		if (UImage* SelectedCookReference = EnsureLedgerImage(Blueprint, Canvas,
			TEXT("SettingsChoiceSelectedCookReference"), Art.ChoiceButtonSelected,
			FVector2D::ZeroVector, FVector2D(1.f, 1.f), -10))
		{
			SelectedCookReference->SetVisibility(ESlateVisibility::Collapsed);
		}

		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("SettingsTitleText_Center"),
			FVector2D(515.f, 34.f), FVector2D(540.f, 76.f), 5,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInTextureContent(Blueprint, Canvas, TEXT("AudioSectionHeader_Center"),
			FVector2D(188.f, 150.f), FVector2D(450.f, 112.f), RibbonContent, 4);
		PlaceInTextureContent(Blueprint, Canvas, TEXT("DisplaySectionHeader_Center"),
			FVector2D(928.f, 145.f), FVector2D(450.f, 112.f), RibbonContent, 4);

		struct FSliderRow
		{
			const TCHAR* Plate;
			const TCHAR* Label;
			const TCHAR* Track;
			const TCHAR* Slider;
			const TCHAR* Fill;
		};
		const FSliderRow SliderRows[] = {
			{ TEXT("Set_row_master_plate"), TEXT("MasterVolumeRow_Label_Center"),
				TEXT("Set_slider_track_master"), TEXT("MasterVolumeSlider"),
				TEXT("Set_slider_fill_master") },
			{ TEXT("Set_row_bgm_plate"), TEXT("BGMVolumeRow_Label_Center"),
				TEXT("Set_slider_track_bgm"), TEXT("BGMVolumeSlider"),
				TEXT("Set_slider_fill_bgm") },
			{ TEXT("Set_row_sfx_plate"), TEXT("SFXVolumeRow_Label_Center"),
				TEXT("Set_slider_track_sfx"), TEXT("SFXVolumeSlider"),
				TEXT("Set_slider_fill_sfx") },
			{ TEXT("Set_row_ui_plate"), TEXT("UIVolumeRow_Label_Center"),
				TEXT("Set_slider_track_ui"), TEXT("UIVolumeSlider"),
				TEXT("Set_slider_fill_ui") },
		};
		constexpr float AudioRowX = 125.f;
		constexpr float AudioRowWidth = 595.f;
		constexpr float AudioRowHeight = 78.f;
		constexpr float AudioRowStartY = 266.f;
		constexpr float AudioRowPitch = 93.f;
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(SliderRows); ++Index)
		{
			const float Y = AudioRowStartY + Index * AudioRowPitch;
			const FSliderRow& Row = SliderRows[Index];
			EnsureRowSurface(Blueprint, Canvas,
				FName(*(FString(Row.Plate) + TEXT("_Surface"))),
				FVector2D(AudioRowX, Y), FVector2D(AudioRowWidth, AudioRowHeight));
			ApplyTextureToImage(Blueprint, Row.Plate, Art.RowLabel);
			PlaceInSettingsCanvas(Blueprint, Canvas, Row.Plate,
				FVector2D(140.f, Y + 4.f), FVector2D(190.f, 71.f), 1,
				ESlateVisibility::SelfHitTestInvisible);
			PlaceInTextureContent(Blueprint, Canvas, Row.Label,
				FVector2D(140.f, Y + 4.f), FVector2D(190.f, 71.f),
				RowLabelContent, 3);
			ApplyTextureToImage(Blueprint, Row.Track, Art.SliderTrack);
			PlaceInSettingsCanvas(Blueprint, Canvas, Row.Track,
				FVector2D(338.f, Y + 12.4f), FVector2D(360.f, 53.15f), 1,
				ESlateVisibility::SelfHitTestInvisible);
			if (Art.SliderFill != nullptr)
			{
				EnsureSliderFill(Blueprint, Canvas, Row.Fill, Art.SliderFill,
					FVector2D(352.f, Y + 24.f), FVector2D(332.f, 29.f));
			}
			ApplyLedgerSliderStyle(Blueprint, Row.Slider, Art.SliderThumb);
			PlaceInSettingsCanvas(Blueprint, Canvas, Row.Slider,
				FVector2D(338.f, Y + 7.f), FVector2D(360.f, 64.f), 3,
				ESlateVisibility::Visible);
		}

		struct FToggleRow
		{
			const TCHAR* Plate;
			const TCHAR* Label;
			const TCHAR* CheckBox;
		};
		const FToggleRow ToggleRows[] = {
			{ TEXT("Set_row_shake_plate"), TEXT("ScreenShakeRow_Label_Center"),
				TEXT("ScreenShakeCheckBox") },
			{ TEXT("Set_row_effects_plate"), TEXT("EffectsRow_Label_Center"),
				TEXT("EffectsCheckBox") },
		};
		constexpr float DisplayRowX = 848.f;
		constexpr float DisplayRowWidth = 575.f;
		constexpr float DisplayRowHeight = 74.f;
		constexpr float DisplayRowStartY = 260.f;
		constexpr float DisplayRowPitch = 86.f;
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(ToggleRows); ++Index)
		{
			const float Y = DisplayRowStartY + Index * DisplayRowPitch;
			const FToggleRow& Row = ToggleRows[Index];
			EnsureRowSurface(Blueprint, Canvas,
				FName(*(FString(Row.Plate) + TEXT("_Surface"))),
				FVector2D(DisplayRowX, Y), FVector2D(DisplayRowWidth, DisplayRowHeight));
			ApplyTextureToImage(Blueprint, Row.Plate, Art.RowLabel);
			PlaceInSettingsCanvas(Blueprint, Canvas, Row.Plate,
				FVector2D(864.f, Y + 3.f), FVector2D(238.f, 68.f), 1,
				ESlateVisibility::SelfHitTestInvisible);
			PlaceInTextureContent(Blueprint, Canvas, Row.Label,
				FVector2D(864.f, Y + 3.f), FVector2D(238.f, 68.f),
				RowLabelContent, 3);
			ApplyLedgerCheckBoxStyle(Blueprint, Row.CheckBox,
				Art.ToggleOff, Art.ToggleOn);
			PlaceScaledControl(Blueprint, Canvas, Row.CheckBox,
				FVector2D(1288.f, Y + 7.f), FVector2D(120.f, 60.f), 3);
		}

		// Vibration has no persistence or platform implementation yet. Keep its
		// compatibility widgets, but do not advertise a switch that cannot work.
		for (const TCHAR* Name : { TEXT("Set_row_vibration_plate"),
			TEXT("VibrationRow_Label_Center"), TEXT("VibrationCheckBox") })
		{
			CollapseNamed(Blueprint, Name);
		}

		const float QualityY = 432.f;
		EnsureRowSurface(Blueprint, Canvas, TEXT("SettingsQualityRowSurface"),
			FVector2D(DisplayRowX, QualityY),
			FVector2D(DisplayRowWidth, DisplayRowHeight));
		ApplyTextureToImage(Blueprint, TEXT("Set_row_quality_plate"), Art.RowLabel);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("Set_row_quality_plate"),
			FVector2D(864.f, QualityY + 3.f), FVector2D(180.f, 68.f), 1,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInTextureContent(Blueprint, Canvas, TEXT("QualityRow_Label_Center"),
			FVector2D(864.f, QualityY + 3.f), FVector2D(180.f, 68.f),
			RowLabelContent, 3);
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const TCHAR* MountNames[] = { TEXT("LowQualityButtonPlateMount"),
				TEXT("MediumQualityButtonPlateMount"), TEXT("HighQualityButtonPlateMount") };
			PlaceInSettingsCanvas(Blueprint, Canvas, MountNames[Index],
				FVector2D(1058.f + Index * 120.f, QualityY + 8.f),
				FVector2D(112.f, 56.f), 2, ESlateVisibility::SelfHitTestInvisible);
		}

		const float FpsY = 516.f;
		EnsureRowSurface(Blueprint, Canvas, TEXT("SettingsFpsRowSurface"),
			FVector2D(DisplayRowX, FpsY),
			FVector2D(DisplayRowWidth, DisplayRowHeight));
		ApplyTextureToImage(Blueprint, TEXT("Set_row_fps_plate"), Art.RowLabel);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("Set_row_fps_plate"),
			FVector2D(864.f, FpsY + 3.f), FVector2D(180.f, 68.f), 1,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInTextureContent(Blueprint, Canvas, TEXT("FpsRow_Label_Center"),
			FVector2D(864.f, FpsY + 3.f), FVector2D(180.f, 68.f),
			RowLabelContent, 3);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("FpsThirtyButtonPlateMount"),
			FVector2D(1080.f, FpsY + 7.f), FVector2D(148.f, 58.f), 2,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("FpsSixtyButtonPlateMount"),
			FVector2D(1245.f, FpsY + 7.f), FVector2D(148.f, 58.f), 2,
			ESlateVisibility::SelfHitTestInvisible);

		PlaceInTextureContent(Blueprint, Canvas, TEXT("GameplaySectionHeader_Center"),
			FVector2D(928.f, 602.f), FVector2D(450.f, 112.f), RibbonContent, 4);
		EnsureRowSurface(Blueprint, Canvas, TEXT("SettingsLanguageRowSurface"),
			FVector2D(DisplayRowX, 714.f),
			FVector2D(DisplayRowWidth, DisplayRowHeight));
		ApplyTextureToImage(Blueprint, TEXT("Set_row_language_plate"), Art.RowLabel);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("Set_row_language_plate"),
			FVector2D(864.f, 717.f), FVector2D(190.f, 68.f), 1,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInTextureContent(Blueprint, Canvas, TEXT("LanguageRow_Label_Center"),
			FVector2D(864.f, 717.f), FVector2D(190.f, 68.f),
			RowLabelContent, 3);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("LanguageKoreanButtonPlateMount"),
			FVector2D(1080.f, 721.f), FVector2D(148.f, 58.f), 2,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("LanguageEnglishButtonPlateMount"),
			FVector2D(1245.f, 721.f), FVector2D(148.f, 58.f), 2,
			ESlateVisibility::SelfHitTestInvisible);

		// Every functional button keeps its existing transparent hit target; only the
		// plate resource changes. Runtime selection tinting still targets these exact
		// plate names.
		for (const TCHAR* Name : {
			TEXT("LowQualityButtonPlate"), TEXT("MediumQualityButtonPlate"),
			TEXT("HighQualityButtonPlate"), TEXT("FpsThirtyButtonPlate"),
			TEXT("FpsSixtyButtonPlate"), TEXT("LanguageKoreanButtonPlate"),
			TEXT("LanguageEnglishButtonPlate") })
		{
			ApplyTextureToImage(Blueprint, Name, Art.ChoiceButton);
		}
		for (const TCHAR* Name : {
			TEXT("BackButtonPlate"), TEXT("ResetButtonPlate"),
			TEXT("SaveAndExitButtonPlate"), TEXT("CancelAbandonButtonPlate") })
		{
			ApplyTextureToImage(Blueprint, Name, Art.ActionButton);
		}
		for (const TCHAR* Name : {
			TEXT("AbandonRunButtonPlate"), TEXT("ConfirmAbandonButtonPlate") })
		{
			ApplyTextureToImage(Blueprint, Name, Art.ActionButtonDanger);
		}
		ApplyConfirmPanelTexture(Blueprint, Art.ConfirmPanel);

		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("StatusText_Center"),
			FVector2D(335.f, 798.f), FVector2D(900.f, 34.f), 4,
			ESlateVisibility::SelfHitTestInvisible);

		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("BackButtonPlateMount"),
			FVector2D(90.f, 842.f), FVector2D(300.f, 118.f), 2,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("ResetButtonPlateMount"),
			FVector2D(400.f, 842.f), FVector2D(300.f, 118.f), 2,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("RunActionsPanel"),
			FVector2D(805.f, 842.f), FVector2D(620.f, 118.f), 2,
			ESlateVisibility::SelfHitTestInvisible);

		// The retired mockup-only rows stay in the tree for compatibility but cannot
		// accidentally reappear in the functional settings composition.
		for (const TCHAR* Name : { TEXT("BrightnessRow_Label_Center"),
			TEXT("FastModeRow_Label_Center"), TEXT("SkipAnimationRow_Label_Center"),
			TEXT("AutoEndTurnRow_Label_Center"), TEXT("CreditsOpenButtonText_Center"),
			TEXT("LicenseOpenButtonText_Center") })
		{
			CollapseNamed(Blueprint, Name);
		}

		struct FTextFitSpec
		{
			const TCHAR* Text;
			const TCHAR* Container;
			UTexture2D* Texture;
			FVector2D Bounds;
			FTextureContentRect Content;
		};
		const FVector2D ConfirmBounds = [&Blueprint](const TCHAR* Mount)
		{
			const FVector2D Found = CanvasMountSize(Blueprint, Mount);
			return Found.X > 0.f && Found.Y > 0.f
				? Found : FVector2D(300.f, 100.f);
		}(TEXT("ConfirmAbandonButtonPlateMount"));
		const FVector2D CancelBounds = [&Blueprint](const TCHAR* Mount)
		{
			const FVector2D Found = CanvasMountSize(Blueprint, Mount);
			return Found.X > 0.f && Found.Y > 0.f
				? Found : FVector2D(300.f, 100.f);
		}(TEXT("CancelAbandonButtonPlateMount"));
		const FTextFitSpec TextFits[] = {
			{ TEXT("BackButtonText"), TEXT("BackButtonText_Center"), Art.ActionButton, FVector2D(300.f, 118.f), ActionContent },
			{ TEXT("ResetButtonText"), TEXT("ResetButtonText_Center"), Art.ActionButton, FVector2D(300.f, 118.f), ActionContent },
			{ TEXT("FpsThirtyButtonText"), TEXT("FpsThirtyButtonText_Center"), Art.ChoiceButton, FVector2D(148.f, 58.f), ChoiceContent },
			{ TEXT("FpsSixtyButtonText"), TEXT("FpsSixtyButtonText_Center"), Art.ChoiceButton, FVector2D(148.f, 58.f), ChoiceContent },
			{ TEXT("LowQualityButtonText"), TEXT("LowQualityButtonText_Center"), Art.ChoiceButton, FVector2D(112.f, 56.f), ChoiceContent },
			{ TEXT("MediumQualityButtonText"), TEXT("MediumQualityButtonText_Center"), Art.ChoiceButton, FVector2D(112.f, 56.f), ChoiceContent },
			{ TEXT("HighQualityButtonText"), TEXT("HighQualityButtonText_Center"), Art.ChoiceButton, FVector2D(112.f, 56.f), ChoiceContent },
			{ TEXT("LanguageKoreanButtonText"), TEXT("LanguageKoreanButtonText_Center"), Art.ChoiceButton, FVector2D(148.f, 58.f), ChoiceContent },
			{ TEXT("LanguageEnglishButtonText"), TEXT("LanguageEnglishButtonText_Center"), Art.ChoiceButton, FVector2D(148.f, 58.f), ChoiceContent },
			{ TEXT("SaveAndExitButtonText"), TEXT("Set_run_SaveAndExitButton"), Art.ActionButton, FVector2D(310.f, 118.f), ActionContent },
			{ TEXT("AbandonRunButtonText"), TEXT("Set_run_AbandonRunButton"), Art.ActionButtonDanger, FVector2D(310.f, 118.f), ActionContent },
			{ TEXT("ConfirmAbandonButtonText"), TEXT("ConfirmAbandonButtonText_Center"), Art.ActionButtonDanger, ConfirmBounds, ActionContent },
			{ TEXT("CancelAbandonButtonText"), TEXT("CancelAbandonButtonText_Center"), Art.ActionButton, CancelBounds, ActionContent },
		};
		for (const FTextFitSpec& Fit : TextFits)
		{
			// 버튼 이름이 길어질 때 ScaleBox가 전체 클릭 영역을 기준으로만
			// 축소하도록 라벨 영역을 버튼과 정확히 일치시킨다. 이미지 안쪽 면을
			// 다시 여백으로 적용하면 언어마다 보이는 중심이 달라진다.
			EnsureButtonTextFit(Blueprint, Fit.Text, Fit.Container, FMargin(0.f));
		}
	}

	int32 ExactFontSize(const FString& Name)
	{
		if (Name.Contains(TEXT("SettingsTitle"))
			|| Name.Contains(TEXT("AbandonConfirmTitle")))
		{
			return Name.Contains(TEXT("AbandonConfirmTitle")) ? 40 : 50;
		}

		if (Name.Contains(TEXT("BackButtonText"))
			|| Name.Contains(TEXT("ResetButtonText"))
			|| Name.Contains(TEXT("SaveAndExitButtonText"))
			|| Name.Contains(TEXT("AbandonRunButtonText"))
			|| Name.Contains(TEXT("ConfirmAbandonButtonText"))
			|| Name.Contains(TEXT("CancelAbandonButtonText")))
		{
			return 30;
		}

		if (Name.Contains(TEXT("FpsThirtyButtonText"))
			|| Name.Contains(TEXT("FpsSixtyButtonText"))
			|| Name.Contains(TEXT("LowQualityButtonText"))
			|| Name.Contains(TEXT("MediumQualityButtonText"))
			|| Name.Contains(TEXT("HighQualityButtonText"))
			|| Name.Contains(TEXT("LanguageKoreanButtonText"))
			|| Name.Contains(TEXT("LanguageEnglishButtonText"))
			|| Name.Contains(TEXT("Set_fps30_text"))
			|| Name.Contains(TEXT("Set_fps60_text")))
		{
			return 25;
		}

		if (Name.Contains(TEXT("SectionHeader")))
		{
			return 32;
		}
		if (Name.Contains(TEXT("StatusText"))
			|| Name.Contains(TEXT("AbandonConfirmBody")))
		{
			return 22;
		}
		return 27;
	}

	/**
	 * Old mockup passes aligned glyph baselines with large negative top padding
	 * (title -63.9, rows -22.2, option buttons -17.7). Once the project font and
	 * final sizes are applied those offsets pull ink outside its cell. Keep each
	 * wrapper's layout padding intact, but normalize the TextBlock's own child slot.
	 */
	void CenterTextInOwnCell(UTextBlock* Text)
	{
		Text->SetJustification(ETextJustify::Center);

		if (UOverlaySlot* Slot = Cast<UOverlaySlot>(Text->Slot))
		{
			Slot->SetPadding(FMargin(0.f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Center);
		}
		else if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Text->Slot))
		{
			ButtonSlot->SetPadding(FMargin(0.f));
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
			ButtonSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	void ApplyProjectTextStyle(UTextBlock* Text)
	{
		const FString Name = Text->GetName();
		const int32 Size = ExactFontSize(Name);
		FSlateFontInfo Font = UIFont::MakeProjectExact(Text->GetFont(), Size);
		const bool bParchmentInk = Name.Contains(TEXT("SettingsTitle"))
			|| Name.Contains(TEXT("Row_Label"))
			|| Name.Contains(TEXT("StatusText"));
		Font.OutlineSettings.OutlineSize = bParchmentInk ? 0 : (Size >= 28 ? 1 : 0);
		Font.OutlineSettings.OutlineColor = FLinearColor(0.025f, 0.012f, 0.004f, 1.f);
		Text->SetFont(Font);

		if (bParchmentInk)
		{
			Text->SetColorAndOpacity(FSlateColor(
				FLinearColor(0.19f, 0.065f, 0.014f, 1.f)));
			Text->SetShadowOffset(FVector2D(.8f, .8f));
			Text->SetShadowColorAndOpacity(FLinearColor(.55f, .31f, .10f, .24f));
		}
		else
		{
			// Ribbons and wood/brass buttons use warm ivory ink. Runtime selection
			// changes only the plate tint, so localization never changes readability.
			Text->SetColorAndOpacity(FSlateColor(
				FLinearColor(1.f, .90f, .68f, 1.f)));
			Text->SetShadowOffset(FVector2D(1.5f, 1.5f));
			Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .68f));
		}
		CenterTextInOwnCell(Text);
	}

	void FillRunActionSlot(UWidgetBlueprint* Blueprint, const TCHAR* ContainerName,
		const TCHAR* ButtonName)
	{
		UWidget* Container = Blueprint->WidgetTree->FindWidget(FName(ContainerName));
		if (UHorizontalBoxSlot* Slot = Container != nullptr
			? Cast<UHorizontalBoxSlot>(Container->Slot) : nullptr)
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetPadding(FMargin(0.f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}

		UWidget* Button = Blueprint->WidgetTree->FindWidget(FName(ButtonName));
		if (UOverlaySlot* Slot = Button != nullptr
			? Cast<UOverlaySlot>(Button->Slot) : nullptr)
		{
			Slot->SetPadding(FMargin(0.f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	/**
	 * Keep the functional transparent button in the same overlay as its visible
	 * plate. Moving only the plate mount leaves the old button at its legacy canvas
	 * coordinates: the art looks tappable, but the hit target is somewhere else.
	 */
	void FillButtonPlateMount(UWidgetBlueprint* Blueprint, const TCHAR* MountName,
		const TCHAR* ButtonName)
	{
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			return;
		}

		UOverlay* Mount = Cast<UOverlay>(
			Blueprint->WidgetTree->FindWidget(FName(MountName)));
		UButton* Button = Cast<UButton>(
			Blueprint->WidgetTree->FindWidget(FName(ButtonName)));
		if (Mount == nullptr || Button == nullptr)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_SETTINGS_READABILITY_BUILD missing button mount %s/%s"),
				MountName, ButtonName);
			return;
		}

		if (Button->GetParent() != Mount)
		{
			if (UPanelWidget* PreviousParent = Button->GetParent())
			{
				PreviousParent->RemoveChild(Button);
			}
			Mount->AddChildToOverlay(Button);
		}

		if (UOverlaySlot* Slot = CastChecked<UOverlaySlot>(Button->Slot))
		{
			Slot->SetPadding(FMargin(0.f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
		Button->SetVisibility(ESlateVisibility::Visible);
		Button->SetIsEnabled(true);
		Button->SetTouchMethod(EButtonTouchMethod::PreciseTap);
		Button->SetClickMethod(EButtonClickMethod::PreciseClick);
	}

	bool SaveCompiledBlueprint(UWidgetBlueprint* Blueprint)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		return UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename,
			FSavePackageArgs());
	}

	/** 설정판의 좌표/아트는 보존하고 C++ localization 기본값만 WBP에 동기화한다. */
	void SyncTextDefaultsOnly()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr,
			TEXT("/Game/UI/WBP_SettingsPanel.WBP_SettingsPanel"));
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_SETTINGS_TEXT_SYNC missing WBP"));
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		ApplyRuntimeTextDefaults(Blueprint);
		if (SaveCompiledBlueprint(Blueprint) == false)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_SETTINGS_TEXT_SYNC save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_SETTINGS_TEXT_SYNC success source=SettingsPanelWidget::SyncText"));
	}

	void Build()
	{
		LogPhase(TEXT("begin"));
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr,
			TEXT("/Game/UI/WBP_SettingsPanel.WBP_SettingsPanel"));
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_SETTINGS_READABILITY_BUILD missing WBP"));
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		Blueprint->WidgetTree->ForEachWidget([](UWidget* Widget)
		{
			UTextBlock* Text = Cast<UTextBlock>(Widget);
			if (Text == nullptr)
			{
				return;
			}
			ApplyProjectTextStyle(Text);
		});
		LogPhase(TEXT("styled"));

		// FPS 버튼에는 오래된 Button 자식 텍스트와 판 위의 정식 라벨이
		// 동시에 남아 있었다. 두 글자가 겹치지 않도록 정식 라벨만 그린다.
		for (const TCHAR* Name : { TEXT("Set_fps30_text"), TEXT("Set_fps60_text") })
		{
			if (UTextBlock* Duplicate = Cast<UTextBlock>(
				Blueprint->WidgetTree->FindWidget(FName(Name))))
			{
				Duplicate->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		// Move every visible setting into a panel-local functional row. The previous
		// asset mixed label children of the frame Overlay with controls on the root
		// Canvas, so the composition only happened to line up at one resolution.
		LayoutSettingsContent(Blueprint);
		ApplyRuntimeTextDefaults(Blueprint);
		LogPhase(TEXT("laid-out"));

		// RunActionsPanel is 620x118. Its two children and the actual transparent
		// buttons fill equal 310x118 functional targets.
		FillRunActionSlot(Blueprint, TEXT("Set_run_SaveAndExitButton"),
			TEXT("SaveAndExitButton"));
		FillRunActionSlot(Blueprint, TEXT("Set_run_AbandonRunButton"),
			TEXT("AbandonRunButton"));
		// The ledger redesign moves these visible mounts. Move the real hit targets
		// with them so both mouse and Android touch land on the rendered buttons.
		FillButtonPlateMount(Blueprint, TEXT("BackButtonPlateMount"),
			TEXT("BackButton"));
		FillButtonPlateMount(Blueprint, TEXT("ResetButtonPlateMount"),
			TEXT("ResetButton"));

		// Visible button plates use their texture aspect. Their mounts/buttons keep the
		// existing functional size, so the extra space becomes transparent padding.
		struct FCanvasPlate
		{
			const TCHAR* PlateName;
			const TCHAR* MountName;
		};
		const FCanvasPlate CanvasPlates[] = {
			{ TEXT("BackButtonPlate"), TEXT("BackButtonPlateMount") },
			{ TEXT("ResetButtonPlate"), TEXT("ResetButtonPlateMount") },
			{ TEXT("FpsThirtyButtonPlate"), TEXT("FpsThirtyButtonPlateMount") },
			{ TEXT("FpsSixtyButtonPlate"), TEXT("FpsSixtyButtonPlateMount") },
			{ TEXT("LowQualityButtonPlate"), TEXT("LowQualityButtonPlateMount") },
			{ TEXT("MediumQualityButtonPlate"), TEXT("MediumQualityButtonPlateMount") },
			{ TEXT("HighQualityButtonPlate"), TEXT("HighQualityButtonPlateMount") },
			{ TEXT("LanguageKoreanButtonPlate"), TEXT("LanguageKoreanButtonPlateMount") },
			{ TEXT("LanguageEnglishButtonPlate"), TEXT("LanguageEnglishButtonPlateMount") },
			{ TEXT("ConfirmAbandonButtonPlate"), TEXT("ConfirmAbandonButtonPlateMount") },
			{ TEXT("CancelAbandonButtonPlate"), TEXT("CancelAbandonButtonPlateMount") },
		};
		for (const FCanvasPlate& Plate : CanvasPlates)
		{
			AspectFitButtonPlate(Blueprint, Plate.PlateName,
				CanvasMountSize(Blueprint, Plate.MountName));
		}

		// RunActionsPanel is a 620x118 HorizontalBox. Each Fill child owns half,
		// while the actual buttons still fill that full 310x118 hit area.
		const FVector2D RunActionBounds(310.0f, 118.0f);
		AspectFitButtonPlate(Blueprint, TEXT("SaveAndExitButtonPlate"), RunActionBounds);
		AspectFitButtonPlate(Blueprint, TEXT("AbandonRunButtonPlate"), RunActionBounds);
		LogPhase(TEXT("buttons"));

		// Confirmation now uses the authored Marchbound detail panel at a fixed aspect.
		// Book/spine and generated labels are authored fixed illustrations. Turning
		// any of them into a 9-slice would bend the spine or duplicate brass rivets.
		for (const TCHAR* Name : {
			TEXT("Set_panel_body"), TEXT("Set_row_master_plate"),
			TEXT("Set_row_bgm_plate"), TEXT("Set_row_sfx_plate"),
			TEXT("Set_row_ui_plate"), TEXT("Set_row_shake_plate"),
			TEXT("Set_row_vibration_plate"), TEXT("Set_row_effects_plate"),
			TEXT("Set_row_quality_plate"), TEXT("Set_row_fps_plate"),
			TEXT("Set_row_language_plate"), TEXT("Set_slider_track_master"),
			TEXT("Set_slider_track_bgm"), TEXT("Set_slider_track_sfx"),
			TEXT("Set_slider_track_ui") })
		{
			NormalizeFixedImage(Blueprint, Name);
		}
		LogPhase(TEXT("brushes"));

		// UE 5.7 requires every live source widget to have a stable GUID.
		// ConstructWidget and reparenting do not populate this map for commandlet edits.
		Blueprint->WidgetTree->ForEachWidget([Blueprint](UWidget* Widget)
		{
			if (Widget == nullptr)
			{
				return;
			}

			const FName WidgetName = Widget->GetFName();
			if (!Blueprint->WidgetVariableNameToGuidMap.Contains(WidgetName))
			{
				Blueprint->OnVariableAdded(WidgetName);
			}
		});
		LogPhase(TEXT("guids"));

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		LogPhase(TEXT("compile-begin"));
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		LogPhase(TEXT("compile-end"));
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		LogPhase(TEXT("save-begin"));
		if (!UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename,
			FSavePackageArgs()))
		{
			UE_LOG(LogTemp, Error, TEXT("RD_SETTINGS_READABILITY_BUILD save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_SETTINGS_READABILITY_BUILD success font=project-composite alignment=center"));
	}
}

void RegisterSettingsPanelWidgetBuilderCommands()
{
	using namespace SettingsPanelWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildSettingsReadability"),
		TEXT("Build the Settings Ledger art, layout and project typography into WBP_SettingsPanel."),
		FConsoleCommandDelegate::CreateStatic(&Build));
	TextDefaultsCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.SyncSettingsTextDefaults"),
		TEXT("Sync WBP_SettingsPanel text defaults to the runtime localization keys."),
		FConsoleCommandDelegate::CreateStatic(&SyncTextDefaultsOnly));
}

void UnregisterSettingsPanelWidgetBuilderCommands()
{
	SettingsPanelWidgetBuilder::BuildCommand.Reset();
	SettingsPanelWidgetBuilder::TextDefaultsCommand.Reset();
}
