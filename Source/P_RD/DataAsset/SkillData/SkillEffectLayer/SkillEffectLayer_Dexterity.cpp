#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Dexterity.h"
#include "TAS/Effect/Tag/TacticalEffect_Dexterity.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Dexterity::GetTagEffectClass() const
{
	if (mTagGain < 0)
	{
		return UTacticalEffect_GetDebuff_Dexterity::StaticClass();
	}

	return UTacticalEffect_GetBuff_Dexterity::StaticClass();
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_Dexterity"

FText FSkillEffectLayer_Dexterity::GetTagDisplayName() const
{
	return LOCTEXT("DexterityName", "재치");
}

#undef LOCTEXT_NAMESPACE
