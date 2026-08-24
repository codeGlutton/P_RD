/*****************************************************************//**
 * @file   TacticalEffect_Root.h
 * @brief  Root(속박) 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-08-10
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_Root.generated.h"

 /**
  * @brief  Root(속박) 이펙트
  */
UCLASS()
class P_RD_API UTacticalEffect_Root : public UTacticalEffect_DurationStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_Root();
};

/**
 * @brief  Root(속박) 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddRoot : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddRoot();
};

/**
 * @brief  Root(속박) 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetRoot : public UTacticalEffect_GetStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetRoot();
};
