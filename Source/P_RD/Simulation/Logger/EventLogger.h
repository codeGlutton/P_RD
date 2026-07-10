/*****************************************************************//**
 * @file   EventLogger.h
 * @brief  시뮬레이션 결과 기록용 로거 헤더
 * @author 모호재
 * @date   2026-06-17
 *********************************************************************/
#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Simulation/Logger/EventLog.h"
#include "EventLogger.generated.h"

// Event Logger 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogEventLogger, Log, All)

struct FRoomContext;

UCLASS(abstract)
class P_RD_API UEventLogger : public UObject
{
	GENERATED_BODY()

public:
	void SetContext(FRoomContext& RoomContext);

public:
	virtual void BeginTurnLog(int32 SourceUnitID, UClass* UnitActorModelClass) PURE_VIRTUAL(UEventLogger::BeginTurnLog, return;);
	virtual void EndTurnLog() PURE_VIRTUAL(UEventLogger::EndTurnLog, return;);

	virtual void BeginActionLog(const FTileIndex& SourceTileIndex) PURE_VIRTUAL(UEventLogger::BeginActionLog, return;);
	virtual void EndActionLog() PURE_VIRTUAL(UEventLogger::EndActionLog, return;);

	virtual void BeginMotionLog() PURE_VIRTUAL(UEventLogger::BeginMotionLog, return;);
	virtual void EndMotionLog() PURE_VIRTUAL(UEventLogger::EndMotionLog, return;);

	virtual void LogTagEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTagEffectEventLog& Log) PURE_VIRTUAL(UEventLogger::LogTagEffect, return;);
	virtual void LogAttributeEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGAttributeEffectEventLog& Log) PURE_VIRTUAL(UEventLogger::LogAttributeEffect, return;);
	virtual void LogTileEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTileEffectEventLog& Log) PURE_VIRTUAL(UEventLogger::LogTileEffect, return;);

public:
	virtual TArray<FSRPGTurnEventLog> PopSRPGLogs() PURE_VIRTUAL(UEventLogger::PopSRPGLogs, return TArray<FSRPGTurnEventLog>(););

protected:
	FRoomContext* mRoomContext = nullptr;
};

/**
 * @brief 실제 게임에서의 이벤트 로거
 */
UCLASS()
class UGameEventLogger : public UEventLogger
{
	GENERATED_BODY()

public:
	void BeginTurnLog(int32 SourceUnitID, UClass* UnitActorModelClass) override;
	void EndTurnLog() override;

	void BeginActionLog(const FTileIndex& SourceTileIndex) override;
	void EndActionLog() override;

	void BeginMotionLog() override;
	void EndMotionLog() override;

	void LogTagEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTagEffectEventLog& Log) override;
	void LogAttributeEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGAttributeEffectEventLog& Log) override;
	void LogTileEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTileEffectEventLog& Log) override;

public:
	TArray<FSRPGTurnEventLog> PopSRPGLogs() override;

protected:
	TArray<FSRPGTurnEventLog> mTurnEventLogs;

protected:
	FSRPGTurnEventLog* mCurrentTurnEventLog = nullptr;
	FSRPGActionEventLog* mCurrentActionEventLog = nullptr;
	FSRPGMotionEventLog* mCurrentMotionEventLog = nullptr;

	int32 mActiveTurnSourceUnitID = INDEX_NONE;
	TSubclassOf<UObject> mActiveTurnActorModelClass = nullptr;
};

/**
 * @brief 시뮬레이션에서의 이벤트 로거
 */
UCLASS()
class USimulationEventLogger : public UEventLogger
{
	GENERATED_BODY()

public:
	void BeginTurnLog(int32 SourceUnitID, UClass* UnitActorModelClass) override;
	void EndTurnLog() override;

	void BeginActionLog(const FTileIndex& SourceTileIndex) override;
	void EndActionLog() override;

	void BeginMotionLog() override;
	void EndMotionLog() override;

	void LogTagEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTagEffectEventLog& Log) override;
	void LogAttributeEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGAttributeEffectEventLog& Log) override;
	void LogTileEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTileEffectEventLog& Log) override;

public:
	TArray<FSRPGTurnEventLog> PopSRPGLogs() override;

protected:
	TArray<FSRPGTurnEventLog> mTurnEventLogs;

protected:
	FSRPGTurnEventLog* mCurrentTurnEventLog = nullptr;
	FSRPGActionEventLog* mCurrentActionEventLog = nullptr;
	FSRPGMotionEventLog* mCurrentMotionEventLog = nullptr;
};


