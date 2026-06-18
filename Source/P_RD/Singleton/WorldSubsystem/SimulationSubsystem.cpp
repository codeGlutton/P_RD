#include "Singleton/WorldSubsystem/SimulationSubsystem.h"

#include "Simulation/RoomContext.h"
#include "Simulation/Logger/EventLogger.h"
#include "Simulation/Factory/ModelFactory.h"

DEFINE_LOG_CATEGORY(LogSimulation)

void USimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	mGameRoomContext.mModelFactory = NewObject<UGameModelFactory>();
	mGameRoomContext.mEventLogger = NewObject<UGameEventLogger>();

	mSimulationRoomContext.mModelFactory = NewObject<USimulationModelFactory>();
	mSimulationRoomContext.mEventLogger = NewObject<USimulationEventLogger>();

	SetSimulationState(mSimulationState);
}

TArray<FSRPGTurnEventLog> USimulationSubsystem::SimulateUntilNextAction()
{
	checkf(mSimulationState == ESRPGSimulationState::RunningGame, TEXT("이미 시뮬레이션 중"));
	SetSimulationState(ESRPGSimulationState::RunningSimulation);

	// TODO

	TArray<FSRPGTurnEventLog> ResultLogs = GetEventLogger()->PopSRPGLogs();

	SetSimulationState(ESRPGSimulationState::RunningGame);
	return MoveTemp(ResultLogs);
}

TArray<FSRPGTurnEventLog> USimulationSubsystem::SimulateUntilNextPlayerTurn()
{
	checkf(mSimulationState == ESRPGSimulationState::RunningGame, TEXT("이미 시뮬레이션 중"));
	SetSimulationState(ESRPGSimulationState::RunningSimulation);

	// TODO

	TArray<FSRPGTurnEventLog> ResultLogs = GetEventLogger()->PopSRPGLogs();

	SetSimulationState(ESRPGSimulationState::RunningGame);
	return MoveTemp(ResultLogs);
}

void USimulationSubsystem::SetSimulationState(ESRPGSimulationState State)
{
	mSimulationState = State;
	if (State == ESRPGSimulationState::RunningGame)
	{
		mCurrentRoomContext = &mGameRoomContext;
	}
	else
	{
		mCurrentRoomContext = &mSimulationRoomContext;
	}
}

IEventLogger* USimulationSubsystem::GetEventLogger() const
{
	if (mCurrentRoomContext == nullptr)
	{
		return nullptr;
	}
	return mCurrentRoomContext->mEventLogger.GetInterface();
}

IModelFactory* USimulationSubsystem::GetModelFactory() const
{
	if (mCurrentRoomContext == nullptr)
	{
		return nullptr;
	}
	return mCurrentRoomContext->mModelFactory.GetInterface();
}

ESRPGSimulationState USimulationSubsystem::GetSimulationState() const
{
	return mSimulationState;
}

