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
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Root);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Root);
}

UTacticalEffect_AddRoot::UTacticalEffect_AddRoot()
{
	mStatusEffect = UTacticalEffect_Root::StaticClass();
}

UTacticalEffect_GetRoot::UTacticalEffect_GetRoot()
{
	mStatusEffect = UTacticalEffect_Root::StaticClass();
}
