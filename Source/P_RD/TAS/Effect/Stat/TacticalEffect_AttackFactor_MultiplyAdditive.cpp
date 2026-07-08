/*****************************************************************//**
 * @file   TacticalEffect_AttackFactor_MultiplyAdditive.cpp
 * @brief  AttackFactor MultiplyAdditive 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_AttackFactor_MultiplyAdditive.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_AttackFactor_MultiplyAdditive::UTacticalEffect_AttackFactor_MultiplyAdditive()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetAttackFactorAttribute();
	Info.mModifierOp = ETacticalModOp::MultiplyAdditive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
