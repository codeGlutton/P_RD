/*****************************************************************//**
 * @file   TacticalEffect_ActionPointFactor_Override.cpp
 * @brief  ActionPointFactor Override 이펙트 구현
 * @author 이문환, 모호재
 * @date   2026-08-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_ActionPointFactor_Override.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TacticalEffect_ActionPointFactor_Override)

UTacticalEffect_ActionPointFactor_Override::UTacticalEffect_ActionPointFactor_Override()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetActionPointFactorAttribute();
	Info.mModifierOp = ETacticalModOp::Override;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
