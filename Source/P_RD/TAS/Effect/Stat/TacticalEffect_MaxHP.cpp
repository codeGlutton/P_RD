#include "TAS/Effect/Stat/TacticalEffect_MaxHP.h"
#include "AttributeSet/CombatTargetAttributeSet.h"
#include "Simulation/Logger/EventLogger.h"

#include "TAS/Effect/TacticalEffectContext.h"

UTacticalEffect_MaxHP::UTacticalEffect_MaxHP()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetMaxHPAttribute();
	Info.mModifierOp = ETacticalModOp::AddBase;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}

void UTacticalEffect_MaxHP::OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	Super::OnExecuted(ActiveTEContainer, TESpec);

	FSRPGAttributeEffectEventLog Log;
	Log.mEffectAttribute = UCombatTargetAttributeSet::GetMaxHPAttribute();
	Log.mMagnitude = TESpec.GetModifiedAttribute(UCombatTargetAttributeSet::GetMaxHPAttribute())->mTotalMagnitude;

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	const UActorModel* Target = AttributeSetCompModelInstance->GetOwnerModel();

	GetWorldEventLogger(Target)->LogAttributeEffect(Target->GetModelId(), Target->GetClass(), Log);
}
