/*****************************************************************//**
 * @file   TacticalEffect_Cooldown.h
 * @brief  Cooldown 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-07-24
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_Cooldown.generated.h"

/**
 * @brief Cooldown 이펙트 베이스
 */
UCLASS(abstract)
class P_RD_API UTacticalEffect_Cooldown : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_Cooldown();
};

/**
 * @brief 라운드 기반 Cooldown 이펙트 베이스
 */
UCLASS()
class P_RD_API UTacticalEffect_RoundCooldown : public UTacticalEffect_Cooldown
{
	GENERATED_BODY()

public:
	UTacticalEffect_RoundCooldown();
};

/**
 * @brief 턴 기반 Cooldown 이펙트 베이스
 */
UCLASS()
class P_RD_API UTacticalEffect_TurnCooldown : public UTacticalEffect_Cooldown
{
	GENERATED_BODY()

public:
	UTacticalEffect_TurnCooldown();
};

