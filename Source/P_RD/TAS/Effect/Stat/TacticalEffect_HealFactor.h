/*****************************************************************//**
 * @file   TacticalEffect_HealFactor.h
 * @brief  HealFactor 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_HealFactor.generated.h"

/**
 * @brief HealFactor 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_HealFactor : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_HealFactor();
};
