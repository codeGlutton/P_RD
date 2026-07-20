#include "FunctionLibrary/RandomStreamFunctionLibrary.h"
#include "Engine/GameInstance.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/WorldSubsystem/SimulationSubsystem.h"

const FRandomStream& URandomStreamFunctionLibrary::GetStageBuildStream(const UObject* WorldContextObject)
{
	UPersistentDataSubsystem* PersistentSubsystem = UGameplayStatics::GetGameInstance(WorldContextObject)->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentSubsystem->GetRunPersistData()->GetStageBuildStream();
}

const FRandomStream& URandomStreamFunctionLibrary::GetEventStream(const UObject* WorldContextObject)
{
	USimulationSubsystem* SimulationSubsystem = WorldContextObject->GetWorld()->GetSubsystem<USimulationSubsystem>();
	checkf(SimulationSubsystem != nullptr, TEXT("시뮬레이션 서브시스템 nullptr"));

	return SimulationSubsystem->GetEventStream();
}

int32 URandomStreamFunctionLibrary::GetRandomFromInterval(const FRandomStream& Stream, const FInt32Interval& Interval)
{
	return Stream.RandRange(Interval.Min, Interval.Max);
}

float URandomStreamFunctionLibrary::GetRandomFromInterval(const FRandomStream& Stream, const FFloatInterval& Interval)
{
	return Stream.RandRange(Interval.Min, Interval.Max);
}

