/*****************************************************************//**
 * @file   TacticalEffect_AttackFactor_DivideAdditive.cpp
 * @brief  AttackFactor DivideAdditive 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_AttackFactor_DivideAdditive.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_AttackFactor_DivideAdditive::UTacticalEffect_AttackFactor_DivideAdditive()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetAttackFactorAttribute();
	Info.mModifierOp = ETacticalModOp::DivideAdditive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
