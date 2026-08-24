/*****************************************************************//**
 * @file   TacticalEffect_StatusTag.h
 * @brief  상태이상 GameplayTag 이펙트 부모 클래스 정의 헤더
 * @author 모호재
 * @date   2026-08-02
 *********************************************************************/

#pragma once

#include "GameplayTagContainer.h"
#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_StatusTag.generated.h"

/**
 * @brief 상태이상 자체를 의미하는 이펙트 부모 클래스
 */
UCLASS(Abstract)
class P_RD_API UTacticalEffect_Status : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_Status();
};

/**
 * @brief 영구적 상태이상 자체를 의미하는 이펙트 부모 클래스
 */
UCLASS(Abstract)
class P_RD_API UTacticalEffect_InfiniteStatus : public UTacticalEffect_Status
{
	GENERATED_BODY()

public:
	UTacticalEffect_InfiniteStatus();
};

/**
 * @brief 일정 턴 수를 가진 상태이상 자체를 의미하는 이펙트 부모 클래스
 */
UCLASS(Abstract)
class P_RD_API UTacticalEffect_DurationStatus : public UTacticalEffect_Status
{
	GENERATED_BODY()

public:
	UTacticalEffect_DurationStatus();
};

/**
 * @brief 상태이상 GameplayTag 변경 이펙트 부모 클래스
 */
UCLASS(Abstract)
class P_RD_API UTacticalEffect_AddStatus : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddStatus();

	/* UTacticalEffect 상속 */
public:
	void OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const override;

protected:
	/** @brief 부여할 상태이상 Effect */
	UPROPERTY(Category = "TacticalEffect", EditDefaultsOnly)
	TSubclassOf<UTacticalEffect_Status> mStatusEffect;
};

/**
 * @brief 상태이상 GameplayTag 부여 이펙트 부모 클래스
 */
UCLASS(Abstract)
class P_RD_API UTacticalEffect_GetStatus : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

	/* UTacticalEffect_AddStatus 상속 */
public:
	bool CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const override;
};
