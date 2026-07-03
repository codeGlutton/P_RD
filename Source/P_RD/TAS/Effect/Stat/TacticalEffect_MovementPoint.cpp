/*****************************************************************//**
 * @file   TacticalEffect_MovementPoint.cpp
 * @brief  MovementPoint 이펙트 구현
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_MovementPoint.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_MovementPoint::UTacticalEffect_MovementPoint()
{
	// 일시적
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetMovementPointAttribute();
	Info.mModifierOp = ETacticalModOp::Additive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
