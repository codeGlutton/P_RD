#include "TAS/Effect/Tag/TacticalEffect_Weakness.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Simulation/Logger/EventLogger.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UTacticalEffect_Weakness::UTacticalEffect_Weakness()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;
}

void UTacticalEffect_Weakness::OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	Super::OnExecuted(ActiveTEContainer, TESpec);

	const int32 TagCount = FMath::Floor(TESpec.mDynamicMagnitude);

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	const UActorModel* Instigator = AttributeSetCompModelInstance->GetOwnerModel();

	AttributeSetCompModelInstance->AddLooseGameplayTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Weakness, TagCount);

	FSRPGTagEffectEventLog Log;
	Log.mEffectTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Weakness;
	Log.mCount = TagCount;

	GetWorldEventLogger(Instigator)->LogTagEffect(Instigator->GetModelId(), Instigator->GetClass(), Log);
}
