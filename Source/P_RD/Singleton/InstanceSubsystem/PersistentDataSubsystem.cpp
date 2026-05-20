#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

void UPersistentDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	mUserPersistData = NewObject<UUserPersistData>(this);
	mRunPersistData = NewObject<URunPersistData>(this);
}

const UUserPersistData* UPersistentDataSubsystem::GetUserPersistData() const
{
	return mUserPersistData;
}

const URunPersistData* UPersistentDataSubsystem::GetRunPersistData() const
{
	return mRunPersistData;
}