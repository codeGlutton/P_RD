#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

void UPersistentDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	mUserPersistData = NewObject<UUserPersistData>(this);
	mRunPersistData = NewObject<URunPersistData>(this);
	mOptionPersistData = NewObject<UOptionPersistData>(this);

	mOptionPersistData->MakeCaches();
}

const UUserPersistData* UPersistentDataSubsystem::GetUserPersistData() const
{
	return mUserPersistData;
}

const URunPersistData* UPersistentDataSubsystem::GetRunPersistData() const
{
	return mRunPersistData;
}

const UOptionPersistData* UPersistentDataSubsystem::GetOptionPersistData() const
{
	return mOptionPersistData;
}

void UPersistentDataSubsystem::DoStageBuildTest(bool UpdateBuildStream)
{
#if WITH_EDITOR
	if (UpdateBuildStream == true)
	{
		mRunPersistData->StartRun(FPrimaryAssetId(), 1);
	}
	mRunPersistData->MakeStageAsync(EStageLevelType::Stage1, FOnCreateStage());
#endif
}
