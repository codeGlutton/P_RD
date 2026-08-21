#include "TAS/Effect/Tag/TacticalEffect_Stun.h"
#include "GameplayTagType.h"

UTacticalEffect_Stun::UTacticalEffect_Stun()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Stun;
}

UTacticalEffect_GetStun::UTacticalEffect_GetStun()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Stun;
}
