#include "Simulation/RoomInstance.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"

#include "ObjectModel.h"

void URoomInstance::CollectGameDatas()
{
	mCopiedData.Reset();

	/* 데이터 채우기 */

	UWorld* World = GetWorld();
	if (World != nullptr)
	{
		UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(World);
		if (GameInstance != nullptr)
		{
			UPersistentDataSubsystem* PersistentSubsystem = GameInstance->GetSubsystem<UPersistentDataSubsystem>();
			if (PersistentSubsystem != nullptr)
			{
				UE_LOG(LogRD, Log, TEXT("게임 룸 인스턴스 채우기 완료"));
				mEventStreamPtr = &PersistentSubsystem->GetRunPersistData()->GetEventStream();
			}
		}
	}
}

void URoomInstance::CollectSimulationDatas()
{
	mCopiedData.Reset();

	/* 복제본 생성 */

	mCopiedData.InitializeAs<FRoomCopyData>();
	FRoomCopyData& CopyData = mCopiedData.GetMutable();

	/* 데이터 채우기 */

	UWorld* World = GetWorld();
	if (World != nullptr)
	{
		UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(World);
		if (GameInstance != nullptr)
		{
			UPersistentDataSubsystem* PersistentSubsystem = GameInstance->GetSubsystem<UPersistentDataSubsystem>();
			if (PersistentSubsystem != nullptr)
			{
				UE_LOG(LogRD, Log, TEXT("시뮬 룸 인스턴스 채우기 완료"));
				CopyData.mCopiedEventStream = PersistentSubsystem->GetRunPersistData()->GetEventStream();
				mEventStreamPtr = &CopyData.mCopiedEventStream;
			}
		}
	}
}
