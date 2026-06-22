/*****************************************************************//**
 * @file   SimulationType.h
 * @brief  시뮬레이션에서 사용되는 타입들 정의 헤더
 * @author 모호재
 * @date   2026-06-20
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SimulationType.generated.h"

/**
 * @brief 시뮬레이션 상태 열거형
 */
UENUM(BlueprintType)
enum class ESRPGSimulationState : uint8
{
	RunningGame        UMETA(ToolTip = "인 게임 진행 중"),
	RunningSimulation  UMETA(ToolTip = "시물레이션 진행 중"),
};
