#include "Singleton/WorldSubsystem/PresentationSyncSubsystem.h"

TSharedPtr<FPresentationBarrier> UPresentationSyncSubsystem::MakePresentationBarrier(FOnFinishPresentation OnFinishPresentation)
{
	return TSharedPtr<FPresentationBarrier>(
		new FPresentationBarrier(MoveTemp(OnFinishPresentation)),
		[](FPresentationBarrier* Barrier) {
			delete Barrier;
		}
	);
}
