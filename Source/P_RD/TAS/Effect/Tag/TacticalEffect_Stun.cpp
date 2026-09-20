#include "TAS/Effect/Tag/TacticalEffect_Stun.h"
#include "GameplayTagType.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UTacticalEffect_Stun::UTacticalEffect_Stun()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Stun);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Stun);
}

UTacticalEffect_AddStun::UTacticalEffect_AddStun()
{
	mStatusEffect = UTacticalEffect_Stun::StaticClass();
}

UTacticalEffect_GetStun::UTacticalEffect_GetStun()
{
	mStatusEffect = UTacticalEffect_Stun::StaticClass();
}

bool UTacticalEffect_GetStun::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
{
	if (Super::CanApply(ActiveTEContainer, TESpec) == false)
	{
		return false;
	}

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	if (AttributeSetCompModelInstance != nullptr && AttributeSetCompModelInstance->HasMatchingGameplayTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_ControlImmunity) == true)
	{
		return false;
	}

	return true;
}
