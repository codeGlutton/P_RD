#include "UI/TitleMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "UI/UITextureLoader.h"

void UTitleMenuWidget::EnsureTitleBackgroundVideo()
{
	EnsureTitleBackgroundMediaObjects();
	ApplyTitleBackgroundVideoBrush();

	if (AttachTitleBackgroundVideoImage() == false)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: Failed to attach title background video image."));
		return;
	}

	PlayTitleBackgroundVideo();
}

void UTitleMenuWidget::EnsureTitleBackgroundMediaObjects()
{
	if (mBackgroundRuntime.mMediaPlayer == nullptr)
	{
		mBackgroundRuntime.mMediaPlayer = NewObject<UMediaPlayer>(this, TEXT("TitleBackgroundMediaPlayer"));
	}

	if (mBackgroundRuntime.mMediaPlayer != nullptr)
	{
		mBackgroundRuntime.mMediaPlayer->SetLooping(true);
		mBackgroundRuntime.mMediaPlayer->OnMediaOpened.AddUniqueDynamic(this, &UTitleMenuWidget::HandleTitleBackgroundMediaOpened);
		mBackgroundRuntime.mMediaPlayer->OnMediaOpenFailed.AddUniqueDynamic(this, &UTitleMenuWidget::HandleTitleBackgroundMediaOpenFailed);
	}

	if (mBackgroundRuntime.mMediaTexture == nullptr)
	{
		mBackgroundRuntime.mMediaTexture = NewObject<UMediaTexture>(this, TEXT("TitleBackgroundMediaTexture"));
		mBackgroundRuntime.mMediaTexture->AutoClear = true;
		mBackgroundRuntime.mMediaTexture->ClearColor = FLinearColor::Black;
	}

	if (mBackgroundRuntime.mMediaTexture != nullptr)
	{
		mBackgroundRuntime.mMediaTexture->SetMediaPlayer(mBackgroundRuntime.mMediaPlayer);
		mBackgroundRuntime.mMediaTexture->UpdateResource();
	}

	if (mBackgroundRuntime.mMediaSource == nullptr)
	{
		mBackgroundRuntime.mMediaSource = NewObject<UFileMediaSource>(this, TEXT("TitleBackgroundMediaSource"));
		mBackgroundRuntime.mMediaSource->PrecacheFile = true;
	}

	if (mBackgroundRuntime.mVideoImage == nullptr && WidgetTree != nullptr)
	{
		mBackgroundRuntime.mVideoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RuntimeTitleBackgroundVideo"));
		mBackgroundRuntime.mVideoImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

bool UTitleMenuWidget::AttachTitleBackgroundVideoImage()
{
	if (mBackgroundRuntime.mVideoImage == nullptr)
	{
		return false;
	}

	if (mBackgroundRuntime.mVideoImage->Slot != nullptr)
	{
		return true;
	}

	if (UCanvasPanel* StartCanvas = Cast<UCanvasPanel>(StartScreen))
	{
		return AttachTitleBackgroundVideoImageToCanvas(StartCanvas);
	}

	if (UOverlay* StartOverlay = Cast<UOverlay>(StartScreen))
	{
		const int32 InsertIndex = FMath::Min(1, StartOverlay->GetChildrenCount());
		if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(StartOverlay->InsertChildAt(InsertIndex, mBackgroundRuntime.mVideoImage)))
		{
			OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(VAlign_Fill);
			OverlaySlot->SetPadding(FMargin(0.0f));
			return true;
		}
	}

	UWidget* RootWidget = WidgetTree != nullptr ? WidgetTree->RootWidget : nullptr;
	if (UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(RootWidget))
	{
		return AttachTitleBackgroundVideoImageToCanvas(RootCanvas);
	}

	if (UOverlay* RootOverlay = Cast<UOverlay>(RootWidget))
	{
		const int32 InsertIndex = FMath::Min(1, RootOverlay->GetChildrenCount());
		if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(RootOverlay->InsertChildAt(InsertIndex, mBackgroundRuntime.mVideoImage)))
		{
			OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(VAlign_Fill);
			OverlaySlot->SetPadding(FMargin(0.0f));
			return true;
		}
	}

	return false;
}

bool UTitleMenuWidget::AttachTitleBackgroundVideoImageToCanvas(UCanvasPanel* TargetCanvas)
{
	if (TargetCanvas == nullptr || mBackgroundRuntime.mVideoImage == nullptr)
	{
		return false;
	}

	const int32 ChildCountBeforeAdd = TargetCanvas->GetChildrenCount();
	UCanvasPanelSlot* BackgroundSlot = TargetCanvas->AddChildToCanvas(mBackgroundRuntime.mVideoImage);
	if (BackgroundSlot == nullptr)
	{
		return false;
	}

	BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(0.0f));
	BackgroundSlot->SetAlignment(FVector2D::ZeroVector);
	BackgroundSlot->SetZOrder(1);

	for (int32 ChildIndex = 0; ChildIndex < ChildCountBeforeAdd; ++ChildIndex)
	{
		UWidget* ChildWidget = TargetCanvas->GetChildAt(ChildIndex);
		UCanvasPanelSlot* ChildSlot = ChildWidget != nullptr ? Cast<UCanvasPanelSlot>(ChildWidget->Slot) : nullptr;
		if (ChildWidget == nullptr || ChildSlot == nullptr)
		{
			continue;
		}

		const bool bLikelyFlatBackground = ChildWidget->IsA<UImage>() || ChildWidget->IsA<UBorder>();
		if (bLikelyFlatBackground)
		{
			continue;
		}

		ChildSlot->SetZOrder(FMath::Max(ChildSlot->GetZOrder(), 10));
	}

	return true;
}

void UTitleMenuWidget::ApplyTitleBackgroundVideoBrush()
{
	if (mBackgroundRuntime.mVideoImage == nullptr || mBackgroundRuntime.mMediaTexture == nullptr)
	{
		return;
	}

	mBackgroundRuntime.mVideoBrush = FSlateBrush();
	mBackgroundRuntime.mVideoBrush.DrawAs = ESlateBrushDrawType::Image;
	mBackgroundRuntime.mVideoBrush.ImageSize = FVector2D(1280.0f, 720.0f);
	mBackgroundRuntime.mVideoBrush.SetResourceObject(mBackgroundRuntime.mMediaTexture);
	mBackgroundRuntime.mVideoImage->SetBrush(mBackgroundRuntime.mVideoBrush);
}

void UTitleMenuWidget::PlayTitleBackgroundVideo()
{
	EnsureTitleBackgroundMediaObjects();

	if (mBackgroundRuntime.mMediaPlayer == nullptr || mBackgroundRuntime.mMediaSource == nullptr)
	{
		return;
	}

	const FString VideoPath = ResolveTitleBackgroundVideoPath();
	if (FPaths::FileExists(VideoPath) == false)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: title background video file missing: %s"), *VideoPath);
		return;
	}

	mBackgroundRuntime.mMediaPlayer->SetLooping(true);
	mBackgroundRuntime.mMediaPlayer->Close();
	mBackgroundRuntime.mMediaSource->SetFilePath(VideoPath);

	if (mBackgroundRuntime.mMediaPlayer->OpenSource(mBackgroundRuntime.mMediaSource) == false)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: failed to open title background video: %s"), *VideoPath);
	}
}

void UTitleMenuWidget::StopTitleBackgroundVideo()
{
	if (mBackgroundRuntime.mMediaPlayer != nullptr)
	{
		mBackgroundRuntime.mMediaPlayer->OnMediaOpened.RemoveAll(this);
		mBackgroundRuntime.mMediaPlayer->OnMediaOpenFailed.RemoveAll(this);
		mBackgroundRuntime.mMediaPlayer->Close();
	}
}

FString UTitleMenuWidget::ResolveTitleBackgroundVideoPath() const
{
	return RDUITexture::ResolveContentFilePath(mTitleBackgroundVideoPath);
}

void UTitleMenuWidget::HandleTitleBackgroundMediaOpened(FString OpenedUrl)
{
	if (mBackgroundRuntime.mMediaPlayer != nullptr)
	{
		mBackgroundRuntime.mMediaPlayer->SetLooping(true);
		mBackgroundRuntime.mMediaPlayer->Play();
	}

	UE_LOG(LogRD, Display, TEXT("TitleMenuWidget: title background media opened: %s"), *OpenedUrl);
}

void UTitleMenuWidget::HandleTitleBackgroundMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: title background media open failed: %s"), *FailedUrl);
}
