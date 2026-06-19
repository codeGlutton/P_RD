/*****************************************************************//**
 * @file   SimulationSubsystem.h
 * @brief  시뮬레이션을 처리하는 서브시스템 정의 헤더
 * @author 모호재
 * @date   2026-06-16
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/Logger/EventLog.h"
#include "Simulation/RoomContext.h"
#include "SimulationSubsystem.generated.h"

// Simulation 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogSimulation, Log, All)

struct FRoomContext;
class UEventLogger;
class UObjectModelFactory;

/**
 * @brief 시뮬레이션 상태 열거형
 */
UENUM(BlueprintType)
enum class ESRPGSimulationState : uint8
{
	RunningGame        UMETA(ToolTip = "인 게임 진행 중"),
	RunningSimulation  UMETA(ToolTip = "시물레이션 진행 중"),
};

/**
 * @brief  SRPG 시뮬레이션을 처리하는 서브시스템
 */
UCLASS()
class P_RD_API USimulationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	/* UWorldSubsystem 상속 */
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;

	/* 시뮬레이션 함수 */
public:
	/**
	 * 다음 액션까지 결과를 시뮬레이션 돌리는 함수
	 * @return 결과 로그
	 */
	TArray<FSRPGTurnEventLog> SimulateUntilNextAction();
	/**
	 * 다음 플레이어 턴까지 결과를 시뮬레이션 돌리는 함수
	 * @return 결과 로그
	 */
	TArray<FSRPGTurnEventLog> SimulateUntilNextPlayerTurn();

protected:
	void SetSimulationState(ESRPGSimulationState State);

public:
	const FRandomStream& GetEventStream() const;
	UEventLogger* GetEventLogger() const;
	UObjectModelFactory* GetModelFactory() const;

	ESRPGSimulationState GetSimulationState() const;

protected:
	ESRPGSimulationState mSimulationState = ESRPGSimulationState::RunningGame;
	FRoomContext* mCurrentRoomContext;

protected:
	UPROPERTY(Category = Context, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "GameRoomContext"))
	FRoomContext mGameRoomContext;
	UPROPERTY(Category = Context, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "SimulationRoomContext"))
	FRoomContext mSimulationRoomContext;
};
