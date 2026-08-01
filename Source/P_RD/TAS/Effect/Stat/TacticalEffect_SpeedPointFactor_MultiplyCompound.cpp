/*****************************************************************//**
 * @file   TacticalEffect_SpeedPointFactor_MultiplyCompound.cpp
 * @brief  SpeedPointFactor MultiplyCompound 이펙트 구현
 * @author 모호재
 * @date   2026-08-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_SpeedPointFactor_MultiplyCompound.h"
#include "AttributeSet/UnitAttributeSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TacticalEffect_SpeedPointFactor_MultiplyCompound)

UTacticalEffect_SpeedPointFactor_MultiplyCompound::UTacticalEffect_SpeedPointFactor_MultiplyCompound()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetSpeedPointFactorAttribute();
	Info.mModifierOp = ETacticalModOp::MultiplyCompound;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
