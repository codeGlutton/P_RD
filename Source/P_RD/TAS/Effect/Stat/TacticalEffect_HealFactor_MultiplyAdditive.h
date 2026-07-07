/*****************************************************************//**
 * @file   TacticalEffect_HealFactor_MultiplyAdditive.h
 * @brief  HealFactor MultiplyAdditive 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_HealFactor_MultiplyAdditive.generated.h"

/**
 * @brief HealFactor MultiplyAdditive 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_HealFactor_MultiplyAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_HealFactor_MultiplyAdditive();
};
