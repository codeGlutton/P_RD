/*****************************************************************//**
 * @file   TacticalEffect_Immunity.h
 * @brief  Immunity(면역) 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-08-23
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_ActorState.h"
#include "TacticalEffect_Immunity.generated.h"

/**
 * @brief  Immunity(면역) 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Immunity : public UTacticalEffect_ActorState
{
	GENERATED_BODY()

public:
	UTacticalEffect_Immunity();
};

/**
 * @brief  Immunity(면역) 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddImmunity : public UTacticalEffect_AddActorState
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddImmunity();
};

/**
 * @brief  Immunity(면역) 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetImmunity : public UTacticalEffect_GetActorState
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetImmunity();
};
