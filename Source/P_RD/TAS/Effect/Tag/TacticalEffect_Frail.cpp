#include "TAS/Effect/Tag/TacticalEffect_Frail.h"
#include "GameplayTagType.h"

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
