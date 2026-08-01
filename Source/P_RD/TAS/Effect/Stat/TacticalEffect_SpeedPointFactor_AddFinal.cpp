/*****************************************************************//**
 * @file   TacticalEffect_SpeedPointFactor_AddFinal.cpp
 * @brief  SpeedPointFactor AddFinal 이펙트 구현
 * @author 모호재
 * @date   2026-08-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_SpeedPointFactor_AddFinal.h"
#include "AttributeSet/UnitAttributeSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TacticalEffect_SpeedPointFactor_AddFinal)

UTacticalEffect_SpeedPointFactor_AddFinal::UTacticalEffect_SpeedPointFactor_AddFinal()
{
	// 지속형
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetSpeedPointFactorAttribute();
	Info.mModifierOp = ETacticalModOp::AddFinal;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
