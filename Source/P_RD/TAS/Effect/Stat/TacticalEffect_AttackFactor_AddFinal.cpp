/*****************************************************************//**
 * @file   TacticalEffect_AttackFactor_AddFinal.cpp
 * @brief  AttackFactor AddFinal 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_AttackFactor_AddFinal.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_AttackFactor_AddFinal::UTacticalEffect_AttackFactor_AddFinal()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetAttackFactorAttribute();
	Info.mModifierOp = ETacticalModOp::AddFinal;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
