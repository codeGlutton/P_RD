/*****************************************************************//**
 * @file   TacticalEffect_Dead.h
 * @brief  Dead 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-07-09
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_Dead.generated.h"

/**
 * @brief  Dead 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Dead : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_Dead();

	/* UTacticalEffect 상속 */
public:
	void OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const override;
};
