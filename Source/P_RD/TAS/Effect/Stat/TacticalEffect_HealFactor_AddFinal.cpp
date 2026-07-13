/*****************************************************************//**
 * @file   TacticalEffect_HealFactor_AddFinal.cpp
 * @brief  HealFactor AddFinal 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_HealFactor_AddFinal.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

UTacticalEffect_HealFactor_AddFinal::UTacticalEffect_HealFactor_AddFinal()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetHealFactorAttribute();
	Info.mModifierOp = ETacticalModOp::AddFinal;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
