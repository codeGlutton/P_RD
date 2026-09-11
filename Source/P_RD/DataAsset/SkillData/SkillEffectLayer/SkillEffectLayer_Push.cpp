#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Push.h"
#include "TAS/Effect/Tag/TacticalEffect_Push.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Push::GetTagEffectClass() const
{
	return UTacticalEffect_GetPush::StaticClass();
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_Push"

FText FSkillEffectLayer_Push::MakeDescription() const
{
	return FText::Format(
		LOCTEXT("PushFormat", "대상을 {0}칸 밀어냅니다."),
		FText::AsNumber(mTagGain)
	);
}

#undef LOCTEXT_NAMESPACE
