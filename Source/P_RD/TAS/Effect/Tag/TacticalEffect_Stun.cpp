#include "TAS/Effect/Tag/TacticalEffect_Stun.h"
#include "GameplayTagType.h"

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
