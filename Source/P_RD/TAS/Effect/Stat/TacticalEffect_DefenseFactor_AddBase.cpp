/*****************************************************************//**
 * @file   TacticalEffect_DefenseFactor_AddBase.cpp
 * @brief  DefenseFactor AddBase 이펙트 구현
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_DefenseFactor_AddBase.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_DefenseFactor_AddBase::UTacticalEffect_DefenseFactor_AddBase()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetDefenseFactorAttribute();
	Info.mModifierOp = ETacticalModOp::AddBase;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
