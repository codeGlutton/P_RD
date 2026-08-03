#include "TAS/Effect/Tag/TacticalEffect_Agility.h"
#include "GameplayTagType.h"

UTacticalEffect_Agility::UTacticalEffect_Agility()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Agility;
}

UTacticalEffect_GetAgility::UTacticalEffect_GetAgility()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Agility;
}
