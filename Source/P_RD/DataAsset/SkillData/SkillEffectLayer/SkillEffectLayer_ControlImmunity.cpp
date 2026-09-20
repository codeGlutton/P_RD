#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_ControlImmunity.h"
#include "TAS/Effect/Tag/TacticalEffect_ControlImmunity.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_ControlImmunity::GetTagEffectClass() const
{
	return UTacticalEffect_GetControlImmunity::StaticClass();
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_ControlImmunity"

FText FSkillEffectLayer_ControlImmunity::GetTagDisplayName() const
{
	return LOCTEXT("ControlImmunityName", "억제 면역");
}

#undef LOCTEXT_NAMESPACE
