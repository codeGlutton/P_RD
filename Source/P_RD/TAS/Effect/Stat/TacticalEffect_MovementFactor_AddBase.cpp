/*****************************************************************//**
 * @file   TacticalEffect_MovementFactor_AddBase.cpp
 * @brief  MovementFactor AddBase 이펙트 구현
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_MovementFactor_AddBase.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

UTacticalEffect_MovementFactor_AddBase::UTacticalEffect_MovementFactor_AddBase()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetMovementFactorAttribute();
	Info.mModifierOp = ETacticalModOp::AddBase;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
