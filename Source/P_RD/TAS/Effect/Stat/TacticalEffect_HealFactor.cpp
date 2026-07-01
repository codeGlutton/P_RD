/*****************************************************************//**
 * @file   TacticalEffect_HealFactor.cpp
 * @brief  HealFactor 이펙트 구현
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_HealFactor.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_HealFactor::UTacticalEffect_HealFactor()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetHealFactorAttribute();
	Info.mModifierOp = ETacticalModOp::Additive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
