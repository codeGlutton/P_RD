/*****************************************************************//**
 * @file   TacticalEffect_HealFactor_DivideAdditive.cpp
 * @brief  HealFactor DivideAdditive 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_HealFactor_DivideAdditive.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

UTacticalEffect_HealFactor_DivideAdditive::UTacticalEffect_HealFactor_DivideAdditive()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetHealFactorAttribute();
	Info.mModifierOp = ETacticalModOp::DivideAdditive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
