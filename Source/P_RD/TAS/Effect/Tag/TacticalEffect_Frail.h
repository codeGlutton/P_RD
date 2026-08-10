/*****************************************************************//**
 * @file   TacticalEffect_Frail.h
 * @brief  Frail 이펙트 정의 헤더
 * @details 방어도를 더 적게 얻는 상태이상 이펙트
 * @author 김준형
 * @date   2026-08-10
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_Frail.generated.h"

/**
 * @brief  Frail 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Frail : public UTacticalEffect_StatusTag
{
	GENERATED_BODY()

public:
	UTacticalEffect_Frail();
};

/**
 * @brief  Frail 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetFrail : public UTacticalEffect_GetStatusTag
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetFrail();
};
