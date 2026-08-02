#include "TAS/Effect/Tag/TacticalEffect_Fortification.h"
#include "GameplayTagType.h"

UTacticalEffect_Fortification::UTacticalEffect_Fortification()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Fortification;
}

UTacticalEffect_GetFortification::UTacticalEffect_GetFortification()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Fortification;
}
