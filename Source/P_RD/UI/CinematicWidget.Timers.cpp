// 이 파일: 영상 종료 이벤트가 안 와도 인트로가 멈추지 않게 하는 "안전 타이머".
//          지정 시간이 지나면 FinishCinematic을 부른다. 영상 파일 누락·플랫폼 미디어 초기화 실패 대비.
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
