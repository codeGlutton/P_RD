/*****************************************************************//**
 * @file   TacticalEffect_HP.h
 * @brief  체력(HP) 가감 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-06-30
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Calculation/TacticalEffectExecutionCalculation.h"
#include "TacticalEffect_HP.generated.h"

/**
 * @brief 체력(HP)을 변경하는 이펙트
 *
 * @details
 * HP, Additive, Instant를 갖는 이펙트로 정의하고 기본 크기는 1.0f
 * 패시브가 이 이펙트를 적용할 때 mDynamicMagnitude로 배율을 설정.
 * 부호가 +면 회복, -면 피해.
 * HP는 즉시형만 있으므로 클래스 이름에 Instant 붙이지 않음
 */
UCLASS()
class P_RD_API UTacticalEffect_HP : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_HP();

	/* UTacticalEffect 상속 */
public:
	void OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const override;
};

/**
 * @brief HP 데미지 계산기
 */
UCLASS()
class UTacticalEffectExecutionCalculation_Heal : public UTacticalEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	void Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

/**
 * @brief HP 데미지 적용 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Heal : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_Heal();

	/* UTacticalEffect 상속 */
public:
	bool CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const override;
};

/**
 * @brief HP 데미지 계산기
 */
UCLASS()
class UTacticalEffectExecutionCalculation_Attack : public UTacticalEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	void Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

/**
 * @brief HP 데미지 적용 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Attack : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_Attack();

	/* UTacticalEffect 상속 */
public:
	bool CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const override;
};


