#include "TAS/Effect/Stat/TacticalEffect_SpeedPointFactor.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_SpeedPointFactor_AddBase::UTacticalEffect_SpeedPointFactor_AddBase()
{
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetSpeedPointFactorAttribute();
	Info.mModifierOp = ETacticalModOp::AddBase;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}

UTacticalEffect_SpeedPointFactor_AddFinal::UTacticalEffect_SpeedPointFactor_AddFinal()
{
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetSpeedPointFactorAttribute();
	Info.mModifierOp = ETacticalModOp::AddFinal;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}

UTacticalEffect_SpeedPointFactor_DivideAdditive::UTacticalEffect_SpeedPointFactor_DivideAdditive()
{
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetSpeedPointFactorAttribute();
	Info.mModifierOp = ETacticalModOp::DivideAdditive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}

UTacticalEffect_SpeedPointFactor_MultiplyAdditive::UTacticalEffect_SpeedPointFactor_MultiplyAdditive()
{
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetSpeedPointFactorAttribute();
	Info.mModifierOp = ETacticalModOp::MultiplyAdditive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}

UTacticalEffect_SpeedPointFactor_MultiplyCompound::UTacticalEffect_SpeedPointFactor_MultiplyCompound()
{
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetSpeedPointFactorAttribute();
	Info.mModifierOp = ETacticalModOp::MultiplyCompound;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}

UTacticalEffect_SpeedPointFactor_Override::UTacticalEffect_SpeedPointFactor_Override()
{
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetSpeedPointFactorAttribute();
	Info.mModifierOp = ETacticalModOp::Override;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
