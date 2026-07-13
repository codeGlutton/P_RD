/*****************************************************************//**
 * @file   TacticalEffect_Movement.h
 * @brief  Movement 이펙트 정의 헤더
 * @author 이문환, 모호재
 * @date   2026-07-01
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Calculation/TacticalEffectExecutionCalculation.h"
#include "TacticalEffect_Movement.generated.h"

/**
 * @brief Movement 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Movement : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_Movement();

	/* UTacticalEffect 상속 */
public:
	void OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const override;
};

/**
 * @brief Movement 습득 계산기
 */
UCLASS()
class UTacticalEffectExecutionCalculation_GetMovement : public UTacticalEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	void Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

/**
 * @brief Movement 습득 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetMovement : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetMovement();
};

