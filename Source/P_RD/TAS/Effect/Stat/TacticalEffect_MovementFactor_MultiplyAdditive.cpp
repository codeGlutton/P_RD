/*****************************************************************//**
 * @file   TacticalEffect_MovementFactor_MultiplyAdditive.cpp
 * @brief  MovementFactor MultiplyAdditive 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_MovementFactor_MultiplyAdditive.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_MovementFactor_MultiplyAdditive::UTacticalEffect_MovementFactor_MultiplyAdditive()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetMovementFactorAttribute();
	Info.mModifierOp = ETacticalModOp::MultiplyAdditive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
