/*****************************************************************//**
 * @file   GameplayAbility_SkillBase.h
 * @brief  스킬 어빌리티 기본 베이스
 * @author 김준형
 * @date   2026-06-01
 *********************************************************************/

#pragma once

#include "../../GASMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbility_SkillBase.generated.h"

/**
 * 
 */
UCLASS()
class P_RD_API UGameplayAbility_SkillBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGameplayAbility_SkillBase();
	
protected:
	/**
	* @brief 주사위 소모값
	* @details
	* 
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	float		mDiceCost;

	TSubclassOf<UGameplayEffect>	mDiceCostClass;

	/**
	* @brief 주사위 눈금
	* @details
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	float		mDiceDots;

	TSubclassOf<UGameplayEffect>	mDiceDotsClass;

	/**
	* @brief 어빌리티 사용하고 있는지 여부
	* @details
	* 현재 뚜렷한 사용처는 없음
	*/
	bool mAbilityActive = false;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;


	bool CommitActorToEffect(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		class AUnit* Unit, const struct FUnitCommitResult& CommitResult);
};
