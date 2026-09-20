/*****************************************************************//**
 * @file   TacticalEffect_MaxHP.h
 * @brief  MaxHP 이펙트 정의 헤더
 * @author 이문환, 모호재
 * @date   2026-07-01
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_MaxHP.generated.h"

/**
 * @brief MaxHP 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_MaxHP : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_MaxHP();

	/* UTacticalEffect 상속 */
public:
	void OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const override;
};
