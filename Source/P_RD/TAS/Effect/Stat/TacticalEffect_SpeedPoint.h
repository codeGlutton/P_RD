/*****************************************************************//**
 * @file   TacticalEffect_SpeedPoint.h
 * @brief  SpeedPoint 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-08-01
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Calculation/TacticalEffectExecutionCalculation.h"
#include "TacticalEffect_SpeedPoint.generated.h"

/**
 * @brief SpeedPoint 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_SpeedPoint : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_SpeedPoint();

	/* UTacticalEffect 상속 */
public:
	void OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const override;
};

/**
 * @brief SpeedPoint 습득 계산기
 */
UCLASS()
class UTacticalEffectExecutionCalculation_GetSpeedPoint : public UTacticalEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	void Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

/**
 * @brief SpeedPoint 습득 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetSpeedPoint : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetSpeedPoint();
};
