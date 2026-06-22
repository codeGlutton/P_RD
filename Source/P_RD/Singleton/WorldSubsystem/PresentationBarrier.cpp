#include "Singleton/WorldSubsystem/PresentationBarrier.h"

TSharedPtr<FPresentationBarrier> FPresentationBarrier::Make(FOnFinishPresentation OnFinishPresentation)
{
	return TSharedPtr<FPresentationBarrier>(
		new FPresentationBarrier(MoveTemp(OnFinishPresentation)),
		[](FPresentationBarrier* Barrier) {
			delete Barrier;
		}
	);
}
