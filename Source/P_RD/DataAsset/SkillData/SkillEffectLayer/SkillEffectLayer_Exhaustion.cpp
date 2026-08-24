#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Exhaustion.h"
#include "TAS/Effect/Tag/TacticalEffect_Exhaustion.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Exhaustion::GetTagEffectClass() const
{
	return UTacticalEffect_GetExhaustion::StaticClass();
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_Exhaustion"

FText FSkillEffectLayer_Exhaustion::GetTagDisplayName() const
{
	return LOCTEXT("ExhaustionName", "탈진");
}

#undef LOCTEXT_NAMESPACE

