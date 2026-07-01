/*****************************************************************//**
 * @file   TacticalEffect_AttackPoint.cpp
 * @brief  AttackPoint 이펙트 구현
 * @author 이문환
 * @date   2026-06-26
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_AttackPoint.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_AttackPoint::UTacticalEffect_AttackPoint()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetAttackPointAttribute();
	Info.mModifierOp = ETacticalModOp::Additive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
