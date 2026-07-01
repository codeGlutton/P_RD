/*****************************************************************//**
 * @file   TacticalEffect_AttackFactor.cpp
 * @brief  AttackFactor 이펙트 구현
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_AttackFactor.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_AttackFactor::UTacticalEffect_AttackFactor()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetAttackFactorAttribute();
	Info.mModifierOp = ETacticalModOp::Additive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
