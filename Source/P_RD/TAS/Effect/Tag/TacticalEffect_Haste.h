/*****************************************************************//**
 * @file   TacticalEffect_Haste.h
 * @brief  Haste 이펙트 정의 헤더
 * @details SP(스피드 포인트)를 더 많이 얻는 버프 이펙트
 * @author 김준형
 * @date   2026-08-10
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_Haste.generated.h"

 /**
  * @brief Haste(신속) 이펙트
  */
UCLASS()
class P_RD_API UTacticalEffect_Haste : public UTacticalEffect_DurationStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_Haste();
};

/**
 * @brief Haste(신속) 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddHaste : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddHaste();
};

/**
 * @brief Haste(신속) 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetHaste : public UTacticalEffect_GetStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetHaste();
};
