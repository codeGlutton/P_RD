#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Weakness.h"
#include "TAS/Effect/Tag/TacticalEffect_Weakness.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Weakness::GetTagEffectClass() const
{
    return UTacticalEffect_Weakness::StaticClass();
}
