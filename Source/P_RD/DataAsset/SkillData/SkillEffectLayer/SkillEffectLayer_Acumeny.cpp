#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Acumeny.h"
#include "TAS/Effect/Tag/TacticalEffect_Acumeny.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Acumeny::GetTagEffectClass() const
{
	if (mTagGain < 0)
	{
		return UTacticalEffect_GetDebuff_Acumeny::StaticClass();
	}

	return UTacticalEffect_GetBuff_Acumeny::StaticClass();
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_Acumeny"

FText FSkillEffectLayer_Acumeny::GetTagDisplayName() const
{
	return LOCTEXT("AcumenyName", "예리함");
}

#undef LOCTEXT_NAMESPACE
