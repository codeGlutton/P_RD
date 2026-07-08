/*****************************************************************//**
 * @file   TacticalEffect_HealPoint.cpp
 * @brief  HealPoint 이펙트 구현
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_HealPoint.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_HealPoint::UTacticalEffect_HealPoint()
{
	// 일시적
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetHealPointAttribute();
	Info.mModifierOp = ETacticalModOp::AddBase;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
