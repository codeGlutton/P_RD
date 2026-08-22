#include "TAS/Effect/Tag/TacticalEffect_Dead.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Simulation/Logger/EventLogger.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UTacticalEffect_Dead::UTacticalEffect_Dead()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_ActorState_Dead);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_ActorState_Dead);
}

UTacticalEffect_AddDead::UTacticalEffect_AddDead()
{
	mStatusEffect = UTacticalEffect_Dead::StaticClass();
}

UTacticalEffect_GetDead::UTacticalEffect_GetDead()
{
	mStatusEffect = UTacticalEffect_Dead::StaticClass();
}

