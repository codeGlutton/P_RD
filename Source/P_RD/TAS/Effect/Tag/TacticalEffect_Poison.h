/*****************************************************************//**
 * @file   TacticalEffect_Poison.h
 * @brief  Poison(중독) 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-08-23
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_Poison.generated.h"

/**
 * @brief  Poison(중독) 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Poison : public UTacticalEffect_DurationStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_Poison();

	/* UTacticalEffect_DurationStatus 상속 */
public:
	void OnReduceTimeRemaining(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const override;
};

/**
 * @brief  Poison(중독) 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddPoison : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddPoison();
};

/**
 * @brief  Poison(중독) 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetPoison : public UTacticalEffect_GetStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetPoison();
};
