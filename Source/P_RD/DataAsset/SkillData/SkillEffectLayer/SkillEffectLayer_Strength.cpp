#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Strength.h"
#include "TAS/Effect/Tag/TacticalEffect_Strength.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Strength::GetTagEffectClass() const
{
	if (mTagGain < 0)
	{
		return UTacticalEffect_GetDebuff_Strength::StaticClass();
	}

	return UTacticalEffect_GetBuff_Strength::StaticClass();
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_Strength"

FText FSkillEffectLayer_Strength::GetTagDisplayName() const
{
	return LOCTEXT("StrengthName", "완력");
}

#undef LOCTEXT_NAMESPACE
