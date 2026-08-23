#include "TAS/Effect/Tag/TacticalEffect_Weakness.h"
#include "GameplayTagType.h"

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
