#include "RDMinimal.h"
#include "Singleton/WorldSubsystem/SimulationSubsystem.h"

UObjectModel* GetWorldSubsystemModel(const UObject* WorldContextObject, UClass* Class)
{
	if (IsValid(WorldContextObject) == false || Class == nullptr)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (IsValid(World) == false)
	{
		return nullptr;
	}

	USimulationSubsystem* SimulationSubsystem = World->GetSubsystem<USimulationSubsystem>();
    if (SimulationSubsystem == nullptr)
    {
        return nullptr;
    }

    return SimulationSubsystem->GetSubsystemModel(Class);
}

UObjectModelFactory* GetWorldModelFactory(const UObject* WorldContextObject)
{
	if (IsValid(WorldContextObject) == false)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (IsValid(World) == false)
	{
		return nullptr;
	}

	USimulationSubsystem* SimulationSubsystem = World->GetSubsystem<USimulationSubsystem>();
    if (SimulationSubsystem == nullptr)
    {
        return nullptr;
    }

    return &SimulationSubsystem->GetModelFactory();
}

UEventLogger* GetWorldEventLogger(const UObject* WorldContextObject)
{
	if (IsValid(WorldContextObject) == false)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (IsValid(World) == false)
	{
		return nullptr;
	}

	USimulationSubsystem* SimulationSubsystem = World->GetSubsystem<USimulationSubsystem>();
    if (SimulationSubsystem == nullptr)
    {
        return nullptr;
    }

    return &SimulationSubsystem->GetEventLogger();
}
