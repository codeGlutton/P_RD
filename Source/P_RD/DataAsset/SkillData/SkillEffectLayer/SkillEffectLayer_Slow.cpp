#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Slow.h"
#include "TAS/Effect/Tag/TacticalEffect_Slow.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Slow::GetTagEffectClass() const
{
	return UTacticalEffect_GetSlow ::StaticClass();
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_Slow"

FText FSkillEffectLayer_Slow::GetTagDisplayName() const
{
	return LOCTEXT("SlowName", "둔화");
}

#undef LOCTEXT_NAMESPACE

