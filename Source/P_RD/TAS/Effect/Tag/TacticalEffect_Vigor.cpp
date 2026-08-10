#include "TAS/Effect/Tag/TacticalEffect_Vigor.h"
#include "GameplayTagType.h"

UTacticalEffect_Vigor::UTacticalEffect_Vigor()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Vigor;
}

UTacticalEffect_GetVigor::UTacticalEffect_GetVigor()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Vigor;
}
