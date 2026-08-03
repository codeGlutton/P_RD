#include "TAS/Effect/Tag/TacticalEffect_Weakness.h"
#include "GameplayTagType.h"

UTacticalEffect_Weakness::UTacticalEffect_Weakness()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Weakness;
}

UTacticalEffect_GetWeakness::UTacticalEffect_GetWeakness()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Weakness;
}
