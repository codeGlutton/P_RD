/*****************************************************************//**
 * @file   TacticalEffect_AttackFactor_AddBase.h
 * @brief  AttackFactor AddBase 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_AttackFactor_AddBase.generated.h"

/**
 * @brief AttackFactor AddBase 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AttackFactor_AddBase : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_AttackFactor_AddBase();
};
