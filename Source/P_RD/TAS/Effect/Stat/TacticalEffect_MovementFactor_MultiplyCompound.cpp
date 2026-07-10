/*****************************************************************//**
 * @file   TacticalEffect_MovementFactor_MultiplyCompound.cpp
 * @brief  MovementFactor MultiplyCompound 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_MovementFactor_MultiplyCompound.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_MovementFactor_MultiplyCompound::UTacticalEffect_MovementFactor_MultiplyCompound()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetMovementFactorAttribute();
	Info.mModifierOp = ETacticalModOp::MultiplyCompound;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
