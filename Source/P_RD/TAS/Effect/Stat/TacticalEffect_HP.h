/*****************************************************************//**
 * @file   TacticalEffect_HP.h
 * @brief  체력(HP) 가감 이펙트 정의 헤더
 * @author 이문환, 모호재
 * @date   2026-06-30
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Effect/Stat/TacticalEffect_Unit.h"
#include "TAS/Calculation/TacticalEffectExecutionCalculation.h"
#include "TacticalEffect_HP.generated.h"

/**
 * @brief 체력(HP)을 변경하는 이펙트
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
 * @brief 퍼센트 기반 시스템 명령 HP 힐 계산기
 */
UCLASS()
class UTacticalEffectExecutionCalculation_SystemHeal : public UTacticalEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	void Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const override;

protected:
	virtual float GetHealRatio() const;
};

/**
 * @brief 휴식으로 인한 HP 계산기
 */
UCLASS()
class UTacticalEffectExecutionCalculation_BreakTimeHeal : public UTacticalEffectExecutionCalculation_SystemHeal
{
	GENERATED_BODY()

protected:
	float GetHealRatio() const override;
};

/**
 * @brief 휴식으로 인한 HP 적용 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_BreakTimeHeal : public UTacticalEffect_Unit
{
	GENERATED_BODY()

public:
	UTacticalEffect_BreakTimeHeal();
};

/**
 * @brief 클리어로 인한 HP 계산기
 */
UCLASS()
class UTacticalEffectExecutionCalculation_StageClearHeal : public UTacticalEffectExecutionCalculation_SystemHeal
{
	GENERATED_BODY()

protected:
	float GetHealRatio() const override;
};

/**
 * @brief 클리어로 인한 HP 적용 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_StageClearHeal : public UTacticalEffect_Unit
{
	GENERATED_BODY()

public:
	UTacticalEffect_StageClearHeal();
};

/**
 * @brief HP 힐 계산기
 */
UCLASS()
class UTacticalEffectExecutionCalculation_Heal : public UTacticalEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	void Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

/**
 * @brief HP 힐 적용 이펙트
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


