#include "UI/CinematicWidget.h"

UCinematicWidget::UCinematicWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UCinematicWidget::CloseUI(FOnEndUICloseAnimation Callback)
{
	PlaybackState = ECinematicPlaybackState::Stopped;
	Super::CloseUI(MoveTemp(Callback));
}

void UCinematicWidget::PlayCinematic(FOnEndCinematicAnimation Callback)
{
	if (!IsOpened())
	{
		return;
	}

	OnEndCinematicAnimation = MoveTemp(Callback);
	PlaybackState = ECinematicPlaybackState::Playing;
	PlayCinematicAnimation();
}

void UCinematicWidget::FinishCinematic()
{
	if (PlaybackState != ECinematicPlaybackState::Playing)
	{
		return;
	}

	PlaybackState = ECinematicPlaybackState::Stopped;
	if (OnEndCinematicAnimation.IsBound())
	{
		OnEndCinematicAnimation.Execute(this);
		OnEndCinematicAnimation.Unbind();
	}
}

void UCinematicWidget::PlayCinematicAnimation_Implementation()
{
	FinishCinematic();
}
