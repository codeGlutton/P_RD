#include "UI/CinematicWidget.h"

UCinematicWidget::UCinematicWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bRemoveFromParentOnClose = true;
}

void UCinematicWidget::PlayCinematic(FOnEndCinematicAnimation Callback)
{
	if (IsOpened() == false)
	{
		return;
	}

	OnEndCinematicAnimation = MoveTemp(Callback);
	bCinematicFinished = false;
	PlayCinematicAnimation();
}

void UCinematicWidget::FinishCinematic()
{
	if (bCinematicFinished)
	{
		return;
	}

	bCinematicFinished = true;
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
