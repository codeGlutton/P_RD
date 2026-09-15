#include "TAS/Effect/Tag/TacticalEffect_ForcedMovementImmunity.h"
#include "GameplayTagType.h"

UTacticalEffect_ForcedMovementImmunity::UTacticalEffect_ForcedMovementImmunity()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_ForcedMovementImmunity);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_ForcedMovementImmunity);
}

UTacticalEffect_AddForcedMovementImmunity::UTacticalEffect_AddForcedMovementImmunity()
{
	mStatusEffect = UTacticalEffect_ForcedMovementImmunity::StaticClass();
}

UTacticalEffect_GetForcedMovementImmunity::UTacticalEffect_GetForcedMovementImmunity()
{
	mStatusEffect = UTacticalEffect_ForcedMovementImmunity::StaticClass();
}
