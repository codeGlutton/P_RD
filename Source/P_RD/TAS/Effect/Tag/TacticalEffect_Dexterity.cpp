#include "TAS/Effect/Tag/TacticalEffect_Dexterity.h"
#include "GameplayTagType.h"

UTacticalEffect_Buff_Dexterity::UTacticalEffect_Buff_Dexterity()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Infinite_Buff_Dexterity);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Infinite_Buff_Dexterity);
}

UTacticalEffect_AddBuff_Dexterity::UTacticalEffect_AddBuff_Dexterity()
{
	mStatusEffect = UTacticalEffect_Buff_Dexterity::StaticClass();
}

UTacticalEffect_GetBuff_Dexterity::UTacticalEffect_GetBuff_Dexterity()
{
	mStatusEffect = UTacticalEffect_Buff_Dexterity::StaticClass();
}

UTacticalEffect_Debuff_Dexterity::UTacticalEffect_Debuff_Dexterity()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Infinite_Debuff_Dexterity);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Infinite_Debuff_Dexterity);
}

UTacticalEffect_AddDebuff_Dexterity::UTacticalEffect_AddDebuff_Dexterity()
{
	mStatusEffect = UTacticalEffect_Debuff_Dexterity::StaticClass();
}

#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UTacticalEffect_GetDebuff_Dexterity::UTacticalEffect_GetDebuff_Dexterity()
{
	mStatusEffect = UTacticalEffect_Debuff_Dexterity::StaticClass();
}

bool UTacticalEffect_GetDebuff_Dexterity::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
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
