#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Pull.h"
#include "TAS/Effect/Tag/TacticalEffect_Pull.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Pull::GetTagEffectClass() const
{
	return UTacticalEffect_GetPull::StaticClass();
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_Pull"

FText FSkillEffectLayer_Pull::MakeDescription() const
{
	return FText::Format(
		LOCTEXT("PushFormat", "시전자 방향으로 {0}칸 끌어옵니다."),
		FText::AsNumber(mTagGain)
	);
}

#undef LOCTEXT_NAMESPACE
