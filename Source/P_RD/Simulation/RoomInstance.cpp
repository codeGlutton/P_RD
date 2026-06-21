#include "Simulation/RoomInstance.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"

#include "ObjectModel.h"

void URoomInstance::CollectGameDatas()
{
	mCopiedData.Reset();

	/* 데이터 채우기 */

	{
		UPersistentDataSubsystem* PersistentSubsystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UPersistentDataSubsystem>();
		checkf(PersistentSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

		mEventStreamPtr = &PersistentSubsystem->GetRunPersistData()->GetEventStream();
	}
}

void URoomInstance::CollectSimulationDatas()
{
	mCopiedData.Reset();

	/* 복제본 생성 */

	mCopiedData.InitializeAs<FRoomCopyData>();
	FRoomCopyData& CopyData = mCopiedData.GetMutable();

	{
		UPersistentDataSubsystem* PersistentSubsystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UPersistentDataSubsystem>();
		checkf(PersistentSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

		CopyData.mCopiedEventStream = PersistentSubsystem->GetRunPersistData()->GetEventStream();
	}

	/* 데이터 채우기 */

	{
		mEventStreamPtr = &CopyData.mCopiedEventStream;
	}
}
