#include "UI/TitleMenuWidget.h"

#include "Components/Image.h"
#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "UI/UITextureLoader.h"

/** @brief WBP가 가진 배경 Image에 런타임 MediaTexture를 연결해 타이틀 루프 영상을 시작한다. */
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

/** @brief 타이틀 위젯이 소유하는 Transient 미디어 객체를 지연 생성하고 델리게이트를 연결한다. */
void UTitleMenuWidget::EnsureTitleBackgroundMediaObjects()
{
	/*
	 * Media 객체들은 WBP에 저장하지 않고 런타임 Transient로만 둔다.
	 * 배경 영상 파일 경로/재생 상태는 실행 환경에 묶이고, WBP는 화면 배치만 검토 가능한 상태로 남아야 한다.
	 */
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

/** @brief MediaTexture를 Slate Brush에 넣어 WBP 배경 Image에 반영한다. */
void UTitleMenuWidget::ApplyTitleBackgroundVideoBrush()
{
	if (TitleBackgroundImage == nullptr || mBackgroundRuntime.mMediaTexture == nullptr)
	{
		return;
	}

	// WBP는 Image의 위치와 크기만 소유하고, 런타임 영상 리소스는 이 브러시로 주입한다.
	mBackgroundRuntime.mVideoBrush = FSlateBrush();
	mBackgroundRuntime.mVideoBrush.DrawAs = ESlateBrushDrawType::Image;
	// [합의필요] 현재 타이틀 영상 기준 해상도다. 영상 교체 시 WBP 스케일 정책과 함께 조정해야 한다.
	mBackgroundRuntime.mVideoBrush.ImageSize = FVector2D(1280.0f, 720.0f);
	mBackgroundRuntime.mVideoBrush.SetResourceObject(mBackgroundRuntime.mMediaTexture);
	TitleBackgroundImage->SetBrush(mBackgroundRuntime.mVideoBrush);
}

/** @brief 파일 존재와 MediaSource 열기까지 책임지고, 실패해도 타이틀 UI 자체는 유지한다. */
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

/** @brief 위젯 Destruct 시 MediaPlayer 델리게이트와 파일 핸들을 정리한다. */
void UTitleMenuWidget::StopTitleBackgroundVideo()
{
	if (mBackgroundRuntime.mMediaPlayer != nullptr)
	{
		mBackgroundRuntime.mMediaPlayer->OnMediaOpened.RemoveAll(this);
		mBackgroundRuntime.mMediaPlayer->OnMediaOpenFailed.RemoveAll(this);
		mBackgroundRuntime.mMediaPlayer->Close();
	}
}

/** @brief Content 상대 경로를 플랫폼별 실제 파일 경로로 바꾼다. */
FString UTitleMenuWidget::ResolveTitleBackgroundVideoPath() const
{
	return RDUITexture::ResolveContentFilePath(mTitleBackgroundVideoPath);
}

/** @brief MediaPlayer가 파일을 연 뒤 실제 재생을 시작하는 비동기 완료 지점이다. */
void UTitleMenuWidget::HandleTitleBackgroundMediaOpened(FString OpenedUrl)
{
	if (mBackgroundRuntime.mMediaPlayer != nullptr)
	{
		mBackgroundRuntime.mMediaPlayer->SetLooping(true);
		mBackgroundRuntime.mMediaPlayer->Play();
	}

	UE_LOG(LogRD, Display, TEXT("TitleMenuWidget: title background media opened: %s"), *OpenedUrl);
}

/** @brief 미디어 파일 누락/코덱 실패를 화면 전환 실패로 승격하지 않고 로그에 남긴다. */
void UTitleMenuWidget::HandleTitleBackgroundMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: title background media open failed: %s"), *FailedUrl);
}
