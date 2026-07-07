/*****************************************************************//**
 * @file   TacticalEffect_HealFactor_Override.h
 * @brief  HealFactor Override 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_HealFactor_Override.generated.h"

/**
 * @brief HealFactor Override 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_HealFactor_Override : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_HealFactor_Override();
};
