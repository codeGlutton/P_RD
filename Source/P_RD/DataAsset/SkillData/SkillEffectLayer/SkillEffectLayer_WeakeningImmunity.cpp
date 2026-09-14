#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_WeakeningImmunity.h"
#include "TAS/Effect/Tag/TacticalEffect_WeakeningImmunity.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_WeakeningImmunity::GetTagEffectClass() const
{
	return UTacticalEffect_GetWeakeningImmunity::StaticClass();
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_WeakeningImmunity"

FText FSkillEffectLayer_WeakeningImmunity::GetTagDisplayName() const
{
	return LOCTEXT("WeakeningImmunityName", "쇠약 면역");
}

#undef LOCTEXT_NAMESPACE
