/*****************************************************************//**
 * @file   TacticalEffect_DefenseFactor_MultiplyCompound.cpp
 * @brief  DefenseFactor MultiplyCompound 이펙트 구현
 * @author 이문환
 * @date   2026-07-07
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_DefenseFactor_MultiplyCompound.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

UTacticalEffect_DefenseFactor_MultiplyCompound::UTacticalEffect_DefenseFactor_MultiplyCompound()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetDefenseFactorAttribute();
	Info.mModifierOp = ETacticalModOp::MultiplyCompound;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
