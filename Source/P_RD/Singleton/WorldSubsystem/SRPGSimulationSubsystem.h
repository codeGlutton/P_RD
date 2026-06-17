/*****************************************************************//**
 * @file   SRPGSimulationSubsystem.h
 * @brief  SRPG 시뮬레이션을 처리하는 서브시스템 정의 헤더
 * @author 모호재
 * @date   2026-06-16
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGSimulationSubsystem.generated.h"

// SRPG Simulation 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogSRPGSimulation, Log, All)

/**
 * @brief  SRPG 시뮬레이션을 처리하는 서브시스템
 */
UCLASS()
class P_RD_API USRPGSimulationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void LogNewEvent();

private:
	ESRPGSimulationState mSimulationState = ESRPGSimulationState::RunningGame;

private:

};
