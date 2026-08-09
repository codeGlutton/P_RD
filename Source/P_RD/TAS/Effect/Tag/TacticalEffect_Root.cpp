/*****************************************************************//**
 * @file   TacticalEffect_Root.cpp
 * @brief  Root(속박) 이펙트 구현 파일
 * @author 이문환
 * @date   2026-08-10
 *********************************************************************/

#include "TAS/Effect/Tag/TacticalEffect_Root.h"
#include "GameplayTagType.h"

UTacticalEffect_Root::UTacticalEffect_Root()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Root;
}

UTacticalEffect_GetRoot::UTacticalEffect_GetRoot()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Root;
}
