/*****************************************************************//**
 * @file   TacticalEffect_DefenseFactor_AddBase.h
 * @brief  DefenseFactor AddBase 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_DefenseFactor_AddBase.generated.h"

/**
 * @brief DefenseFactor AddBase 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_DefenseFactor_AddBase : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_DefenseFactor_AddBase();
};
