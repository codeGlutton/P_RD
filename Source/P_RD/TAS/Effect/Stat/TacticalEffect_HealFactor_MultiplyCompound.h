/*****************************************************************//**
 * @file   TacticalEffect_HealFactor_MultiplyCompound.h
 * @brief  HealFactor MultiplyCompound 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_HealFactor_MultiplyCompound.generated.h"

/**
 * @brief HealFactor MultiplyCompound 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_HealFactor_MultiplyCompound : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_HealFactor_MultiplyCompound();
};
