/*****************************************************************//**
 * @file   TacticalEffect_ActionPointFactor_DivideAdditive.cpp
 * @brief  ActionPointFactor DivideAdditive 이펙트 구현
 * @author 이문환, 모호재
 * @date   2026-08-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_ActionPointFactor_DivideAdditive.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TacticalEffect_ActionPointFactor_DivideAdditive)

UTacticalEffect_ActionPointFactor_DivideAdditive::UTacticalEffect_ActionPointFactor_DivideAdditive()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetActionPointFactorAttribute();
	Info.mModifierOp = ETacticalModOp::DivideAdditive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
