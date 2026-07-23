#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Fortification.h"
#include "TAS/Effect/Tag/TacticalEffect_Fortification.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Fortification::GetTagEffectClass() const
{
    return UTacticalEffect_Fortification::StaticClass();
}
