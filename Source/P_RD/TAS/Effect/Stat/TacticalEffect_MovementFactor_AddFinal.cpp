/*****************************************************************//**
 * @file   TacticalEffect_MovementFactor_AddFinal.cpp
 * @brief  MovementFactor AddFinal 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_MovementFactor_AddFinal.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

UTacticalEffect_MovementFactor_AddFinal::UTacticalEffect_MovementFactor_AddFinal()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetMovementFactorAttribute();
	Info.mModifierOp = ETacticalModOp::AddFinal;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
