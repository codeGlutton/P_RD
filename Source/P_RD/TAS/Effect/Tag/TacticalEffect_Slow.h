/*****************************************************************//**
 * @file   TacticalEffect_Slow.h
 * @brief  Slow 이펙트 정의 헤더
 * @details SP(스피드 포인트)을 더 적게 얻는 상태이상 이펙트
 * @author 김준형
 * @date   2026-08-10
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_Slow.generated.h"

/**
 * @brief Slow(둔화) 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Slow : public UTacticalEffect_DurationStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_Slow();
};

/**
 * @brief Slow(둔화) 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddSlow : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddSlow();
};

/**
 * @brief Slow(둔화) 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetSlow : public UTacticalEffect_GetStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetSlow();
};
