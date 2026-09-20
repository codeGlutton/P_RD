#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Haste.h"
#include "TAS/Effect/Tag/TacticalEffect_Haste.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Haste::GetTagEffectClass() const
{
	return UTacticalEffect_GetHaste::StaticClass();
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_Haste"

FText FSkillEffectLayer_Haste::GetTagDisplayName() const
{
	return LOCTEXT("HasteName", "신속");
}

#undef LOCTEXT_NAMESPACE

