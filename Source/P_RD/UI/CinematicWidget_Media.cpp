// 이 파일: 인트로 MP4 영상 재생 담당. MediaPlayer/MediaTexture/FileMediaSource를 만들어
//          영상을 Slate 이미지에 그리고, 영상 열림→재생 / 실패·종료→FinishCinematic 으로 처리한다.
//          파일은 SVN로 동기화되는 mp4를 디스크에서 직접 연다(없으면 경고만 + 폴백 타이머로 진행).
#include "UI/CinematicWidget.h"

#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "Widgets/Images/SImage.h"

void UCinematicWidget::EnsureCinematicMediaObjects()
{
	if (mCinematicMediaPlayer == nullptr)
	{
		mCinematicMediaPlayer = NewObject<UMediaPlayer>(this, TEXT("IntroCinematicMediaPlayer"));
		mCinematicMediaPlayer->SetLooping(false);
		mCinematicMediaPlayer->OnMediaOpened.AddUniqueDynamic(this, &UCinematicWidget::HandleCinematicMediaOpened);
		mCinematicMediaPlayer->OnMediaOpenFailed.AddUniqueDynamic(this, &UCinematicWidget::HandleCinematicMediaOpenFailed);
		mCinematicMediaPlayer->OnEndReached.AddUniqueDynamic(this, &UCinematicWidget::HandleCinematicMediaEndReached);
	}

	if (mCinematicMediaTexture == nullptr)
	{
		mCinematicMediaTexture = NewObject<UMediaTexture>(this, TEXT("IntroCinematicMediaTexture"));
		mCinematicMediaTexture->AutoClear = true;
		mCinematicMediaTexture->ClearColor = FLinearColor::Black;
		mCinematicMediaTexture->SetMediaPlayer(mCinematicMediaPlayer);
		mCinematicMediaTexture->UpdateResource();
	}

	if (mCinematicMediaSource == nullptr)
	{
		mCinematicMediaSource = NewObject<UFileMediaSource>(this, TEXT("IntroCinematicMediaSource"));
		mCinematicMediaSource->PrecacheFile = true;
	}

	if (mCinematicVideoImage.IsValid())
	{
		mCinematicVideoBrush.SetResourceObject(mCinematicMediaTexture);
		mCinematicVideoImage->SetImage(&mCinematicVideoBrush);
	}
}

bool UCinematicWidget::PlayCinematicVideo()
{
	EnsureCinematicMediaObjects();

	if (mCinematicMediaPlayer == nullptr || mCinematicMediaSource == nullptr)
	{
		return false;
	}

	const FString VideoPath = ResolveCinematicVideoPath();
	if (FPaths::FileExists(VideoPath) == false)
	{
		UE_LOG(LogRD, Warning, TEXT("Intro cinematic file missing: %s"), *VideoPath);
		return false;
	}

	mCinematicMediaPlayer->Rewind();
	mCinematicMediaPlayer->Close();
	mCinematicMediaSource->SetFilePath(VideoPath);
	return mCinematicMediaPlayer->OpenSource(mCinematicMediaSource);
}

void UCinematicWidget::StopCinematicMedia()
{
	if (mCinematicMediaPlayer != nullptr)
	{
		mCinematicMediaPlayer->OnMediaOpened.RemoveAll(this);
		mCinematicMediaPlayer->OnMediaOpenFailed.RemoveAll(this);
		mCinematicMediaPlayer->OnEndReached.RemoveAll(this);
		mCinematicMediaPlayer->Close();
	}
}

FString UCinematicWidget::ResolveCinematicVideoPath() const
{
	if (FPaths::IsRelative(mCinematicVideoPath) == false)
	{
		return FPaths::ConvertRelativePathToFull(mCinematicVideoPath);
	}

	return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir(), mCinematicVideoPath);
}

void UCinematicWidget::HandleCinematicMediaOpened(FString OpenedUrl)
{
	if (mCinematicMediaPlayer != nullptr)
	{
		mCinematicMediaPlayer->Play();

		// 영상 실제 해상도를 한 번 확보해 cover 비율 기준으로 쓴다.
		// (미디어 텍스처 GetWidth/Height는 첫 프레임 디코드 전까지 0/부정확할 수 있어, 폴백 시 정사각(640x640) 영상이 16:9로 눌릴 수 있다.)
		const int32 VideoTrack = mCinematicMediaPlayer->GetSelectedTrack(EMediaPlayerTrack::Video);
		const int32 VideoFormat = mCinematicMediaPlayer->GetTrackFormat(EMediaPlayerTrack::Video, VideoTrack);
		const FIntPoint VideoDimensions = mCinematicMediaPlayer->GetVideoTrackDimensions(VideoTrack, VideoFormat);
		if (VideoDimensions.X > 0 && VideoDimensions.Y > 0)
		{
			mCinematicVideoNativeSize = FVector2D(VideoDimensions.X, VideoDimensions.Y);
		}

		const float MediaDurationSeconds = StaticCast<float>(mCinematicMediaPlayer->GetDuration().GetTotalSeconds());
		if (MediaDurationSeconds > 0.0f)
		{
			StartDefaultCinematicTimer(MediaDurationSeconds + 1.0f);
		}
		else
		{
			StartDefaultCinematicTimer(mDefaultCinematicDuration);
		}
	}
}

void UCinematicWidget::HandleCinematicMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogRD, Warning, TEXT("Intro cinematic media open failed: %s"), *FailedUrl);
	FinishCinematic();
}

void UCinematicWidget::HandleCinematicMediaEndReached()
{
	FinishCinematic();
}
