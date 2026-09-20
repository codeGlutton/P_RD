#include "TAS/Effect/Tag/TacticalEffect_Vigor.h"
#include "GameplayTagType.h"

UTacticalEffect_Vigor::UTacticalEffect_Vigor()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Vigor);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Vigor);
}

UTacticalEffect_AddVigor::UTacticalEffect_AddVigor()
{
	mStatusEffect = UTacticalEffect_Vigor::StaticClass();
}

UTacticalEffect_GetVigor::UTacticalEffect_GetVigor()
{
	mStatusEffect = UTacticalEffect_Vigor::StaticClass();
}
