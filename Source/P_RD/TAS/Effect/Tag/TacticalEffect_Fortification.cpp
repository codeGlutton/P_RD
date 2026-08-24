#include "TAS/Effect/Tag/TacticalEffect_Fortification.h"
#include "GameplayTagType.h"

UTacticalEffect_Fortification::UTacticalEffect_Fortification()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Fortification);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Fortification);
}

UTacticalEffect_AddFortification::UTacticalEffect_AddFortification()
{
	mStatusEffect = UTacticalEffect_Fortification::StaticClass();
}

UTacticalEffect_GetFortification::UTacticalEffect_GetFortification()
{
	mStatusEffect = UTacticalEffect_Fortification::StaticClass();
}
