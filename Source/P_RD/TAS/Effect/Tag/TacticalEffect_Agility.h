/*****************************************************************//**
 * @file   TacticalEffect_Agility.h
 * @brief  Agility 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-07-06
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_Agility.generated.h"

/**
 * @brief  Agility 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Agility : public UTacticalEffect_StatusTag
{
	GENERATED_BODY()

public:
	UTacticalEffect_Agility();
};

/**
 * @brief  Agility 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetAgility : public UTacticalEffect_GetStatusTag
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetAgility();
};
