#include "TAS/Effect/Tag/TacticalEffect_Poison.h"
#include "GameplayTagType.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "TAS/Effect/Stat/TacticalEffect_HP.h"

UTacticalEffect_Poison::UTacticalEffect_Poison()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Poison);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff);
}

void UTacticalEffect_Poison::OnReduceTimeRemaining(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	Super::OnReduceTimeRemaining(ActiveTEContainer, TESpec);

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	if (AttributeSetCompModelInstance == nullptr)
	{
		return;
	}

	UTacticalEffectContext* Context = AttributeSetCompModelInstance->MakeEffectContext();
	TSharedPtr<FTacticalEffectSpec> NewSpec = AttributeSetCompModelInstance->MakeOutgoingSpec(UTacticalEffect_HP::StaticClass(), Context);
	NewSpec->mDynamicMagnitude = TESpec.GetStackCount();
	AttributeSetCompModelInstance->ApplyTacticalEffectSpecToSelf(*NewSpec);
}

UTacticalEffect_AddPoison::UTacticalEffect_AddPoison()
{
	mStatusEffect = UTacticalEffect_Poison::StaticClass();
}

UTacticalEffect_GetPoison::UTacticalEffect_GetPoison()
{
	mStatusEffect = UTacticalEffect_Poison::StaticClass();
}
