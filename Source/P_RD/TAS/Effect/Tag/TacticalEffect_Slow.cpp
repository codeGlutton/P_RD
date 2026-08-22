#include "TAS/Effect/Tag/TacticalEffect_Slow.h"

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


