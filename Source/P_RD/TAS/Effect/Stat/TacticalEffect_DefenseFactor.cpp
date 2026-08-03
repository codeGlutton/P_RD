#include "TAS/Effect/Stat/TacticalEffect_DefenseFactor.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

UTacticalEffect_DefenseFactor_AddBase::UTacticalEffect_DefenseFactor_AddBase()
{
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetDefenseFactorAttribute();
	Info.mModifierOp = ETacticalModOp::AddBase;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}

UTacticalEffect_DefenseFactor_AddFinal::UTacticalEffect_DefenseFactor_AddFinal()
{
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetDefenseFactorAttribute();
	Info.mModifierOp = ETacticalModOp::AddFinal;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}

UTacticalEffect_DefenseFactor_DivideAdditive::UTacticalEffect_DefenseFactor_DivideAdditive()
{
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetDefenseFactorAttribute();
	Info.mModifierOp = ETacticalModOp::DivideAdditive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}

UTacticalEffect_DefenseFactor_MultiplyAdditive::UTacticalEffect_DefenseFactor_MultiplyAdditive()
{
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetDefenseFactorAttribute();
	Info.mModifierOp = ETacticalModOp::MultiplyAdditive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}

UTacticalEffect_DefenseFactor_MultiplyCompound::UTacticalEffect_DefenseFactor_MultiplyCompound()
{
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetDefenseFactorAttribute();
	Info.mModifierOp = ETacticalModOp::MultiplyCompound;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}

UTacticalEffect_DefenseFactor_Override::UTacticalEffect_DefenseFactor_Override()
{
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetDefenseFactorAttribute();
	Info.mModifierOp = ETacticalModOp::Override;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
