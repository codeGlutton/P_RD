/*****************************************************************//**
 * @file   TacticalEffect_ActionPointFactor_AddBase.h
 * @brief  ActionPointFactor AddBase 이펙트 정의 헤더
 * @author 이문환, 모호재
 * @date   2026-08-01
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_ActionPointFactor_AddBase.generated.h"

/**
 * @brief ActionPointFactor AddBase 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_ActionPointFactor_AddBase : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_ActionPointFactor_AddBase();
};
