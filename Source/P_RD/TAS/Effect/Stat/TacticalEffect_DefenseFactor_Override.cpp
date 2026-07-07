/*****************************************************************//**
 * @file   TacticalEffect_DefenseFactor_Override.cpp
 * @brief  DefenseFactor Override 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_DefenseFactor_Override.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_DefenseFactor_Override::UTacticalEffect_DefenseFactor_Override()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetDefenseFactorAttribute();
	Info.mModifierOp = ETacticalModOp::Override;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
