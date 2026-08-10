#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Frail.h"
#include "TAS/Effect/Tag/TacticalEffect_Frail.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Frail::GetTagEffectClass() const
{
    return UTacticalEffect_GetFrail::StaticClass();
}

#if WITH_EDITOR
#define LOCTEXT_NAMESPACE "SkillEffectLayer_Frail"

FText FSkillEffectLayer_Frail::GetTagDisplayName() const
{
	return LOCTEXT("FrailName", "손상");
}

#undef LOCTEXT_NAMESPACE
#endif
