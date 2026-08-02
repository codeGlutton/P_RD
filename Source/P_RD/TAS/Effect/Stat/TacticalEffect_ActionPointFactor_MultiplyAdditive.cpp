/*****************************************************************//**
 * @file   TacticalEffect_ActionPointFactor_MultiplyAdditive.cpp
 * @brief  ActionPointFactor MultiplyAdditive 이펙트 구현
 * @author 이문환, 모호재
 * @date   2026-08-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_ActionPointFactor_MultiplyAdditive.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TacticalEffect_ActionPointFactor_MultiplyAdditive)

UTacticalEffect_ActionPointFactor_MultiplyAdditive::UTacticalEffect_ActionPointFactor_MultiplyAdditive()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetActionPointFactorAttribute();
	Info.mModifierOp = ETacticalModOp::MultiplyAdditive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
