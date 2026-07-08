/*****************************************************************//**
 * @file   TacticalEffect_DefenseFactor_DivideAdditive.cpp
 * @brief  DefenseFactor DivideAdditive 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_DefenseFactor_DivideAdditive.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_DefenseFactor_DivideAdditive::UTacticalEffect_DefenseFactor_DivideAdditive()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetDefenseFactorAttribute();
	Info.mModifierOp = ETacticalModOp::DivideAdditive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
