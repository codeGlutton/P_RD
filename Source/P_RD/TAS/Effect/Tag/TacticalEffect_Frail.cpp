#include "TAS/Effect/Tag/TacticalEffect_Frail.h"
#include "GameplayTagType.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UTacticalEffect_Frail::UTacticalEffect_Frail()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Frail);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Frail);
}

UTacticalEffect_AddFrail::UTacticalEffect_AddFrail()
{
	mStatusEffect = UTacticalEffect_Frail::StaticClass();
}

UTacticalEffect_GetFrail::UTacticalEffect_GetFrail()
{
	mStatusEffect = UTacticalEffect_Frail::StaticClass();
}

bool UTacticalEffect_GetFrail::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
{
	if (Super::CanApply(ActiveTEContainer, TESpec) == false)
	{
		return false;
	}

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	if (AttributeSetCompModelInstance != nullptr && AttributeSetCompModelInstance->HasMatchingGameplayTag(EffectTags::GameplayEffect_StatusEffect_Infinite_Buff_WeakeningImmunity) == true)
	{
		return false;
	}

	return true;
}
