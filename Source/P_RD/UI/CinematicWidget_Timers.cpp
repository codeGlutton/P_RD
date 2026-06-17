#include "UI/CinematicWidget.h"

void UCinematicWidget::StartDefaultCinematicTimer(float DurationSeconds)
{
	// 영상 파일을 열 수 없거나 길이를 얻지 못했을 때도 인트로가 영원히 멈추지 않게 하는 안전 타이머다.
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
