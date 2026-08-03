/*****************************************************************//**
 * @file   TacticalEffect_Weakness.h
 * @brief  Weakness 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-07-06
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_Weakness.generated.h"

 /**
  * @brief  Weakness 이펙트
  */
UCLASS()
class P_RD_API UTacticalEffect_Weakness : public UTacticalEffect_StatusTag
{
	GENERATED_BODY()

public:
	UTacticalEffect_Weakness();
};

/**
 * @brief  Weakness 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetWeakness : public UTacticalEffect_GetStatusTag
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetWeakness();
};
