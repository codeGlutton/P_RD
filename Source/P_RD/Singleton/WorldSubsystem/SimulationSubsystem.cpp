#include "Singleton/WorldSubsystem/SimulationSubsystem.h"

#include "FunctionLibrary/RandomStreamFunctionLibrary.h"

#include "Simulation/RoomContext.h"
#include "Simulation/RoomInstance.h"
#include "Simulation/Logger/EventLogger.h"
#include "Simulation/Factory/ObjectModelFactory.h"

DEFINE_LOG_CATEGORY(LogSimulation)

void USimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	{
		mGameRoomContext.mRoomInstance = NewObject<URoomInstance>(this);

		mGameRoomContext.mModelFactory = NewObject<UGameObjectModelFactory>(this);
		mGameRoomContext.mModelFactory->SetContext(mGameRoomContext);

		mGameRoomContext.mEventLogger = NewObject<UGameEventLogger>(this);
		mGameRoomContext.mEventLogger->SetContext(mGameRoomContext);
	}

	{
		mSimulationRoomContext.mRoomInstance = nullptr;

		mSimulationRoomContext.mModelFactory = NewObject<USimulationObjectModelFactory>(this);
		mSimulationRoomContext.mModelFactory->SetContext(mGameRoomContext);

		mSimulationRoomContext.mEventLogger = NewObject<USimulationEventLogger>(this);
		mSimulationRoomContext.mEventLogger->SetContext(mGameRoomContext);
	}

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
		mSimulationRoomContext.mRoomInstance = nullptr;
		mCurrentRoomContext = &mGameRoomContext;
	}
	else
	{
		mSimulationRoomContext.mRoomInstance = Cast<URoomInstance>(StaticDuplicateObject(mSimulationRoomContext.mRoomInstance, this));
		mCurrentRoomContext = &mSimulationRoomContext;
	}
}

const FRandomStream& USimulationSubsystem::GetEventStream() const
{
	if (mSimulationState == ESRPGSimulationState::RunningGame)
	{
		return URandomStreamFunctionLibrary::GetEventStream(GetWorld());
	}
	else
	{
		return mSimulationRoomContext.mRoomInstance->mCopiedEventStream;
	}
}

UEventLogger* USimulationSubsystem::GetEventLogger() const
{
	if (mCurrentRoomContext == nullptr)
	{
		return nullptr;
	}
	return mCurrentRoomContext->mEventLogger;
}

UObjectModelFactory* USimulationSubsystem::GetModelFactory() const
{
	if (mCurrentRoomContext == nullptr)
	{
		return nullptr;
	}
	return mCurrentRoomContext->mModelFactory;
}

ESRPGSimulationState USimulationSubsystem::GetSimulationState() const
{
	return mSimulationState;
}

