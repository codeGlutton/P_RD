#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Agility.h"
#include "TAS/Effect/Tag/TacticalEffect_Agility.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Agility::GetTagEffectClass() const
{
    return UTacticalEffect_Agility::StaticClass();
}

#if WITH_EDITOR
#define LOCTEXT_NAMESPACE "SkillEffectLayer_Agility"

FText FSkillEffectLayer_Agility::GetTagDisplayName() const
{
	return LOCTEXT("AgilityName", "신속");
}

#undef LOCTEXT_NAMESPACE
#endif

