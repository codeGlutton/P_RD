/*****************************************************************//**
 * @file   TacticalEffect_Slow.h
 * @brief  Slow 이펙트 정의 헤더
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
class P_RD_API UTacticalEffect_Slow : public UTacticalEffect_StatusTag
{
	GENERATED_BODY()

public:
	UTacticalEffect_Slow();
};

/**
 * @brief Slow(둔화) 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetSlow : public UTacticalEffect_StatusTag
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetSlow();
};
