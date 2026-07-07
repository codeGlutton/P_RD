/*****************************************************************//**
 * @file   TacticalEffect_MovementFactor_MultiplyCompound.h
 * @brief  MovementFactor MultiplyCompound 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_MovementFactor_MultiplyCompound.generated.h"

/**
 * @brief MovementFactor MultiplyCompound 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_MovementFactor_MultiplyCompound : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_MovementFactor_MultiplyCompound();
};
