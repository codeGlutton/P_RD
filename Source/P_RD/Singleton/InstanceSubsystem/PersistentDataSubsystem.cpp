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

UUserPersistData* UPersistentDataSubsystem::GetUserPersistData()
{
	return mUserPersistData;
}

URunPersistData* UPersistentDataSubsystem::GetRunPersistData()
{
	return mRunPersistData;
}

UOptionPersistData* UPersistentDataSubsystem::GetOptionPersistData()
{
	return mOptionPersistData;
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
		// 스테이지 짜는 것만 보는 자리라 누구를 데려가든 상관없다. 다만 빈
		// 파티로는 런이 시작되지 않으므로 아무나 한 명은 세워 준다.
		mRunPersistData->StartRun({ FPrimaryAssetId() }, 1);
	}
	mRunPersistData->MakeStageAsync(EStageLevelType::Stage1, FOnCreateStage());
#endif
}
