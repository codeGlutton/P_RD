#include "Singleton/WorldSubsystem/PresentationSyncSubsystem.h"

DEFINE_LOG_CATEGORY(LogPresentationSync)

TSharedPtr<FPresentationBarrier> UPresentationSyncSubsystem::MakePresentationBarrier(FOnFinishPresentation OnFinishPresentation)
{
	UE_LOG(LogPresentationSync, Log, TEXT("연출 재생"));
	return TSharedPtr<FPresentationBarrier>(
		new FPresentationBarrier(MoveTemp(OnFinishPresentation)),
		[](FPresentationBarrier* Barrier) {
			UE_LOG(LogPresentationSync, Log, TEXT("연출 종료"));
			delete Barrier;
		}
	);
}
