/*****************************************************************//**
 * @file   TacticalEffect_Fortification.h
 * @brief  Fortification 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-07-06
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_Fortification.generated.h"

/**
 * @brief  Fortification 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Fortification : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_Fortification();

	/* UTacticalEffect 상속 */
public:
	void OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const override;
};
