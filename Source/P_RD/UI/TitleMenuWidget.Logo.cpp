#include "UI/TitleMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Engine/Texture2D.h"
#include "UI/UITextureLoader.h"

void UTitleMenuWidget::EnsureTitleLogoOverlay()
{
	HideLegacyTitleWidgets();
	EnsureTitleLogoObjects();
	ApplyTitleLogoBrush();

	if (AttachTitleLogoImage() == false)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: Failed to attach title logo image."));
	}
}

void UTitleMenuWidget::EnsureTitleLogoObjects()
{
	if (mLogoRuntime.mTexture == nullptr)
	{
		mLogoRuntime.mTexture = RDUITexture::LoadTextureFromContentPng(mTitleLogoImagePath, TEXT("TitleMenuWidget"));
	}

	if (mLogoRuntime.mImage == nullptr && WidgetTree != nullptr)
	{
		mLogoRuntime.mImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RuntimeTitleLogoImage"));
		mLogoRuntime.mImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

bool UTitleMenuWidget::AttachTitleLogoImage()
{
	if (mLogoRuntime.mImage == nullptr)
	{
		return false;
	}

	if (mLogoRuntime.mImage->Slot != nullptr)
	{
		return true;
	}

	if (UCanvasPanel* StartCanvas = Cast<UCanvasPanel>(StartScreen))
	{
		return AttachTitleLogoImageToCanvas(StartCanvas);
	}

	if (UOverlay* StartOverlay = Cast<UOverlay>(StartScreen))
	{
		if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(StartOverlay->AddChildToOverlay(mLogoRuntime.mImage)))
		{
			OverlaySlot->SetHorizontalAlignment(HAlign_Center);
			OverlaySlot->SetVerticalAlignment(VAlign_Center);
			OverlaySlot->SetPadding(FMargin(0.0f, -140.0f, 0.0f, 0.0f));
			return true;
		}
	}

	UWidget* RootWidget = WidgetTree != nullptr ? WidgetTree->RootWidget : nullptr;
	if (UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(RootWidget))
	{
		return AttachTitleLogoImageToCanvas(RootCanvas);
	}

	if (UOverlay* RootOverlay = Cast<UOverlay>(RootWidget))
	{
		if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(RootOverlay->AddChildToOverlay(mLogoRuntime.mImage)))
		{
			OverlaySlot->SetHorizontalAlignment(HAlign_Center);
			OverlaySlot->SetVerticalAlignment(VAlign_Center);
			OverlaySlot->SetPadding(FMargin(0.0f, -140.0f, 0.0f, 0.0f));
			return true;
		}
	}

	return false;
}

bool UTitleMenuWidget::AttachTitleLogoImageToCanvas(UCanvasPanel* TargetCanvas)
{
	if (TargetCanvas == nullptr || mLogoRuntime.mImage == nullptr)
	{
		return false;
	}

	UCanvasPanelSlot* LogoSlot = TargetCanvas->AddChildToCanvas(mLogoRuntime.mImage);
	if (LogoSlot == nullptr)
	{
		return false;
	}

	LogoSlot->SetAnchors(FAnchors(0.50f, 0.39f, 0.50f, 0.39f));
	LogoSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	LogoSlot->SetPosition(FVector2D::ZeroVector);
	LogoSlot->SetSize(FVector2D(640.0f, 322.0f));
	LogoSlot->SetZOrder(20);

	return true;
}

void UTitleMenuWidget::ApplyTitleLogoBrush()
{
	if (mLogoRuntime.mImage == nullptr || mLogoRuntime.mTexture == nullptr)
	{
		return;
	}

	mLogoRuntime.mBrush = FSlateBrush();
	mLogoRuntime.mBrush.DrawAs = ESlateBrushDrawType::Image;
	mLogoRuntime.mBrush.ImageSize = FVector2D(
		static_cast<float>(mLogoRuntime.mTexture->GetSizeX()),
		static_cast<float>(mLogoRuntime.mTexture->GetSizeY()));
	mLogoRuntime.mBrush.SetResourceObject(mLogoRuntime.mTexture);
	mLogoRuntime.mImage->SetBrush(mLogoRuntime.mBrush);
}

void UTitleMenuWidget::HideLegacyTitleWidgets() const
{
	const FName LegacyTitleWidgetNames[] = {
		TEXT("TitleText"),
		TEXT("TitleBlackBackground"),
		TEXT("TitleFormatFrame"),
		TEXT("TitleFrameBackground"),
		TEXT("TitleFrameSlashA"),
		TEXT("TitleFrameSlashB"),
		TEXT("TitleFrameSlashC"),
		TEXT("TitleSubtitleText"),
		TEXT("TitleSubtitleBackground"),
		TEXT("TitleSubtitleFrame"),
		TEXT("TitleSubtitleSlashA"),
		TEXT("TitleSubtitleSlashB"),
		TEXT("TitleSubtitleSlashC"),
	};

	for (const FName& WidgetName : LegacyTitleWidgetNames)
	{
		if (UWidget* LegacyWidget = WidgetTree != nullptr ? WidgetTree->FindWidget(WidgetName) : nullptr)
		{
			LegacyWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
