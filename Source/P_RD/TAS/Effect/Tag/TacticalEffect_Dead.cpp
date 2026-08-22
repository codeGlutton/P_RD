#include "TAS/Effect/Tag/TacticalEffect_Dead.h"
#include "GameplayTagType.h"

UTacticalEffect_Dead::UTacticalEffect_Dead()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_ActorState_Dead);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_ActorState_Dead);
}

UTacticalEffect_AddDead::UTacticalEffect_AddDead()
{
	mActorStateEffect = UTacticalEffect_Dead::StaticClass();
}

UTacticalEffect_GetDead::UTacticalEffect_GetDead()
{
	mActorStateEffect = UTacticalEffect_Dead::StaticClass();
}
