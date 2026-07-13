/*****************************************************************//**
 * @file   TacticalEffect_MovementFactor_Override.cpp
 * @brief  MovementFactor Override 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_MovementFactor_Override.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

UTacticalEffect_MovementFactor_Override::UTacticalEffect_MovementFactor_Override()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetMovementFactorAttribute();
	Info.mModifierOp = ETacticalModOp::Override;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
