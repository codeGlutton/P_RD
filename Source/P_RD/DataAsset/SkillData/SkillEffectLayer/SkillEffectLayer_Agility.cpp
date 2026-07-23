#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Agility.h"
#include "TAS/Effect/Tag/TacticalEffect_Agility.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Agility::GetTagEffectClass() const
{
    return UTacticalEffect_Agility::StaticClass();
}

