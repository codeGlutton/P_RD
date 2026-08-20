/*****************************************************************//**
 * @file   TacticalEffect_Stun.cpp
 * @brief  Stun(기절) 이펙트 구현 파일
 * @author 이문환
 * @date   2026-08-20
 *********************************************************************/

#include "TAS/Effect/Tag/TacticalEffect_Stun.h"
#include "GameplayTagType.h"

UTacticalEffect_Stun::UTacticalEffect_Stun()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Stun;
}

UTacticalEffect_GetStun::UTacticalEffect_GetStun()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Stun;
}
