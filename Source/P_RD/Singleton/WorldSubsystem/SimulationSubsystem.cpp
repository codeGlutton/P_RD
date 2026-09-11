#include "Singleton/WorldSubsystem/SimulationSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Simulation/RoomContext.h"
#include "Simulation/RoomInstance.h"
#include "Simulation/Logger/EventLogger.h"
#include "Simulation/Factory/ObjectModelFactory.h"

#include "Setting/GamePlaySettings.h"

#include "ObjectModel.h"

#include "SRPGFramework/SRPGCommand.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

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
		mSimulationRoomContext.mModelFactory->SetContext(mSimulationRoomContext);

		mSimulationRoomContext.mEventLogger = NewObject<USimulationEventLogger>(this);
		mSimulationRoomContext.mEventLogger->SetContext(mSimulationRoomContext);
	}

	Cast<UGameObjectModelFactory>(mGameRoomContext.mModelFactory)->RegisterSubsystemModels();
	SetSimulationState(ESRPGSimulationState::RunningGame);
}

void USimulationSubsystem::PreDeinitialize()
{
	Cast<UGameObjectModelFactory>(mGameRoomContext.mModelFactory)->UnregisterSubsystemModels();
	
	Super::PreDeinitialize();
}

TArray<FSRPGTurnEventLog> USimulationSubsystem::PlaySimulation(FSimulationOption Option)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(RDSimulation);

	checkf(mSimulationState == ESRPGSimulationState::RunningGame, TEXT("이미 시뮬레이션 중"));
	SetSimulationState(ESRPGSimulationState::RunningSimulation);

	USRPGCombatModel* SRPGCombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);

	/* 전처리 */

	if (Option.mReservedCommand.IsValid() == true)
	{
		SRPGCombatModel->ForcedClearActions();
	}

	/* 옵션 세팅 */

	if (Option.mSkipAIActions == true)
	{
		SRPGCombatModel->RequestSkipAIActions();
	}
	if (Option.mDuration == ESimulationDurtaion::NextAction)
	{
		SRPGCombatModel->RequestAdvanceUntilNextAction();
	}
	else
	{
		SRPGCombatModel->RequestAdvanceUntilAllPlayerTurn();
	}

	/* 시뮬 시작 */

	if (Option.mReservedCommand.IsValid() == true)
	{
		USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
		checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 서브시스템 모델 nullptr"));
		CommandRouterModel->SummitCommand(Option.mReservedCommand);
	}
	else
	{
		SRPGCombatModel->ForcedBeginTurn();
	}

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
		TRACE_CPUPROFILER_EVENT_SCOPE(RDCombatPreviewDuplicate);
		mSimulationRoomContext.mRoomInstance = Cast<URoomInstance>(StaticDuplicateObject(mGameRoomContext.mRoomInstance, this));
		mSimulationRoomContext.mRoomInstance->CollectSimulationDatas();
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

