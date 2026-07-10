/*****************************************************************//**
 * @file   TacticalEffect_AttackFactor_Override.cpp
 * @brief  AttackFactor Override 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_AttackFactor_Override.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_AttackFactor_Override::UTacticalEffect_AttackFactor_Override()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetAttackFactorAttribute();
	Info.mModifierOp = ETacticalModOp::Override;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
