/*****************************************************************//**
 * @file   TacticalEffect_DefensePoint.cpp
 * @brief  DefensePoint 이펙트 구현
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_DefensePoint.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_DefensePoint::UTacticalEffect_DefensePoint()
{
	// 일시적
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetDefensePointAttribute();
	Info.mModifierOp = ETacticalModOp::AddBase;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
