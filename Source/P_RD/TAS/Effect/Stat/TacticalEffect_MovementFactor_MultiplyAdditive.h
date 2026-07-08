/*****************************************************************//**
 * @file   TacticalEffect_MovementFactor_MultiplyAdditive.h
 * @brief  MovementFactor MultiplyAdditive 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_MovementFactor_MultiplyAdditive.generated.h"

/**
 * @brief MovementFactor MultiplyAdditive 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_MovementFactor_MultiplyAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_MovementFactor_MultiplyAdditive();
};
