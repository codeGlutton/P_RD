/*****************************************************************//**
 * @file   TacticalEffect_HealFactor_AddBase.cpp
 * @brief  HealFactor AddBase 이펙트 구현
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_HealFactor_AddBase.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

UTacticalEffect_HealFactor_AddBase::UTacticalEffect_HealFactor_AddBase()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetHealFactorAttribute();
	Info.mModifierOp = ETacticalModOp::AddBase;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
