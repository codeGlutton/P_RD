#include "FunctionLibrary/RandomStreamFunctionLibrary.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"

const FRandomStream& URandomStreamFunctionLibrary::GetStageBuildStream(const UObject* WorldContextObject)
{
	UPersistentDataSubsystem* PersistentSubsystem = UGameplayStatics::GetGameInstance(WorldContextObject)->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentSubsystem->GetRunPersistData()->GetStageBuildStream();
}

const FRandomStream& URandomStreamFunctionLibrary::GetEventStream(const UObject* WorldContextObject)
{
	UPersistentDataSubsystem* PersistentSubsystem = UGameplayStatics::GetGameInstance(WorldContextObject)->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentSubsystem->GetRunPersistData()->GetEventStream();
}

const FRandomStream& URandomStreamFunctionLibrary::GetCombatStream(const UObject* WorldContextObject)
{
	UPersistentDataSubsystem* PersistentSubsystem = UGameplayStatics::GetGameInstance(WorldContextObject)->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentSubsystem->GetRunPersistData()->GetCombatStream();
}
