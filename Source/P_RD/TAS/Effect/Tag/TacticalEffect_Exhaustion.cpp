#include "TAS/Effect/Tag/TacticalEffect_Exhaustion.h"
#include "GameplayTagType.h"

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
