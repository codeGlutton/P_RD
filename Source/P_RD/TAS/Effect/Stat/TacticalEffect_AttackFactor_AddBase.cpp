/*****************************************************************//**
 * @file   TacticalEffect_AttackFactor_AddBase.cpp
 * @brief  AttackFactor AddBase 이펙트 구현
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_AttackFactor_AddBase.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

UTacticalEffect_AttackFactor_AddBase::UTacticalEffect_AttackFactor_AddBase()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetAttackFactorAttribute();
	Info.mModifierOp = ETacticalModOp::AddBase;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
