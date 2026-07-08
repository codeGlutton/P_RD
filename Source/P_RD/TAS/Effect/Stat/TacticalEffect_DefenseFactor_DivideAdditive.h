/*****************************************************************//**
 * @file   TacticalEffect_DefenseFactor_DivideAdditive.h
 * @brief  DefenseFactor DivideAdditive 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_DefenseFactor_DivideAdditive.generated.h"

/**
 * @brief DefenseFactor DivideAdditive 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_DefenseFactor_DivideAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_DefenseFactor_DivideAdditive();
};
