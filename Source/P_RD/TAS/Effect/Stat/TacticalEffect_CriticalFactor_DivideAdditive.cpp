/*****************************************************************//**
 * @file   TacticalEffect_CriticalFactor_DivideAdditive.cpp
 * @brief  CriticalFactor DivideAdditive 이펙트 구현
 * @author 모호재
 * @date   2026-08-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_CriticalFactor_DivideAdditive.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TacticalEffect_CriticalFactor_DivideAdditive)

UTacticalEffect_CriticalFactor_DivideAdditive::UTacticalEffect_CriticalFactor_DivideAdditive()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetCriticalFactorAttribute();
	Info.mModifierOp = ETacticalModOp::DivideAdditive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
