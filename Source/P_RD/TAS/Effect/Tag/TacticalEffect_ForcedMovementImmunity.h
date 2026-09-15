/*****************************************************************//**
 * @file   TacticalEffect_ForcedMovementImmunity.h
 * @brief  ForcedMovementImmunity(강제 이동 면역) 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-09-15
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_ForcedMovementImmunity.generated.h"

/**
 * @brief  ForcedMovementImmunity(강제 이동 면역) 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_ForcedMovementImmunity : public UTacticalEffect_DurationStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_ForcedMovementImmunity();
};

/**
 * @brief  ForcedMovementImmunity(강제 이동 면역) 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddForcedMovementImmunity : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddForcedMovementImmunity();
};

/**
 * @brief  ForcedMovementImmunity(강제 이동 면역) 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetForcedMovementImmunity : public UTacticalEffect_GetStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetForcedMovementImmunity();
};
