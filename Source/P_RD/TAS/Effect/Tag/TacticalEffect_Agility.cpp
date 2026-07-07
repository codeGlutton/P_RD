#include "TAS/Effect/Tag/TacticalEffect_Agility.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Simulation/Logger/EventLogger.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UTacticalEffect_Agility::UTacticalEffect_Agility()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;
}

void UTacticalEffect_Agility::OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	Super::OnExecuted(ActiveTEContainer, TESpec);

	const int32 TagCount = FMath::Floor(TESpec.mDynamicMagnitude);

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	const UActorModel* Instigator = AttributeSetCompModelInstance->GetOwnerModel();

	AttributeSetCompModelInstance->AddLooseGameplayTag(EffectTags::GameplayEffect_StatusEffect_Buff_Agility, TagCount);

	FSRPGTagEffectEventLog Log;
	Log.mEffectTag = EffectTags::GameplayEffect_StatusEffect_Buff_Agility;
	Log.mCount = TagCount;

	GetWorldEventLogger(Instigator)->LogTagEffect(Instigator->GetModelId(), Instigator->GetClass(), Log);
}
