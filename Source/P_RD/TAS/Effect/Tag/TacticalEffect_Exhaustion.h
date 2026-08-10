/*****************************************************************//**
 * @file   TacticalEffect_Exhaustion.h
 * @brief  Exhaustion 이펙트 정의 헤더
 * @details AP(행동력)을 더 적게 얻는 상태이상 이펙트
 * @author 김준형
 * @date   2026-08-10
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_Exhaustion.generated.h"

/**
 * @brief Exhaustion(탈진) 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Exhaustion : public UTacticalEffect_StatusTag
{
	GENERATED_BODY()
	
public:
	UTacticalEffect_Exhaustion();
};

/**
 * @brief Exhaustion(탈진) 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetExhaustion : public UTacticalEffect_GetStatusTag
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetExhaustion();
};
