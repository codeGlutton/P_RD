/*****************************************************************//**
 * @file   TacticalEffect_Fortification.h
 * @brief  Fortification 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-07-06
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_Fortification.generated.h"

/**
 * @brief  Fortification 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Fortification : public UTacticalEffect_StatusTag
{
	GENERATED_BODY()

public:
	UTacticalEffect_Fortification();
};

/**
 * @brief  Fortification 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetFortification : public UTacticalEffect_GetStatusTag
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetFortification();
};
