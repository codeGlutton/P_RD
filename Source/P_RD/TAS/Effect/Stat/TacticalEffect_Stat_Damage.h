/*****************************************************************//**
 * @file   TacticalEffect_Stat_Damage.h
 * @brief  데미지 Effect 구현 헤더
 * @author 김준형
 * @date   2026-06-28
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_Stat_Damage.generated.h"

/**
 * @brief  데미지 Effect
 */
UCLASS()
class P_RD_API UTacticalEffect_Stat_Damage : public UTacticalEffect
{
	GENERATED_BODY()
	
public:
	UTacticalEffect_Stat_Damage();
};
