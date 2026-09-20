/*****************************************************************//**
 * @file   TacticalEffect_ActorState.h
 * @brief  액터 상태 GameplayTag 이펙트 부모 클래스 정의 헤더
 * @author 모호재
 * @date   2026-08-23
 *********************************************************************/

#pragma once

#include "GameplayTagContainer.h"
#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_ActorState.generated.h"

/**
 * @brief 액터 상태 자체를 의미하는 이펙트 부모 클래스 (항상 영구적)
 */
UCLASS(Abstract)
class P_RD_API UTacticalEffect_ActorState : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_ActorState();
};

/**
 * @brief 액터 상태 GameplayTag 변경 이펙트 부모 클래스
 */
UCLASS(Abstract)
class P_RD_API UTacticalEffect_AddActorState : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddActorState();

	/* UTacticalEffect 상속 */
public:
	virtual void OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const override;

protected:
	/** @brief 부여할 액터 상태 Effect */
	UPROPERTY(Category = "TacticalEffect", EditDefaultsOnly)
	TSubclassOf<UTacticalEffect_ActorState> mActorStateEffect;
};

/**
 * @brief 액터 상태 GameplayTag 부여 이펙트 부모 클래스
 */
UCLASS(Abstract)
class P_RD_API UTacticalEffect_GetActorState : public UTacticalEffect_AddActorState
{
	GENERATED_BODY()

	/* UTacticalEffect_AddActorState 상속 */
public:
	virtual bool CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const override;
};
