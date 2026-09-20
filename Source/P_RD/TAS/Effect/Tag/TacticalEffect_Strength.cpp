#include "TAS/Effect/Tag/TacticalEffect_Strength.h"
#include "GameplayTagType.h"

UTacticalEffect_Buff_Strength::UTacticalEffect_Buff_Strength()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Infinite_Buff_Strength);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Infinite_Buff_Strength);
}

UTacticalEffect_AddBuff_Strength::UTacticalEffect_AddBuff_Strength()
{
	mStatusEffect = UTacticalEffect_Buff_Strength::StaticClass();
}

UTacticalEffect_GetBuff_Strength::UTacticalEffect_GetBuff_Strength()
{
	mStatusEffect = UTacticalEffect_Buff_Strength::StaticClass();
}

UTacticalEffect_Debuff_Strength::UTacticalEffect_Debuff_Strength()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Infinite_Debuff_Strength);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Infinite_Debuff_Strength);
}

UTacticalEffect_AddDebuff_Strength::UTacticalEffect_AddDebuff_Strength()
{
	mStatusEffect = UTacticalEffect_Debuff_Strength::StaticClass();
}

#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UTacticalEffect_GetDebuff_Strength::UTacticalEffect_GetDebuff_Strength()
{
	mStatusEffect = UTacticalEffect_Debuff_Strength::StaticClass();
}

bool UTacticalEffect_GetDebuff_Strength::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
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
