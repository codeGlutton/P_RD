#include "TAS/Effect/Tag/TacticalEffect_ControlImmunity.h"
#include "GameplayTagType.h"

UTacticalEffect_ControlImmunity::UTacticalEffect_ControlImmunity()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_ControlImmunity);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_ControlImmunity);
}

UTacticalEffect_AddControlImmunity::UTacticalEffect_AddControlImmunity()
{
	mStatusEffect = UTacticalEffect_ControlImmunity::StaticClass();
}

UTacticalEffect_GetControlImmunity::UTacticalEffect_GetControlImmunity()
{
	mStatusEffect = UTacticalEffect_ControlImmunity::StaticClass();
}
