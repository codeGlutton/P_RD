/*****************************************************************//**
 * @file   TacticalEffect_Push.h
 * @brief  Push(밀치기) 이펙트 정의 헤더
 * @author 모호재, 이문환
 * @date   2026-08-25
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TAS/Calculation/TacticalEffectExecutionCalculation.h"
#include "TacticalEffect_Push.generated.h"

/**
 * @brief 밀치기 계산기
 */
UCLASS()
class UTacticalEffectExecutionCalculation_Push : public UTacticalEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	void Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

/**
 * @brief  Push(밀치기) 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Push : public UTacticalEffect_InstantStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_Push();
};

/**
 * @brief  Push(밀치기) 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddPush : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddPush();
};

/**
 * @brief  Push(밀치기) 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetPush : public UTacticalEffect_GetStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetPush();
};
