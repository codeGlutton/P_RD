#include "UI/SettingsPanelWidgetBuilder.h"
#include "UI/UIFont.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
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
		Surface->SetBrushColor(FLinearColor(0.16f, 0.07f, 0.02f, 0.16f));
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
		Text->SetAutoWrapText(false);
		Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void LayoutSettingsContent(UWidgetBlueprint* Blueprint)
	{
		UCanvasPanel* DesignCanvas = EnsureSettingsDesignCanvas(Blueprint);
		if (DesignCanvas == nullptr)
		{
			return;
		}
		PlaceInSettingsCanvas(Blueprint, DesignCanvas, TEXT("Set_panel_bodyMount"),
			FVector2D(300.f, 140.f), FVector2D(1320.f, 800.f), 10,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInSettingsCanvas(Blueprint, DesignCanvas, TEXT("AbandonConfirmPanel"),
			FVector2D::ZeroVector, FVector2D(1920.f, 1080.f), 100,
			ESlateVisibility::Collapsed);

		UCanvasPanel* Canvas = EnsureSettingsContentCanvas(Blueprint);
		if (Canvas == nullptr)
		{
			return;
		}

		constexpr float LeftX = 55.f;
		constexpr float RightX = 685.f;
		constexpr float RowWidth = 580.f;
		constexpr float RowHeight = 62.f;
		constexpr float RowPitch = 70.f;
		constexpr float RowStartY = 160.f;

		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("SettingsTitleText_Center"),
			FVector2D(440.f, 24.f), FVector2D(440.f, 72.f), 5,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("AudioSectionHeader_Center"),
			FVector2D(LeftX, 108.f), FVector2D(RowWidth, 44.f), 4,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("DisplaySectionHeader_Center"),
			FVector2D(RightX, 108.f), FVector2D(RowWidth, 44.f), 4,
			ESlateVisibility::SelfHitTestInvisible);

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
		UTexture2D* SliderFillTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/"
				"T_KitA_Slider_Fill.T_KitA_Slider_Fill"));
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(SliderRows); ++Index)
		{
			const float Y = RowStartY + Index * RowPitch;
			const FSliderRow& Row = SliderRows[Index];
			EnsureRowSurface(Blueprint, Canvas,
				FName(*(FString(Row.Plate) + TEXT("_Surface"))),
				FVector2D(LeftX, Y), FVector2D(RowWidth, RowHeight));
			PlaceInSettingsCanvas(Blueprint, Canvas, Row.Plate,
				FVector2D(LeftX, Y), FVector2D(RowWidth, RowHeight), 0,
				ESlateVisibility::Collapsed);
			PlaceInSettingsCanvas(Blueprint, Canvas, Row.Label,
				FVector2D(LeftX + 18.f, Y + 6.f), FVector2D(185.f, 50.f), 3,
				ESlateVisibility::SelfHitTestInvisible);
			PlaceInSettingsCanvas(Blueprint, Canvas, Row.Track,
				FVector2D(LeftX + 220.f, Y + 6.64f), FVector2D(330.f, 48.72f), 1,
				ESlateVisibility::SelfHitTestInvisible);
			if (SliderFillTexture != nullptr)
			{
				EnsureSliderFill(Blueprint, Canvas, Row.Fill, SliderFillTexture,
					FVector2D(LeftX + 232.f, Y + 15.57f),
					FVector2D(306.f, 30.86f));
			}
			PlaceInSettingsCanvas(Blueprint, Canvas, Row.Slider,
				FVector2D(LeftX + 220.f, Y + 4.f), FVector2D(330.f, 54.f), 3,
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
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(ToggleRows); ++Index)
		{
			const float Y = RowStartY + Index * RowPitch;
			const FToggleRow& Row = ToggleRows[Index];
			EnsureRowSurface(Blueprint, Canvas,
				FName(*(FString(Row.Plate) + TEXT("_Surface"))),
				FVector2D(RightX, Y), FVector2D(RowWidth, RowHeight));
			PlaceInSettingsCanvas(Blueprint, Canvas, Row.Plate,
				FVector2D(RightX, Y), FVector2D(RowWidth, RowHeight), 0,
				ESlateVisibility::Collapsed);
			PlaceInSettingsCanvas(Blueprint, Canvas, Row.Label,
				FVector2D(RightX + 18.f, Y + 6.f), FVector2D(380.f, 50.f), 3,
				ESlateVisibility::SelfHitTestInvisible);
			PlaceScaledControl(Blueprint, Canvas, Row.CheckBox,
				FVector2D(RightX + 512.f, Y + 6.f), FVector2D(50.f, 50.f), 2);
		}

		// Vibration has no persistence or platform implementation yet. Keep its
		// compatibility widgets, but do not advertise a switch that cannot work.
		for (const TCHAR* Name : { TEXT("Set_row_vibration_plate"),
			TEXT("VibrationRow_Label_Center"), TEXT("VibrationCheckBox") })
		{
			CollapseNamed(Blueprint, Name);
		}

		const float QualityY = RowStartY + 2.f * RowPitch;
		EnsureRowSurface(Blueprint, Canvas, TEXT("SettingsQualityRowSurface"),
			FVector2D(RightX, QualityY), FVector2D(RowWidth, RowHeight));
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("Set_row_quality_plate"),
			FVector2D(RightX, QualityY), FVector2D(RowWidth, RowHeight), 0,
			ESlateVisibility::Collapsed);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("QualityRow_Label_Center"),
			FVector2D(RightX + 18.f, QualityY + 6.f), FVector2D(165.f, 50.f), 3,
			ESlateVisibility::SelfHitTestInvisible);
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const TCHAR* MountNames[] = { TEXT("LowQualityButtonPlateMount"),
				TEXT("MediumQualityButtonPlateMount"), TEXT("HighQualityButtonPlateMount") };
			PlaceInSettingsCanvas(Blueprint, Canvas, MountNames[Index],
				FVector2D(RightX + 190.f + Index * 128.f, QualityY + 3.f),
				FVector2D(120.f, 56.f), 2, ESlateVisibility::SelfHitTestInvisible);
		}

		const float FpsY = RowStartY + 3.f * RowPitch;
		EnsureRowSurface(Blueprint, Canvas, TEXT("SettingsFpsRowSurface"),
			FVector2D(RightX, FpsY), FVector2D(RowWidth, RowHeight));
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("Set_row_fps_plate"),
			FVector2D(RightX, FpsY), FVector2D(RowWidth, RowHeight), 0,
			ESlateVisibility::Collapsed);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("FpsRow_Label_Center"),
			FVector2D(RightX + 18.f, FpsY + 6.f), FVector2D(190.f, 50.f), 3,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("FpsThirtyButtonPlateMount"),
			FVector2D(RightX + 238.f, FpsY + 3.f), FVector2D(155.f, 56.f), 2,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("FpsSixtyButtonPlateMount"),
			FVector2D(RightX + 405.f, FpsY + 3.f), FVector2D(155.f, 56.f), 2,
			ESlateVisibility::SelfHitTestInvisible);

		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("GameplaySectionHeader_Center"),
			FVector2D(LeftX, 448.f), FVector2D(RowWidth, 44.f), 4,
			ESlateVisibility::SelfHitTestInvisible);
		EnsureRowSurface(Blueprint, Canvas, TEXT("SettingsLanguageRowSurface"),
			FVector2D(LeftX, 500.f), FVector2D(RowWidth, RowHeight));
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("Set_row_language_plate"),
			FVector2D(LeftX, 500.f), FVector2D(RowWidth, RowHeight), 0,
			ESlateVisibility::Collapsed);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("LanguageRow_Label_Center"),
			FVector2D(LeftX + 18.f, 506.f), FVector2D(180.f, 50.f), 3,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("LanguageKoreanButtonPlateMount"),
			FVector2D(LeftX + 210.f, 503.f), FVector2D(170.f, 56.f), 2,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("LanguageEnglishButtonPlateMount"),
			FVector2D(LeftX + 392.f, 503.f), FVector2D(170.f, 56.f), 2,
			ESlateVisibility::SelfHitTestInvisible);

		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("StatusText_Center"),
			FVector2D(360.f, 628.f), FVector2D(600.f, 38.f), 4,
			ESlateVisibility::SelfHitTestInvisible);

		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("BackButtonPlateMount"),
			FVector2D(55.f, 700.f), FVector2D(220.f, 70.f), 2,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("ResetButtonPlateMount"),
			FVector2D(285.f, 700.f), FVector2D(220.f, 70.f), 2,
			ESlateVisibility::SelfHitTestInvisible);
		PlaceInSettingsCanvas(Blueprint, Canvas, TEXT("RunActionsPanel"),
			FVector2D(665.f, 700.f), FVector2D(600.f, 70.f), 2,
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
		};
		const FTextFitSpec TextFits[] = {
			{ TEXT("BackButtonText"), TEXT("BackButtonText_Center") },
			{ TEXT("ResetButtonText"), TEXT("ResetButtonText_Center") },
			{ TEXT("FpsThirtyButtonText"), TEXT("FpsThirtyButtonText_Center") },
			{ TEXT("FpsSixtyButtonText"), TEXT("FpsSixtyButtonText_Center") },
			{ TEXT("LowQualityButtonText"), TEXT("LowQualityButtonText_Center") },
			{ TEXT("MediumQualityButtonText"), TEXT("MediumQualityButtonText_Center") },
			{ TEXT("HighQualityButtonText"), TEXT("HighQualityButtonText_Center") },
			{ TEXT("LanguageKoreanButtonText"), TEXT("LanguageKoreanButtonText_Center") },
			{ TEXT("LanguageEnglishButtonText"), TEXT("LanguageEnglishButtonText_Center") },
			{ TEXT("SaveAndExitButtonText"), TEXT("Set_run_SaveAndExitButton") },
			{ TEXT("AbandonRunButtonText"), TEXT("Set_run_AbandonRunButton") },
			{ TEXT("ConfirmAbandonButtonText"), TEXT("ConfirmAbandonButtonText_Center") },
			{ TEXT("CancelAbandonButtonText"), TEXT("CancelAbandonButtonText_Center") },
		};
		for (const FTextFitSpec& Fit : TextFits)
		{
			EnsureButtonTextFit(Blueprint, Fit.Text, Fit.Container);
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
		Font.OutlineSettings.OutlineSize = Size >= 28 ? 1 : 0;
		Font.OutlineSettings.OutlineColor =
			FLinearColor(0.03f, 0.015f, 0.005f, 1.f);
		Text->SetFont(Font);

		// Keep every settings label pure white. Selection state remains functional,
		// but is not communicated by changing the text tint.
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Text->SetShadowOffset(FVector2D(1.5f, 1.5f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .62f));
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
		LogPhase(TEXT("laid-out"));

		// RunActionsPanel is 600x70. Its two children and the actual transparent
		// buttons fill equal 300x70 functional targets.
		FillRunActionSlot(Blueprint, TEXT("Set_run_SaveAndExitButton"),
			TEXT("SaveAndExitButton"));
		FillRunActionSlot(Blueprint, TEXT("Set_run_AbandonRunButton"),
			TEXT("AbandonRunButton"));

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

		// RunActionsPanel is a 600x70 HorizontalBox. Each Fill child owns half, while
		// the actual buttons still fill that full 300x70 hit area.
		const FVector2D RunActionBounds(300.0f, 70.0f);
		AspectFitButtonPlate(Blueprint, TEXT("SaveAndExitButtonPlate"), RunActionBounds);
		AspectFitButtonPlate(Blueprint, TEXT("AbandonRunButtonPlate"), RunActionBounds);
		LogPhase(TEXT("buttons"));

		// These are intentionally resizable surfaces. 9-slice is the only stretching
		// allowed: corner pixels retain their source proportions while the center tiles
		// cover the functional panel/row/slider bounds.
		for (const TCHAR* Name : {
			TEXT("Set_panel_body"), TEXT("Set_confirm_plate"),
			TEXT("Set_row_master_plate"), TEXT("Set_row_bgm_plate"),
			TEXT("Set_row_sfx_plate"), TEXT("Set_row_ui_plate"),
			TEXT("Set_row_shake_plate"), TEXT("Set_row_vibration_plate"),
			TEXT("Set_row_effects_plate"), TEXT("Set_row_quality_plate"),
			TEXT("Set_row_fps_plate"), TEXT("Set_row_language_plate") })
		{
			NormalizeNineSlice(Blueprint, Name);
		}
		for (const TCHAR* Name : { TEXT("Set_slider_track_master"),
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
		TEXT("Apply the project composite font, standard colors and centered text to WBP_SettingsPanel."),
		FConsoleCommandDelegate::CreateStatic(&Build));
}

void UnregisterSettingsPanelWidgetBuilderCommands()
{
	SettingsPanelWidgetBuilder::BuildCommand.Reset();
}
