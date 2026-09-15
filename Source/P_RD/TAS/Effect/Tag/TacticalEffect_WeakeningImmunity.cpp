#include "TAS/Effect/Tag/TacticalEffect_WeakeningImmunity.h"
#include "GameplayTagType.h"

UTacticalEffect_WeakeningImmunity::UTacticalEffect_WeakeningImmunity()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Infinite_Buff_WeakeningImmunity);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Infinite_Buff_WeakeningImmunity);
}

UTacticalEffect_AddWeakeningImmunity::UTacticalEffect_AddWeakeningImmunity()
{
	mStatusEffect = UTacticalEffect_WeakeningImmunity::StaticClass();
}

UTacticalEffect_GetWeakeningImmunity::UTacticalEffect_GetWeakeningImmunity()
{
	mStatusEffect = UTacticalEffect_WeakeningImmunity::StaticClass();
}
