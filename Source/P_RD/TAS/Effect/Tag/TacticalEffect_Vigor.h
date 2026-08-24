/*****************************************************************//**
 * @file   TacticalEffect_Vigor.h
 * @brief  Vigor 이펙트 정의 헤더
 * @details AP(행동력)을 더 많이 얻는 상태이상 이펙트
 * @author 모호재
 * @date   2026-07-06
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_Vigor.generated.h"

/**
 * @brief  Vigor 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Vigor : public UTacticalEffect_DurationStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_Vigor();
};

/**
 * @brief  Vigor 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddVigor : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddVigor();
};

/**
 * @brief  Vigor 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetVigor : public UTacticalEffect_GetStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetVigor();
};
