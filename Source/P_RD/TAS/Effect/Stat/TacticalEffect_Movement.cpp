/*****************************************************************//**
 * @file   TacticalEffect_Movement.cpp
 * @brief  Movement 이펙트 구현
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_Movement.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_Movement::UTacticalEffect_Movement()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetMovementAttribute();
	Info.mModifierOp = ETacticalModOp::Additive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
