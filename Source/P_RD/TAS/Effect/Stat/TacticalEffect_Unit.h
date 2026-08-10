/*****************************************************************//**
 * @file   TacticalEffect_Unit.h
 * @brief  유닛 전용 이펙트 추상 클래스 정의 헤더
 * @author 모호재
 * @date   2026-08-09
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_Unit.generated.h"

/**
 * @brief  유닛 전용 이펙트 추상 클래스
 * @details Target이 UnitAttributeSet을 보유하고 있는 경우에만 적용 가능하도록 CanApply를 검사한다.
 */
UCLASS(abstract)
class P_RD_API UTacticalEffect_Unit : public UTacticalEffect
{
	GENERATED_BODY()

	/* UTacticalEffect 상속 */
public:
	virtual bool CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const override;
};
