#include "UI/CinematicWidget.h"

#include "Engine/World.h"
#include "TimerManager.h"

UCinematicWidget::UCinematicWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mRemoveFromParentOnClose = true;
}

void UCinematicWidget::PlayCinematic(FOnEndCinematicAnimation Callback)
{
	if (IsOpened() == false)
	{
		return;
	}

	OnEndCinematicAnimation = MoveTemp(Callback);
	mCinematicFinished = false;
	PlayCinematicAnimation();
}

void UCinematicWidget::FinishCinematic()
{
	if (mCinematicFinished)
	{
		return;
	}

	mCinematicFinished = true;
	if (OnEndCinematicAnimation.IsBound())
	{
		OnEndCinematicAnimation.Execute(this);
		OnEndCinematicAnimation.Unbind();
	}
}

void UCinematicWidget::PlayCinematicAnimation_Implementation()
{
	if (mDefaultCinematicDuration <= 0.0f)
	{
		FinishCinematic();
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		FinishCinematic();
		return;
	}

	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() {
		FinishCinematic();
		}), mDefaultCinematicDuration, false);
}
