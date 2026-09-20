/*****************************************************************//**
 * @file   SkillEffectLayer_Root.cpp
 * @brief  하나의 스킬 모션 내에서 적용하는 속박 디버프 효과 단위 구현 파일
 * @author 이문환
 * @date   2026-08-10
 *********************************************************************/

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Root.h"
#include "TAS/Effect/Tag/TacticalEffect_Root.h"

TSubclassOf<UTacticalEffect> FSkillEffectLayer_Root::GetTagEffectClass() const
{
    return UTacticalEffect_GetRoot::StaticClass();
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_Root"

FText FSkillEffectLayer_Root::GetTagDisplayName() const
{
	return LOCTEXT("RootName", "속박");
}

#undef LOCTEXT_NAMESPACE
