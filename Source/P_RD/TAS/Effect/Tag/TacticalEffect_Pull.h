/*****************************************************************//**
 * @file   TacticalEffect_Pull.h
 * @brief  Pull(끌어오기) 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-08-25
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TAS/Calculation/TacticalEffectExecutionCalculation.h"
#include "TacticalEffect_Pull.generated.h"

/**
 * @brief 당기기 계산기
 */
UCLASS()
class UTacticalEffectExecutionCalculation_Pull : public UTacticalEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	void Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

/**
 * @brief  Pull(끌어오기) 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Pull : public UTacticalEffect_InstantStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_Pull();
};

/**
 * @brief  Pull(끌어오기) 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddPull : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddPull();
};

/**
 * @brief  Pull(끌어오기) 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetPull : public UTacticalEffect_GetStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetPull();
};
