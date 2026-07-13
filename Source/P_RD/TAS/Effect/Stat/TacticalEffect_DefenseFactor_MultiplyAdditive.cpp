/*****************************************************************//**
 * @file   TacticalEffect_DefenseFactor_MultiplyAdditive.cpp
 * @brief  DefenseFactor MultiplyAdditive 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_DefenseFactor_MultiplyAdditive.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

UTacticalEffect_DefenseFactor_MultiplyAdditive::UTacticalEffect_DefenseFactor_MultiplyAdditive()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetDefenseFactorAttribute();
	Info.mModifierOp = ETacticalModOp::MultiplyAdditive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
