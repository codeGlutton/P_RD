#include "UI/TitleMenuWidget.h"

#include "Components/Image.h"
#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "UI/UITextureLoader.h"

void UTitleMenuWidget::StartTitleBackgroundVideo()
{
	/*
	 * 표시 Image는 WBP가 제공한다.
	 * C++이 Image를 새로 만들지 않는 이유는 타이틀의 정적 배치/해상도 대응을
	 * WBP 자산 한곳에서 검토하게 하기 위해서다. 없으면 영상만 생략한다.
	 */
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

	// WBP는 Image의 위치와 크기만 소유하고, 런타임 영상 리소스는 이 브러시로 주입한다.
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
		// WBP 배경 Image 자체는 남기고 영상 재생만 중단한다. 패키징 누락을 로그에서 찾기 위한 경고다.
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
