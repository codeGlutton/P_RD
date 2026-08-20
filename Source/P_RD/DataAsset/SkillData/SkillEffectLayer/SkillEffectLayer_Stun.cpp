/*****************************************************************//**
 * @file   SkillEffectLayer_Stun.cpp
 * @brief  하나의 스킬 모션 내에서 적용하는 기절 디버프 효과 단위 구현 파일
 * @author 이문환
 * @date   2026-08-20
 *********************************************************************/

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
