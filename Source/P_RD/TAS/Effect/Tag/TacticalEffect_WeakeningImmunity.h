/*****************************************************************//**
 * @file   TacticalEffect_WeakeningImmunity.h
 * @brief  WeakeningImmunity(쇠약 면역) 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-09-15
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_WeakeningImmunity.generated.h"

/**
 * @brief  WeakeningImmunity(쇠약 면역) 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_WeakeningImmunity : public UTacticalEffect_DurationStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_WeakeningImmunity();
};

/**
 * @brief  WeakeningImmunity(쇠약 면역) 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddWeakeningImmunity : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddWeakeningImmunity();
};

/**
 * @brief  WeakeningImmunity(쇠약 면역) 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetWeakeningImmunity : public UTacticalEffect_GetStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetWeakeningImmunity();
};
