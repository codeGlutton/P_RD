/*****************************************************************//**
 * @file   TacticalEffect_AttackFactor_MultiplyCompound.cpp
 * @brief  AttackFactor MultiplyCompound 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_AttackFactor_MultiplyCompound.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_AttackFactor_MultiplyCompound::UTacticalEffect_AttackFactor_MultiplyCompound()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetAttackFactorAttribute();
	Info.mModifierOp = ETacticalModOp::MultiplyCompound;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
