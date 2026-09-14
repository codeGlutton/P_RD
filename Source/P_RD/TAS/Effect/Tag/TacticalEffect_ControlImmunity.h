/*****************************************************************//**
 * @file   TacticalEffect_ControlImmunity.h
 * @brief  ControlImmunity(억제 면역) 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-09-15
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_ControlImmunity.generated.h"

/**
 * @brief  ControlImmunity(억제 면역) 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_ControlImmunity : public UTacticalEffect_InfiniteStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_ControlImmunity();
};

/**
 * @brief  ControlImmunity(억제 면역) 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddControlImmunity : public UTacticalEffect_AddUniqueStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddControlImmunity();
};

/**
 * @brief  ControlImmunity(억제 면역) 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetControlImmunity : public UTacticalEffect_GetUniqueStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetControlImmunity();
};
