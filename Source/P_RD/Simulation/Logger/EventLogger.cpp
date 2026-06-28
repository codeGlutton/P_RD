#include "Simulation/Logger/EventLogger.h"

DEFINE_LOG_CATEGORY(LogEventLogger)

void UEventLogger::SetContext(FRoomContext& RoomContext)
{
	mRoomContext = &RoomContext;
}

void UGameEventLogger::BeginTurnLog(int32 SourceUnitID, UClass* UnitActorModelClass)
{
	UE_LOG(LogEventLogger, Log, TEXT("턴 이벤트 로그 시작"));
}

void UGameEventLogger::EndTurnLog()
{
	UE_LOG(LogEventLogger, Log, TEXT("턴 이벤트 로그 종료"));
}

void UGameEventLogger::BeginActionLog(const FTileIndex& SourceTileIndex)
{
	UE_LOG(LogEventLogger, Log, TEXT("액션 이벤트 로그 시작"));
}

void UGameEventLogger::EndActionLog()
{
	UE_LOG(LogEventLogger, Log, TEXT("액션 이벤트 로그 종료"));
}

void UGameEventLogger::BeginMotionLog()
{
	UE_LOG(LogEventLogger, Log, TEXT("모션 이벤트 로그 시작"));
}

void UGameEventLogger::EndMotionLog()
{
	UE_LOG(LogEventLogger, Log, TEXT("모션 이벤트 로그 종료"));
}

void UGameEventLogger::LogTagEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTagEffectEventLog& Log)
{
	UE_LOG(LogEventLogger, Log, TEXT("[%d][%s : %d] 태그 변경"), TargetActorID, *Log.mEffectTag.ToString(), Log.mCount);
}

void UGameEventLogger::LogAttributeEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGAttributeEffectEventLog& Log)
{
	UE_LOG(LogEventLogger, Log, TEXT("[%d][%s : %f] 속성 변경"), TargetActorID, *Log.mEffectAttribute.GetName(), Log.mMagnitude);
}

void UGameEventLogger::LogTileEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTileEffectEventLog& Log)
{
	UE_LOG(LogEventLogger, Log, TEXT("[%d][%s][(%d, %d) -> (%d, %d)] 타일 위치 이동"), TargetActorID, *EnumToString(Log.mOccupancyState), Log.mPreTileIndex.mX, Log.mPreTileIndex.mY, Log.mNextTileIndex.mX, Log.mNextTileIndex.mY);
}

TArray<FSRPGTurnEventLog> UGameEventLogger::PopSRPGLogs()
{
	// 아무것도 안함

	return TArray<FSRPGTurnEventLog>();
}

void USimulationEventLogger::BeginTurnLog(int32 SourceUnitID, UClass* UnitActorModelClass)
{
	FSRPGTurnEventLog TurnLog;
	TurnLog.mSourceUnitID = SourceUnitID;
	TurnLog.mUnitActorModelClass = UnitActorModelClass;

	checkf(TurnLog.IsValid() == true, TEXT("턴 로그 불량"));

	mTurnEventLogs.Add(MoveTemp(TurnLog));
	mCurrentTurnEventLog = &mTurnEventLogs.Last();

	UE_LOG(LogEventLogger, Log, TEXT("턴 이벤트 로그 시작"));
}

void USimulationEventLogger::EndTurnLog()
{
	checkf(mCurrentTurnEventLog != nullptr, TEXT("턴 로그 시작 없이 턴 로그 종료 오류"));
	mCurrentTurnEventLog = nullptr;

	UE_LOG(LogEventLogger, Log, TEXT("턴 이벤트 로그 종료"));
}

void USimulationEventLogger::BeginActionLog(const FTileIndex& SourceTileIndex)
{
	checkf(mCurrentTurnEventLog != nullptr, TEXT("턴 로그 시작 없이 액션 로그 시작 오류"));

	FSRPGActionEventLog ActionLog;
	ActionLog.mSourceTileIndex = SourceTileIndex;

	checkf(ActionLog.IsValid() == true, TEXT("액션 로그 불량"));

	mCurrentTurnEventLog->mActionEventLogs.Add(MoveTemp(ActionLog));
	mCurrentActionEventLog = &mCurrentTurnEventLog->mActionEventLogs.Last();

	UE_LOG(LogEventLogger, Log, TEXT("액션 이벤트 로그 시작"));
}

void USimulationEventLogger::EndActionLog()
{
	checkf(mCurrentActionEventLog != nullptr, TEXT("액션 로그 시작 없이 액션 로그 종료 오류"));
	mCurrentActionEventLog = nullptr;

	UE_LOG(LogEventLogger, Log, TEXT("액션 이벤트 로그 종료"));
}

void USimulationEventLogger::BeginMotionLog()
{
	checkf(mCurrentTurnEventLog != nullptr, TEXT("턴 로그 시작 없이 모션 로그 시작 오류"));
	checkf(mCurrentActionEventLog != nullptr, TEXT("액션 로그 시작 없이 모션 로그 시작 오류"));

	FSRPGMotionEventLog MotionLog;

	checkf(MotionLog.IsValid() == true, TEXT("모션 로그 불량"));

	mCurrentActionEventLog->mMotionEventLogs.Add(MoveTemp(MotionLog));
	mCurrentMotionEventLog = &mCurrentActionEventLog->mMotionEventLogs.Last();

	UE_LOG(LogEventLogger, Log, TEXT("모션 이벤트 로그 시작"));
}

void USimulationEventLogger::EndMotionLog()
{
	checkf(mCurrentMotionEventLog != nullptr, TEXT("모션 로그 시작 없이 모션 로그 종료 오류"));
	mCurrentMotionEventLog = nullptr;

	UE_LOG(LogEventLogger, Log, TEXT("모션 이벤트 로그 종료"));
}

void USimulationEventLogger::LogTagEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTagEffectEventLog& Log)
{
	checkf(mCurrentMotionEventLog != nullptr, TEXT("모션 로그 시작 없이 태그 변경 기록 오류"));
	checkf(Log.IsValid() == true, TEXT("태그 변경 로그 불량"));

	FSRPGBoardActorEventLog& BoardActorLog = mCurrentMotionEventLog->mBoardActorEventLogs[TargetActorID];
	BoardActorLog.mTargetActorID = TargetActorID;
	BoardActorLog.mBoardActorModelClass = BoardActorModelClass;

	checkf(BoardActorLog.IsValid() == true, TEXT("보드 액터 로그 불량"));

	bool IsAlreadyExisted = false;
	FSRPGTagEffectEventLog& TagEffectEventLog = BoardActorLog.mTagEffectEventLogs.FindOrAdd(Log, &IsAlreadyExisted);
	if (IsAlreadyExisted == true)
	{
		TagEffectEventLog.mEffectTag = Log.mEffectTag;
		TagEffectEventLog.mCount += Log.mCount;
	}

	UE_LOG(LogEventLogger, Log, TEXT("[%d][%s : %d] 태그 변경"), TargetActorID, *Log.mEffectTag.ToString(), Log.mCount);
}

void USimulationEventLogger::LogAttributeEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGAttributeEffectEventLog& Log)
{
	checkf(mCurrentMotionEventLog != nullptr, TEXT("모션 로그 시작 없이 속성 변경 기록 오류"));
	checkf(Log.IsValid() == true, TEXT("속성 변경 로그 불량"));

	FSRPGBoardActorEventLog& BoardActorLog = mCurrentMotionEventLog->mBoardActorEventLogs[TargetActorID];
	BoardActorLog.mTargetActorID = TargetActorID;
	BoardActorLog.mBoardActorModelClass = BoardActorModelClass;

	checkf(BoardActorLog.IsValid() == true, TEXT("보드 액터 로그 불량"));

	bool IsAlreadyExisted = false;
	FSRPGAttributeEffectEventLog& AttributeEffectLog = BoardActorLog.mAttributeEffectEventLogs.FindOrAdd(Log, &IsAlreadyExisted);
	if (IsAlreadyExisted == true)
	{
		AttributeEffectLog.mEffectAttribute = Log.mEffectAttribute;
		AttributeEffectLog.mMagnitude += Log.mMagnitude;
	}

	UE_LOG(LogEventLogger, Log, TEXT("[%d][%s : %f] 속성 변경"), TargetActorID, *Log.mEffectAttribute.GetName(), Log.mMagnitude);
}

void USimulationEventLogger::LogTileEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTileEffectEventLog& Log)
{
	checkf(mCurrentMotionEventLog != nullptr, TEXT("모션 로그 시작 없이 속성 변경 기록 오류"));
	checkf(Log.IsValid() == true, TEXT("타일 위치 변경 로그 불량"));

	FSRPGBoardActorEventLog& BoardActorLog = mCurrentMotionEventLog->mBoardActorEventLogs[TargetActorID];
	BoardActorLog.mTargetActorID = TargetActorID;
	BoardActorLog.mBoardActorModelClass = BoardActorModelClass;

	checkf(BoardActorLog.IsValid() == true, TEXT("보드 액터 로그 불량"));

	BoardActorLog.mTileEffectEventLogs.Add(Log);

	UE_LOG(LogEventLogger, Log, TEXT("[%d][%s][(%d, %d) -> (%d, %d)] 타일 위치 이동"), TargetActorID, *EnumToString(Log.mOccupancyState), Log.mPreTileIndex.mX, Log.mPreTileIndex.mY, Log.mNextTileIndex.mX, Log.mNextTileIndex.mY);
}

TArray<FSRPGTurnEventLog> USimulationEventLogger::PopSRPGLogs()
{
	return MoveTemp(mTurnEventLogs);
}
