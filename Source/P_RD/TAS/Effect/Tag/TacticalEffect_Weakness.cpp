#include "TAS/Effect/Tag/TacticalEffect_Weakness.h"
#include "GameplayTagType.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UTacticalEffect_Weakness::UTacticalEffect_Weakness()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Weakness);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Weakness);
}

UTacticalEffect_AddWeakness::UTacticalEffect_AddWeakness()
{
	mStatusEffect = UTacticalEffect_Weakness::StaticClass();
}

UTacticalEffect_GetWeakness::UTacticalEffect_GetWeakness()
{
	mStatusEffect = UTacticalEffect_Weakness::StaticClass();
}

bool UTacticalEffect_GetWeakness::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
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
