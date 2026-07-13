/*****************************************************************//**
 * @file   TacticalEffect_HealFactor_Override.cpp
 * @brief  HealFactor Override 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_HealFactor_Override.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

UTacticalEffect_HealFactor_Override::UTacticalEffect_HealFactor_Override()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetHealFactorAttribute();
	Info.mModifierOp = ETacticalModOp::Override;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
