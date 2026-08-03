/*****************************************************************//**
 * @file   TacticalEffect_Defense.h
 * @brief  Defense 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Calculation/TacticalEffectExecutionCalculation.h"
#include "TacticalEffect_Defense.generated.h"

/**
 * @brief Defense 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Defense : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_Defense();

	/* UTacticalEffect 상속 */
public:
	void OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const override;
};

/**
 * @brief Defense 습득 계산기
 */
UCLASS()
class UTacticalEffectExecutionCalculation_GetDefense : public UTacticalEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	void Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

/**
 * @brief Defense 습득 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetDefense : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetDefense();

	/* UTacticalEffect 상속 */
public:
	bool CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const override;
};

