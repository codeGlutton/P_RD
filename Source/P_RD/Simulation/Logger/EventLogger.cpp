#include "Simulation/Logger/EventLogger.h"

DEFINE_LOG_CATEGORY(LogEventLogger)

void UEventLogger::SetContext(FRoomContext& RoomContext)
{
	mRoomContext = &RoomContext;
}

void UGameEventLogger::BeginTurnLog(int32 SourceUnitID, UClass* UnitActorModelClass)
{
	mActiveTurnSourceUnitID = SourceUnitID;
	mActiveTurnActorModelClass = UnitActorModelClass;

	UE_LOG(LogEventLogger, Log, TEXT("턴 이벤트 로그 시작"));
}

void UGameEventLogger::EndTurnLog()
{
	mCurrentMotionEventLog = nullptr;
	mCurrentActionEventLog = nullptr;
	mCurrentTurnEventLog = nullptr;
	mActiveTurnSourceUnitID = INDEX_NONE;
	mActiveTurnActorModelClass = nullptr;

	UE_LOG(LogEventLogger, Log, TEXT("턴 이벤트 로그 종료"));
}

void UGameEventLogger::BeginActionLog(const FTileIndex& SourceTileIndex)
{
	if (mActiveTurnSourceUnitID == INDEX_NONE || mActiveTurnActorModelClass == nullptr)
	{
		UE_LOG(LogEventLogger, Warning, TEXT("턴 로그 시작 없이 액션 로그 시작 요청으로 실제 전투 로그를 무시"));
		return;
	}

	FSRPGTurnEventLog TurnLog;
	TurnLog.mSourceUnitID = mActiveTurnSourceUnitID;
	TurnLog.mUnitActorModelClass = mActiveTurnActorModelClass;

	FSRPGActionEventLog ActionLog;
	ActionLog.mSourceTileIndex = SourceTileIndex;

	if (TurnLog.IsValid() == false || ActionLog.IsValid() == false)
	{
		UE_LOG(LogEventLogger, Warning, TEXT("실제 전투 액션 로그 불량으로 무시"));
		return;
	}

	TurnLog.mActionEventLogs.Add(MoveTemp(ActionLog));
	mTurnEventLogs.Add(MoveTemp(TurnLog));
	mCurrentTurnEventLog = &mTurnEventLogs.Last();
	mCurrentActionEventLog = &mCurrentTurnEventLog->mActionEventLogs.Last();
	mCurrentMotionEventLog = nullptr;

	UE_LOG(LogEventLogger, Log, TEXT("액션 이벤트 로그 시작"));
}

void UGameEventLogger::EndActionLog()
{
	mCurrentMotionEventLog = nullptr;
	mCurrentActionEventLog = nullptr;
	mCurrentTurnEventLog = nullptr;

	UE_LOG(LogEventLogger, Log, TEXT("액션 이벤트 로그 종료"));
}

void UGameEventLogger::BeginMotionLog()
{
	if (mCurrentActionEventLog == nullptr)
	{
		UE_LOG(LogEventLogger, Warning, TEXT("액션 로그 시작 없이 모션 로그 시작 요청으로 실제 전투 로그를 무시"));
		return;
	}

	FSRPGMotionEventLog MotionLog;
	if (MotionLog.IsValid() == false)
	{
		UE_LOG(LogEventLogger, Warning, TEXT("실제 전투 모션 로그 불량으로 무시"));
		return;
	}

	mCurrentActionEventLog->mMotionEventLogs.Add(MoveTemp(MotionLog));
	mCurrentMotionEventLog = &mCurrentActionEventLog->mMotionEventLogs.Last();

	UE_LOG(LogEventLogger, Log, TEXT("모션 이벤트 로그 시작"));
}

void UGameEventLogger::EndMotionLog()
{
	mCurrentMotionEventLog = nullptr;

	UE_LOG(LogEventLogger, Log, TEXT("모션 이벤트 로그 종료"));
}

void UGameEventLogger::LogTagEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTagEffectEventLog& Log)
{
	if (mCurrentMotionEventLog == nullptr)
	{
		UE_LOG(LogEventLogger, Log, TEXT("모션 외 범위에서의 실제 전투 태그 로그 요청으로 화면 로그만 무시"));
		UE_LOG(LogEventLogger, Log, TEXT("[%d][%s : %d] 태그 변경"), TargetActorID, *Log.mEffectTag.ToString(), Log.mCount);
		return;
	}
	if (Log.IsValid() == false)
	{
		UE_LOG(LogEventLogger, Warning, TEXT("실제 전투 태그 변경 로그 불량으로 무시"));
		return;
	}

	FSRPGBoardActorEventLog& BoardActorLog = mCurrentMotionEventLog->mBoardActorEventLogs.FindOrAdd(TargetActorID);
	BoardActorLog.mTargetActorID = TargetActorID;
	BoardActorLog.mBoardActorModelClass = BoardActorModelClass;

	if (BoardActorLog.IsValid() == false)
	{
		UE_LOG(LogEventLogger, Warning, TEXT("실제 전투 보드 액터 태그 로그 불량으로 무시"));
		mCurrentMotionEventLog->mBoardActorEventLogs.Remove(TargetActorID);
		return;
	}

	bool IsAlreadyExisted = false;
	FSRPGTagEffectEventLog& TagEffectEventLog = BoardActorLog.mTagEffectEventLogs.FindOrAdd(Log, &IsAlreadyExisted);
	if (IsAlreadyExisted == true)
	{
		TagEffectEventLog.mEffectTag = Log.mEffectTag;
		TagEffectEventLog.mCount += Log.mCount;
	}

	UE_LOG(LogEventLogger, Log, TEXT("[%d][%s : %d] 태그 변경"), TargetActorID, *Log.mEffectTag.ToString(), Log.mCount);
}

void UGameEventLogger::LogAttributeEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGAttributeEffectEventLog& Log)
{
	if (mCurrentMotionEventLog == nullptr)
	{
		UE_LOG(LogEventLogger, Log, TEXT("모션 외 범위에서의 실제 전투 속성 로그 요청으로 화면 로그만 무시"));
		UE_LOG(LogEventLogger, Log, TEXT("[%d][%s : %f] 속성 변경"), TargetActorID, *Log.mEffectAttribute.GetName(), Log.mMagnitude);
		return;
	}
	if (Log.IsValid() == false)
	{
		UE_LOG(LogEventLogger, Warning, TEXT("실제 전투 속성 변경 로그 불량으로 무시"));
		return;
	}

	FSRPGBoardActorEventLog& BoardActorLog = mCurrentMotionEventLog->mBoardActorEventLogs.FindOrAdd(TargetActorID);
	BoardActorLog.mTargetActorID = TargetActorID;
	BoardActorLog.mBoardActorModelClass = BoardActorModelClass;

	if (BoardActorLog.IsValid() == false)
	{
		UE_LOG(LogEventLogger, Warning, TEXT("실제 전투 보드 액터 속성 로그 불량으로 무시"));
		mCurrentMotionEventLog->mBoardActorEventLogs.Remove(TargetActorID);
		return;
	}

	bool IsAlreadyExisted = false;
	FSRPGAttributeEffectEventLog& AttributeEffectLog = BoardActorLog.mAttributeEffectEventLogs.FindOrAdd(Log, &IsAlreadyExisted);
	if (IsAlreadyExisted == true)
	{
		AttributeEffectLog.mEffectAttribute = Log.mEffectAttribute;
		AttributeEffectLog.mMagnitude += Log.mMagnitude;
	}

	UE_LOG(LogEventLogger, Log, TEXT("[%d][%s : %f] 속성 변경"), TargetActorID, *Log.mEffectAttribute.GetName(), Log.mMagnitude);
}

void UGameEventLogger::LogTileEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTileEffectEventLog& Log)
{
	if (mCurrentMotionEventLog == nullptr)
	{
		UE_LOG(LogEventLogger, Log, TEXT("모션 외 범위에서의 실제 전투 타일 로그 요청으로 화면 로그만 무시"));
		UE_LOG(LogEventLogger, Log, TEXT("[%d][%s][(%d, %d) -> (%d, %d)] 타일 위치 이동"), TargetActorID, *EnumToString(Log.mOccupancyState), Log.mPreTileIndex.mX, Log.mPreTileIndex.mY, Log.mNextTileIndex.mX, Log.mNextTileIndex.mY);
		return;
	}
	if (Log.IsValid() == false)
	{
		UE_LOG(LogEventLogger, Warning, TEXT("실제 전투 타일 위치 변경 로그 불량으로 무시"));
		return;
	}

	FSRPGBoardActorEventLog& BoardActorLog = mCurrentMotionEventLog->mBoardActorEventLogs.FindOrAdd(TargetActorID);
	BoardActorLog.mTargetActorID = TargetActorID;
	BoardActorLog.mBoardActorModelClass = BoardActorModelClass;

	if (BoardActorLog.IsValid() == false)
	{
		UE_LOG(LogEventLogger, Warning, TEXT("실제 전투 보드 액터 타일 로그 불량으로 무시"));
		mCurrentMotionEventLog->mBoardActorEventLogs.Remove(TargetActorID);
		return;
	}

	BoardActorLog.mTileEffectEventLogs.Add(Log);
	if (Log.mOccupancyState == ESRPGTileOccupancyState::Enter)
	{
		mCurrentMotionEventLog->mSpawnedBoardActorPositions.Add(TargetActorID, Log.mNextTileIndex);
	}

	UE_LOG(LogEventLogger, Log, TEXT("[%d][%s][(%d, %d) -> (%d, %d)] 타일 위치 이동"), TargetActorID, *EnumToString(Log.mOccupancyState), Log.mPreTileIndex.mX, Log.mPreTileIndex.mY, Log.mNextTileIndex.mX, Log.mNextTileIndex.mY);
}

TArray<FSRPGTurnEventLog> UGameEventLogger::PopSRPGLogs()
{
	mCurrentMotionEventLog = nullptr;
	mCurrentActionEventLog = nullptr;
	mCurrentTurnEventLog = nullptr;

	return MoveTemp(mTurnEventLogs);
}

int32 UGameEventLogger::GetCurrentTurnLogIndex() const
{
	// 열린 턴 로그는 항상 배열의 마지막 원소다(BeginActionLog가 Last를 가리키게 세팅).
	return mCurrentTurnEventLog != nullptr ? mTurnEventLogs.Num() - 1 : INDEX_NONE;
}

int32 UGameEventLogger::GetCurrentActionLogIndex() const
{
	if (mCurrentTurnEventLog == nullptr || mCurrentActionEventLog == nullptr)
	{
		return INDEX_NONE;
	}

	return mCurrentTurnEventLog->mActionEventLogs.Num() - 1;
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
	if (mCurrentMotionEventLog == nullptr)
	{
		UE_LOG(LogEventLogger, Log, TEXT("모션 외 범위에서의 잘못된 로그 요청으로 무시"));
		return;
	}

	checkf(Log.IsValid() == true, TEXT("태그 변경 로그 불량"));

	FSRPGBoardActorEventLog& BoardActorLog = mCurrentMotionEventLog->mBoardActorEventLogs.FindOrAdd(TargetActorID);
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
	if (mCurrentMotionEventLog == nullptr)
	{
		UE_LOG(LogEventLogger, Log, TEXT("모션 외 범위에서의 잘못된 로그 요청으로 무시"));
		return;
	}

	checkf(Log.IsValid() == true, TEXT("속성 변경 로그 불량"));

	FSRPGBoardActorEventLog& BoardActorLog = mCurrentMotionEventLog->mBoardActorEventLogs.FindOrAdd(TargetActorID);
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
	if (mCurrentMotionEventLog == nullptr)
	{
		UE_LOG(LogEventLogger, Log, TEXT("모션 외 범위에서의 잘못된 로그 요청으로 무시"));
		return;
	}

	checkf(Log.IsValid() == true, TEXT("타일 위치 변경 로그 불량"));

	FSRPGBoardActorEventLog& BoardActorLog = mCurrentMotionEventLog->mBoardActorEventLogs.FindOrAdd(TargetActorID);
	BoardActorLog.mTargetActorID = TargetActorID;
	BoardActorLog.mBoardActorModelClass = BoardActorModelClass;

	checkf(BoardActorLog.IsValid() == true, TEXT("보드 액터 로그 불량"));

	BoardActorLog.mTileEffectEventLogs.Add(Log);
	if (Log.mOccupancyState == ESRPGTileOccupancyState::Enter)
	{
		mCurrentMotionEventLog->mSpawnedBoardActorPositions.Add(TargetActorID, Log.mNextTileIndex);
	}

	UE_LOG(LogEventLogger, Log, TEXT("[%d][%s][(%d, %d) -> (%d, %d)] 타일 위치 이동"), TargetActorID, *EnumToString(Log.mOccupancyState), Log.mPreTileIndex.mX, Log.mPreTileIndex.mY, Log.mNextTileIndex.mX, Log.mNextTileIndex.mY);
}

TArray<FSRPGTurnEventLog> USimulationEventLogger::PopSRPGLogs()
{
	return MoveTemp(mTurnEventLogs);
}

int32 USimulationEventLogger::GetCurrentTurnLogIndex() const
{
	// 열린 턴 로그는 항상 배열의 마지막 원소다(BeginTurnLog가 Last를 가리키게 세팅).
	return mCurrentTurnEventLog != nullptr ? mTurnEventLogs.Num() - 1 : INDEX_NONE;
}

int32 USimulationEventLogger::GetCurrentActionLogIndex() const
{
	if (mCurrentTurnEventLog == nullptr || mCurrentActionEventLog == nullptr)
	{
		return INDEX_NONE;
	}

	return mCurrentTurnEventLog->mActionEventLogs.Num() - 1;
}
