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
