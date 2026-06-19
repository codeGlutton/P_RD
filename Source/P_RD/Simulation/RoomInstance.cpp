#include "Simulation/RoomInstance.h"
#include "FunctionLibrary/RandomStreamFunctionLibrary.h"

#include "ObjectModel.h"

void URoomInstance::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	if (DuplicateMode == EDuplicateMode::Normal)
	{
		mCopiedEventStream = URandomStreamFunctionLibrary::GetEventStream(GetWorld());
	}
}