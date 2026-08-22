/*****************************************************************//**
 * @file   TacticalEffect_Stun.h
 * @brief  Stun(기절) 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-08-21
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_Stun.generated.h"

/**
 * @brief  Stun(기절) 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Stun : public UTacticalEffect_DurationStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_Stun();
};

/**
 * @brief  Stun(기절) 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddStun : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddStun();
};

/**
 * @brief  Stun(기절) 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetStun : public UTacticalEffect_GetStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetStun();
};
