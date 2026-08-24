#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Fortification.h"
#include "TAS/Effect/Tag/TacticalEffect_Fortification.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Fortification::GetTagEffectClass() const
{
    return UTacticalEffect_GetFortification::StaticClass();
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_Fortification"

FText FSkillEffectLayer_Fortification::GetTagDisplayName() const
{
	return LOCTEXT("FortificationName", "요새화");
}

#undef LOCTEXT_NAMESPACE
