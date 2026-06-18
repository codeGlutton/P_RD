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

UINTERFACE(MinimalAPI)
class UEventLogger : public UInterface
{
	GENERATED_BODY()
};

class P_RD_API IEventLogger
{
	GENERATED_BODY()

public:
	virtual void BeginTurnLog(int32 SourceUnitID, UClass* UnitActorModelClass) = 0;
	virtual void EndTurnLog() = 0;

	virtual void BeginActionLog(const FTileIndex& SourceTileIndex, const TArray<FTileIndex>& TargetTileIndexes) = 0;
	virtual void EndActionLog() = 0;

	virtual void BeginMotionLog() = 0;
	virtual void EndMotionLog() = 0;

	virtual void LogAttributeEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGAttributeEffectEventLog& Log) = 0;
	virtual void LogTileEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTileEffectEventLog& Log) = 0;

public:
	virtual const TArray<FSRPGTurnEventLog>& GetSRPGLogs() const = 0;
	virtual TArray<FSRPGTurnEventLog> PopSRPGLogs() = 0;
};

/**
 * @brief 실제 게임에서의 이벤트 로거
 */
UCLASS()
class UGameEventLogger : public UObject, public IEventLogger
{
	GENERATED_BODY()

public:
	void BeginTurnLog(int32 SourceUnitID, UClass* UnitActorModelClass) override;
	void EndTurnLog() override;

	void BeginActionLog(const FTileIndex& SourceTileIndex, const TArray<FTileIndex>& TargetTileIndexes) override;
	void EndActionLog() override;

	void BeginMotionLog() override;
	void EndMotionLog() override;

	void LogAttributeEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGAttributeEffectEventLog& Log) override;
	void LogTileEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTileEffectEventLog& Log) override;

public:
	const TArray<FSRPGTurnEventLog>& GetSRPGLogs() const override;
	TArray<FSRPGTurnEventLog> PopSRPGLogs() override;
};

/**
 * @brief 시뮬레이션에서의 이벤트 로거
 */
UCLASS()
class USimulationEventLogger : public UObject, public IEventLogger
{
	GENERATED_BODY()

public:
	void BeginTurnLog(int32 SourceUnitID, UClass* UnitActorModelClass) override;
	void EndTurnLog() override;

	void BeginActionLog(const FTileIndex& SourceTileIndex, const TArray<FTileIndex>& TargetTileIndexes) override;
	void EndActionLog() override;

	void BeginMotionLog() override;
	void EndMotionLog() override;

	void LogAttributeEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGAttributeEffectEventLog& Log) override;
	void LogTileEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTileEffectEventLog& Log) override;

public:
	const TArray<FSRPGTurnEventLog>& GetSRPGLogs() const override;
	TArray<FSRPGTurnEventLog> PopSRPGLogs() override;

protected:
	TArray<FSRPGTurnEventLog> mTurnEventLogs;

protected:
	FSRPGTurnEventLog* mCurrentTurnEventLog = nullptr;
	FSRPGActionEventLog* mCurrentActionEventLog = nullptr;
	FSRPGMotionEventLog* mCurrentMotionEventLog = nullptr;
};


