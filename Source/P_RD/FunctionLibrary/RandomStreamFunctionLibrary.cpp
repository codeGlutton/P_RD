#include "FunctionLibrary/RandomStreamFunctionLibrary.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

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

int32 URandomStreamFunctionLibrary::GetRandomFromInterval(const FRandomStream& Stream, const FInt32Interval& Interval)
{
	return Stream.RandRange(Interval.Min, Interval.Max);
}

float URandomStreamFunctionLibrary::GetRandomFromInterval(const FRandomStream& Stream, const FFloatInterval& Interval)
{
	return Stream.RandRange(Interval.Min, Interval.Max);
}

