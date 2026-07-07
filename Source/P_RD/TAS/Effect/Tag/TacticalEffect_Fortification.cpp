#include "TAS/Effect/Tag/TacticalEffect_Fortification.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Simulation/Logger/EventLogger.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UTacticalEffect_Fortification::UTacticalEffect_Fortification()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;
}

void UTacticalEffect_Fortification::OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	Super::OnExecuted(ActiveTEContainer, TESpec);

	const int32 TagCount = FMath::Floor(TESpec.mDynamicMagnitude);

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	const UActorModel* Instigator = AttributeSetCompModelInstance->GetOwnerModel();

	AttributeSetCompModelInstance->AddLooseGameplayTag(EffectTags::GameplayEffect_StatusEffect_Buff_Fortification, TagCount);

	FSRPGTagEffectEventLog Log;
	Log.mEffectTag = EffectTags::GameplayEffect_StatusEffect_Buff_Fortification;
	Log.mCount = TagCount;

	GetWorldEventLogger(Instigator)->LogTagEffect(Instigator->GetModelId(), Instigator->GetClass(), Log);
}
