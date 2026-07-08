/*****************************************************************//**
 * @file   TacticalEffect_MovementFactor_AddBase.h
 * @brief  MovementFactor AddBase 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_MovementFactor_AddBase.generated.h"

/**
 * @brief MovementFactor AddBase 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_MovementFactor_AddBase : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_MovementFactor_AddBase();
};
