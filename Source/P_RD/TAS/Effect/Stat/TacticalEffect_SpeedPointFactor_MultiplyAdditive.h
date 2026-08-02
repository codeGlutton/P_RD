/*****************************************************************//**
 * @file   TacticalEffect_SpeedPointFactor_MultiplyAdditive.h
 * @brief  SpeedPointFactor MultiplyAdditive 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-08-01
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_SpeedPointFactor_MultiplyAdditive.generated.h"

/**
 * @brief SpeedPointFactor MultiplyAdditive 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_SpeedPointFactor_MultiplyAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_SpeedPointFactor_MultiplyAdditive();
};
