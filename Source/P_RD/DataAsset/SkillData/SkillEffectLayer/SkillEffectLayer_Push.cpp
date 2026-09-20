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
		LOCTEXT("PushFormat", "시전자가 바라보는 방향으로 {0}칸 밀어냅니다."),
		FText::AsNumber(mTagGain)
	);
}

#undef LOCTEXT_NAMESPACE
