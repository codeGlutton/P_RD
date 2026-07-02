/*****************************************************************//**
 * @file   TacticalEffect_AttackFactor.h
 * @brief  AttackFactor 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_AttackFactor.generated.h"

/**
 * @brief AttackFactor 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AttackFactor : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_AttackFactor();
};
