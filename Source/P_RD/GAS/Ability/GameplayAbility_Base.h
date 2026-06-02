// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GASMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbility_Base.generated.h"

/**
 * 
 */
UCLASS()
class P_RD_API UGameplayAbility_Base : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGameplayAbility_Base();
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	float		mDiceCost;

	TSubclassOf<UGameplayEffect>	mDiceCostClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	float		mDiceDots;

	TSubclassOf<UGameplayEffect>	mDiceDotsClass;

	bool mAbilityActive = false;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

};
