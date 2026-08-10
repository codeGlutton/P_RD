#include "TAS/Effect/Tag/TacticalEffect_Frail.h"
#include "GameplayTagType.h"

UTacticalEffect_Frail::UTacticalEffect_Frail()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Frail;
}

UTacticalEffect_GetFrail::UTacticalEffect_GetFrail()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Frail;
}
