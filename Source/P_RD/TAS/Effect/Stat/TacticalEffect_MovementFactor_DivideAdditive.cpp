/*****************************************************************//**
 * @file   TacticalEffect_MovementFactor_DivideAdditive.cpp
 * @brief  MovementFactor DivideAdditive 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_MovementFactor_DivideAdditive.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

UTacticalEffect_MovementFactor_DivideAdditive::UTacticalEffect_MovementFactor_DivideAdditive()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetMovementFactorAttribute();
	Info.mModifierOp = ETacticalModOp::DivideAdditive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
