/*****************************************************************//**
 * @file   TacticalEffect_MovementFactor.cpp
 * @brief  MovementFactor 이펙트 구현
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_MovementFactor.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_MovementFactor::UTacticalEffect_MovementFactor()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetMovementFactorAttribute();
	Info.mModifierOp = ETacticalModOp::Additive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
