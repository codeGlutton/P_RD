/*****************************************************************//**
 * @file   TacticalEffect_ReduceCooldown.h
 * @brief  적용 중인 쿨다운 Effect의 지속 시간을 감소시키는 이펙트 및 계산기 정의 헤더
 * @author 모호재
 * @date   2026-09-21
 *********************************************************************/

#pragma once

#include "TAS/Calculation/TacticalEffectExecutionCalculation.h"
#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_ReduceCooldown.generated.h"

/**
 * @brief 쿨다운 감소 계산기
 */
UCLASS()
class UTacticalEffectExecutionCalculation_ReduceCooldown : public UTacticalEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	void Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

/**
 * @brief 적용 중인 UTacticalEffect_Cooldown 이펙트들의 남은 시간을 단축시키는 즉발 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_ReduceCooldown : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_ReduceCooldown();

	/* UTacticalEffect 상속 */
public:
	virtual bool CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const override;
};
