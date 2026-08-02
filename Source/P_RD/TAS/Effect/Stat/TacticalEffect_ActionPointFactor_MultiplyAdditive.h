/*****************************************************************//**
 * @file   TacticalEffect_ActionPointFactor_MultiplyAdditive.h
 * @brief  ActionPointFactor MultiplyAdditive 이펙트 정의 헤더
 * @author 이문환, 모호재
 * @date   2026-08-01
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_ActionPointFactor_MultiplyAdditive.generated.h"

/**
 * @brief ActionPointFactor MultiplyAdditive 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_ActionPointFactor_MultiplyAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_ActionPointFactor_MultiplyAdditive();
};
