#include "TAS/Effect/Tag/TacticalEffect_Slow.h"
#include "GameplayTagType.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UTacticalEffect_Slow::UTacticalEffect_Slow()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Slow);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Slow);
}

UTacticalEffect_AddSlow::UTacticalEffect_AddSlow()
{
	mStatusEffect = UTacticalEffect_Slow::StaticClass();
}

UTacticalEffect_GetSlow::UTacticalEffect_GetSlow()
{
	mStatusEffect = UTacticalEffect_Slow::StaticClass();
}

bool UTacticalEffect_GetSlow::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
{
	if (Super::CanApply(ActiveTEContainer, TESpec) == false)
	{
		return false;
	}

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	if (AttributeSetCompModelInstance != nullptr && AttributeSetCompModelInstance->HasMatchingGameplayTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_WeakeningImmunity) == true)
	{
		return false;
	}

	return true;
}


