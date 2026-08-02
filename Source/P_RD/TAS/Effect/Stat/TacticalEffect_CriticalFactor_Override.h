/*****************************************************************//**
 * @file   TacticalEffect_CriticalFactor_Override.h
 * @brief  CriticalFactor Override 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-08-01
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_CriticalFactor_Override.generated.h"

/**
 * @brief CriticalFactor Override 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_CriticalFactor_Override : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_CriticalFactor_Override();
};
