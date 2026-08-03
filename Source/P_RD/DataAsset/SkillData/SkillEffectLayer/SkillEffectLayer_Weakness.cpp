#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Weakness.h"
#include "TAS/Effect/Tag/TacticalEffect_Weakness.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Weakness::GetTagEffectClass() const
{
    return UTacticalEffect_GetWeakness::StaticClass();
}

#if WITH_EDITOR
#define LOCTEXT_NAMESPACE "SkillEffectLayer_Weakness"

FText FSkillEffectLayer_Weakness::GetTagDisplayName() const
{
	return LOCTEXT("WeaknessName", "약화");
}

#undef LOCTEXT_NAMESPACE
#endif
