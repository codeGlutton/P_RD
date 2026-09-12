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

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnLogTagEffect, int32 /*TargetActorID*/, UClass* /*BoardActorModelClass*/, const FSRPGTagEffectEventLog& /*Log*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnLogAttributeEffect, int32 /*TargetActorID*/, UClass* /*BoardActorModelClass*/, const FSRPGAttributeEffectEventLog& /*Log*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnLogTileEffect, int32 /*TargetActorID*/, UClass* /*BoardActorModelClass*/, const FSRPGTileEffectEventLog& /*Log*/);

struct FRoomContext;

UCLASS(abstract)
class P_RD_API UEventLogger : public UObject
{
	GENERATED_BODY()

public:
	void SetContext(FRoomContext& RoomContext);

public:
	virtual void BeginTurnLog(int32 RoundIndex, int32 SourceUnitID, UClass* UnitActorModelClass);
	virtual void EndTurnLog();

	virtual void LogAIPlan(const FSRPGAIPlanLog& Log);

	virtual void BeginActionLog(const FTileIndex& SourceTileIndex);
	virtual void EndActionLog();

	virtual void BeginMotionLog();
	virtual void EndMotionLog();

	virtual void LogTagEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTagEffectEventLog& Log);
	virtual void LogAttributeEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGAttributeEffectEventLog& Log);
	virtual void LogTileEffect(int32 TargetActorID, UClass* BoardActorModelClass, const FSRPGTileEffectEventLog& Log);

public:
	virtual TArray<FSRPGTurnEventLog> PopSRPGLogs();

public:
	FOnLogTagEffect OnLogTagEffect;
	FOnLogAttributeEffect OnLogAttributeEffect;
	FOnLogTileEffect OnLogTileEffect;

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
};

/**
 * @brief 시뮬레이션에서의 이벤트 로거
 */
UCLASS()
class USimulationEventLogger : public UEventLogger
{
	GENERATED_BODY()

public:
	void BeginTurnLog(int32 RoundIndex, int32 SourceUnitID, UClass* UnitActorModelClass) override;
	void EndTurnLog() override;

	void LogAIPlan(const FSRPGAIPlanLog& Log) override;

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


