#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Pull.h"
#include "TAS/Effect/Tag/TacticalEffect_Pull.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Pull::GetTagEffectClass() const
{
	return UTacticalEffect_GetPull::StaticClass();
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_Pull"

FText FSkillEffectLayer_Pull::MakeDescription() const
{
	return LOCTEXT("PullFormat", "대상을 시전자 옆까지 끌어옵니다.");
}

#undef LOCTEXT_NAMESPACE
