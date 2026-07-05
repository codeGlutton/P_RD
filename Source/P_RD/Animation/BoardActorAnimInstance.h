/*****************************************************************//**
 * @file   BoardActorAnimInstance.h
 * @brief  보드 액터의 애님 인스턴스 정의 헤더
 * @author 모호재
 * @date   2026-06-29
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Animation/AnimInstance.h"
#include "BoardActorAnimInstance.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTriggerAnimationEvent, FGameplayTag /*Tag*/, UAnimMontage* /*EndAnim*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnTriggerEndAnimationEvent, FGameplayTag /*Tag*/, UAnimMontage* /*EndAnim*/, bool /*IsInterrupted*/);

/**
 * @brief 특정 시기에 호출할 애님 이벤트
 */
USTRUCT()
struct FBoardActorAnimationEvent
{
	GENERATED_BODY()

public:
	FOnTriggerAnimationEvent OnTriggerAnimationEvent;
	bool mIsOneTimeEvent = false;
};

/**
 * @brief  보드 액터의 애님 인스턴스
 */
UCLASS()
class P_RD_API UBoardActorAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	bool PlayMontageUsingTag(const FGameplayTag& MontageTag);
	bool PlayMontageUsingTag(const FGameplayTag& MontageTag, FOnTriggerEndAnimationEvent&& EndEvent);

protected:
	bool PlayMontageUsingTag_Internal(const FGameplayTag& MontageTag);
	void OnEndMontageUsingTag(UAnimMontage* EndAnim, bool IsInterrupted);

public:
	bool RegisterEventOnMontage(const FGameplayTag& EventTag, FBoardActorAnimationEvent&& Event);
	bool TriggerMontageEvent(const FGameplayTag& EventTag);
	bool UnregisterEventOnMontage(const FGameplayTag& EventTag);

protected:
	UPROPERTY(Category = Animation, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "AnimMontageTags"))
	TMap<FGameplayTag, TObjectPtr<UAnimMontage>> mAnimMontageTags;

	TMap<FGameplayTag, FBoardActorAnimationEvent> mActiveMontageEvents;
	FGameplayTag mActiveMontageTag = FGameplayTag::EmptyTag;

protected:
	UPROPERTY(Category = Animation, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "DefaultWalkAnim"))
	TObjectPtr<UBlendSpace> mDefaultWalkAnim;

	UPROPERTY(Category = Animation, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "DefaultIdleAnim"))
	TObjectPtr<UAnimSequenceBase> mDefaultIdleAnim;

protected:
	UPROPERTY(Category = Animation, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "DefaultAppearAnim"))
	TObjectPtr<UAnimSequenceBase> mDefaultAppearAnim;

	UPROPERTY(Category = Animation, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "DefaultDeathAnim"))
	TObjectPtr<UAnimSequenceBase> mDefaultDeathAnim;
};
