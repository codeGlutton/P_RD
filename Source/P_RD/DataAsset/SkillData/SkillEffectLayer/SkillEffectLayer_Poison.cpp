#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Poison.h"
#include "TAS/Effect/Tag/TacticalEffect_Poison.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Poison::GetTagEffectClass() const
{
	return UTacticalEffect_GetPoison::StaticClass();
}

#if WITH_EDITOR
#define LOCTEXT_NAMESPACE "SkillEffectLayer_Poison"

FText FSkillEffectLayer_Poison::GetTagDisplayName() const
{
	return LOCTEXT("PoisonName", "중독");
}

#undef LOCTEXT_NAMESPACE
#endif
