#include "UI/CinematicWidget.h"

void UCinematicWidget::StartDefaultCinematicTimer(float DurationSeconds)
{
	if (DurationSeconds <= 0.0f)
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

	World->GetTimerManager().SetTimer(
		mDefaultCinematicTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			FinishCinematic();
		}),
		DurationSeconds,
		false
	);
}

void UCinematicWidget::ClearDefaultCinematicTimer()
{
	UWorld* World = GetWorld();
	if (World != nullptr)
	{
		World->GetTimerManager().ClearTimer(mDefaultCinematicTimerHandle);
	}
	mDefaultCinematicTimerHandle.Invalidate();
}
