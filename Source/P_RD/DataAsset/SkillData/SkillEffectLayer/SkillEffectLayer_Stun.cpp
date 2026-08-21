#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Stun.h"
#include "TAS/Effect/Tag/TacticalEffect_Stun.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Stun::GetTagEffectClass() const
{
	return UTacticalEffect_GetStun::StaticClass();
}

#if WITH_EDITOR
#define LOCTEXT_NAMESPACE "SkillEffectLayer_Stun"

FText FSkillEffectLayer_Stun::GetTagDisplayName() const
{
	return LOCTEXT("StunName", "기절");
}

#undef LOCTEXT_NAMESPACE
#endif
