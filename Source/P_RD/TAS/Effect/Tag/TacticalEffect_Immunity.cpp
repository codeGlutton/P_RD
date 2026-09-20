#include "TAS/Effect/Tag/TacticalEffect_Immunity.h"
#include "GameplayTagType.h"

UTacticalEffect_Immunity::UTacticalEffect_Immunity()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_ActorState_Immunity);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_ActorState_Immunity);
}

UTacticalEffect_AddImmunity::UTacticalEffect_AddImmunity()
{
	mActorStateEffect = UTacticalEffect_Immunity::StaticClass();
}

UTacticalEffect_GetImmunity::UTacticalEffect_GetImmunity()
{
	mActorStateEffect = UTacticalEffect_Immunity::StaticClass();
}
