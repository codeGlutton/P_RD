/*****************************************************************//**
 * @file   TacticalEffect_HealFactor_DivideAdditive.h
 * @brief  HealFactor DivideAdditive 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_HealFactor_DivideAdditive.generated.h"

/**
 * @brief HealFactor DivideAdditive 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_HealFactor_DivideAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_HealFactor_DivideAdditive();
};
