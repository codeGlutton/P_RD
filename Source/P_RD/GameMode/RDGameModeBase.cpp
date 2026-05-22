#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"

const UUserPersistData* ARDGameModeBase::GetUserPersistData() const
{
	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentDataSubsystem->GetUserPersistData();
}

const URunPersistData* ARDGameModeBase::GetRunPersistData() const
{
	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentDataSubsystem->GetRunPersistData();
}