#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"

#include "Pawn/Unit.h"

#if WITH_EDITOR
void UStaticUnitSpawnData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UStaticUnitSpawnData, mClass))
	{
		mName = GetDefault<AUnit>(mClass.Get())->GetUnitDisplayName();
	}
}
#endif
