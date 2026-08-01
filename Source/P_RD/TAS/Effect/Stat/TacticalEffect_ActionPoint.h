/*****************************************************************//**
 * @file   TacticalEffect_ActionPoint.h
 * @brief  ActionPoint 이펙트 정의 헤더
 * @author 이문환, 모호재
 * @date   2026-08-01
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Calculation/TacticalEffectExecutionCalculation.h"
#include "TacticalEffect_ActionPoint.generated.h"

/**
 * @brief ActionPoint 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_ActionPoint : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_ActionPoint();

	/* UTacticalEffect 상속 */
public:
	void OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const override;
};

/**
 * @brief ActionPoint 습득 계산기
 */
UCLASS()
class UTacticalEffectExecutionCalculation_GetActionPoint : public UTacticalEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	void Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

/**
 * @brief ActionPoint 습득 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetActionPoint : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetActionPoint();
};
