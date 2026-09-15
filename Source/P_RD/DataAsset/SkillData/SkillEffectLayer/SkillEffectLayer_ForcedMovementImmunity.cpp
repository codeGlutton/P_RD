#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_ForcedMovementImmunity.h"
#include "TAS/Effect/Tag/TacticalEffect_ForcedMovementImmunity.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_ForcedMovementImmunity::GetTagEffectClass() const
{
	return UTacticalEffect_GetForcedMovementImmunity::StaticClass();
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_ForcedMovementImmunity"

FText FSkillEffectLayer_ForcedMovementImmunity::GetTagDisplayName() const
{
	return LOCTEXT("ForcedMovementImmunityName", "강제 이동 면역");
}

#undef LOCTEXT_NAMESPACE
