/*****************************************************************//**
 * @file   TacticalEffect_Dead.h
 * @brief  Dead 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-07-09
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_ActorState.h"
#include "TacticalEffect_Dead.generated.h"

/**
 * @brief  Dead 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Dead : public UTacticalEffect_ActorState
{
	GENERATED_BODY()

public:
	UTacticalEffect_Dead();
};

/**
 * @brief  Dead 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddDead : public UTacticalEffect_AddActorState
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddDead();
};

/**
 * @brief  Dead 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetDead : public UTacticalEffect_GetActorState
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetDead();
};
