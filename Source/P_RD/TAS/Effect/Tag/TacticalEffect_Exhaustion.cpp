#include "TAS/Effect/Tag/TacticalEffect_Exhaustion.h"
#include "GameplayTagType.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UTacticalEffect_Exhaustion::UTacticalEffect_Exhaustion()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Exhaustion);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Exhaustion);
}

UTacticalEffect_AddExhaustion::UTacticalEffect_AddExhaustion()
{
	mStatusEffect = UTacticalEffect_Exhaustion::StaticClass();
}

UTacticalEffect_GetExhaustion::UTacticalEffect_GetExhaustion()
{
	mStatusEffect = UTacticalEffect_Exhaustion::StaticClass();
}

bool UTacticalEffect_GetExhaustion::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
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
