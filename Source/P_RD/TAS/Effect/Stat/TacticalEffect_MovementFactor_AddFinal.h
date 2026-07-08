/*****************************************************************//**
 * @file   TacticalEffect_MovementFactor_AddFinal.h
 * @brief  MovementFactor AddFinal 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_MovementFactor_AddFinal.generated.h"

/**
 * @brief MovementFactor AddFinal 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_MovementFactor_AddFinal : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_MovementFactor_AddFinal();
};
