#include "Singleton/WorldSubsystem/SimulationSubsystem.h"

#include "Simulation/RoomContext.h"
#include "Simulation/RoomInstance.h"
#include "Simulation/Logger/EventLogger.h"
#include "Simulation/Factory/ObjectModelFactory.h"

#include "Setting/GamePlaySettings.h"

#include "ObjectModel.h"

DEFINE_LOG_CATEGORY(LogSimulation)

void USimulationSubsystem::PostInitialize()
{
	Super::PostInitialize();

	{
		mGameRoomContext.mRoomInstance = NewObject<URoomInstance>(this);
		mGameRoomContext.mRoomInstance->CollectGameDatas();

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

	Cast<UGameObjectModelFactory>(mGameRoomContext.mModelFactory)->RegisterSubsystemModels();
	SetSimulationState(mSimulationState);
}

void USimulationSubsystem::PreDeinitialize()
{
	Cast<UGameObjectModelFactory>(mGameRoomContext.mModelFactory)->UnregisterSubsystemModels();
	
	Super::PreDeinitialize();
}

TArray<FSRPGTurnEventLog> USimulationSubsystem::SimulateUntilNextAction()
{
	checkf(mSimulationState == ESRPGSimulationState::RunningGame, TEXT("이미 시뮬레이션 중"));
	SetSimulationState(ESRPGSimulationState::RunningSimulation);

	// TODO

	TArray<FSRPGTurnEventLog> ResultLogs = GetEventLogger().PopSRPGLogs();

	SetSimulationState(ESRPGSimulationState::RunningGame);
	return MoveTemp(ResultLogs);
}

TArray<FSRPGTurnEventLog> USimulationSubsystem::SimulateUntilNextPlayerTurn()
{
	checkf(mSimulationState == ESRPGSimulationState::RunningGame, TEXT("이미 시뮬레이션 중"));
	SetSimulationState(ESRPGSimulationState::RunningSimulation);

	// TODO

	TArray<FSRPGTurnEventLog> ResultLogs = GetEventLogger().PopSRPGLogs();

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
		mGameRoomContext.mRoomInstance->CollectSimulationDatas();
		mCurrentRoomContext = &mSimulationRoomContext;
	}
}

UObjectModel* USimulationSubsystem::GetSubsystemModel(UClass* Class) const
{
	TObjectPtr<UObjectModel>* FoundModel = mCurrentRoomContext->mRoomInstance->mAliveSubsystemModels.Find(Class);
	return FoundModel != nullptr ? *FoundModel : nullptr;
}

const FRandomStream& USimulationSubsystem::GetEventStream() const
{
	return *mCurrentRoomContext->mRoomInstance->mEventStreamPtr;
}

UEventLogger& USimulationSubsystem::GetEventLogger() const
{
	return *mCurrentRoomContext->mEventLogger;
}

UObjectModelFactory& USimulationSubsystem::GetModelFactory() const
{
	return *mCurrentRoomContext->mModelFactory;
}

ESRPGSimulationState USimulationSubsystem::GetSimulationState() const
{
	return mSimulationState;
}

