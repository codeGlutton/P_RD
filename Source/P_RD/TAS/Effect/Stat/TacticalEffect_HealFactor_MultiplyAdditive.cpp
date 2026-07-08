/*****************************************************************//**
 * @file   TacticalEffect_HealFactor_MultiplyAdditive.cpp
 * @brief  HealFactor MultiplyAdditive 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_HealFactor_MultiplyAdditive.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_HealFactor_MultiplyAdditive::UTacticalEffect_HealFactor_MultiplyAdditive()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetHealFactorAttribute();
	Info.mModifierOp = ETacticalModOp::MultiplyAdditive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
