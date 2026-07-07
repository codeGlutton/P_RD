/*****************************************************************//**
 * @file   TacticalEffect_HealFactor_MultiplyCompound.cpp
 * @brief  HealFactor MultiplyCompound 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_HealFactor_MultiplyCompound.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_HealFactor_MultiplyCompound::UTacticalEffect_HealFactor_MultiplyCompound()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetHealFactorAttribute();
	Info.mModifierOp = ETacticalModOp::MultiplyCompound;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
