#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Vigor.h"
#include "TAS/Effect/Tag/TacticalEffect_Vigor.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Vigor::GetTagEffectClass() const
{
    return UTacticalEffect_GetVigor::StaticClass();
}

#if WITH_EDITOR
#define LOCTEXT_NAMESPACE "SkillEffectLayer_Vigor"

FText FSkillEffectLayer_Vigor::GetTagDisplayName() const
{
	return LOCTEXT("VigorName", "활력");
}

#undef LOCTEXT_NAMESPACE
#endif

