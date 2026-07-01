/*****************************************************************//**
 * @file   TacticalEffect_DefenseFactor.cpp
 * @brief  DefenseFactor 이펙트 구현
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_DefenseFactor.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_DefenseFactor::UTacticalEffect_DefenseFactor()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetDefenseFactorAttribute();
	Info.mModifierOp = ETacticalModOp::Additive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
