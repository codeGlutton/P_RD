#include "UI/TitleMenuWidget.h"

#include "Components/Image.h"
#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "UI/UITextureLoader.h"

void UTitleMenuWidget::StartTitleBackgroundVideo()
{
	// 표시 Image는 WBP가 제공한다. 없으면 배경 영상은 생략(레이아웃은 WBP 책임).
	if (TitleBackgroundImage == nullptr)
	{
		return;
	}

	EnsureTitleBackgroundMediaObjects();
	ApplyTitleBackgroundVideoBrush();
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
}

void UTitleMenuWidget::ApplyTitleBackgroundVideoBrush()
{
	if (TitleBackgroundImage == nullptr || mBackgroundRuntime.mMediaTexture == nullptr)
	{
		return;
	}

	mBackgroundRuntime.mVideoBrush = FSlateBrush();
	mBackgroundRuntime.mVideoBrush.DrawAs = ESlateBrushDrawType::Image;
	mBackgroundRuntime.mVideoBrush.ImageSize = FVector2D(1280.0f, 720.0f);
	mBackgroundRuntime.mVideoBrush.SetResourceObject(mBackgroundRuntime.mMediaTexture);
	TitleBackgroundImage->SetBrush(mBackgroundRuntime.mVideoBrush);
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
