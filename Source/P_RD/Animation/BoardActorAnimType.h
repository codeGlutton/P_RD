/*****************************************************************//**
 * @file   BoardActorAnimType.h
 * @brief  보드 액터의 애님 타입 정의 헤더
 * @author 모호재
 * @date   2026-07-30
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "BoardActorAnimType.generated.h"

struct FEventTriggerPayloadBase;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnTriggerAnimationEvent, const FBoardActorAnimationContext& /*Context*/, UAnimMontage* /*EndAnim*/, const FEventTriggerPayloadBase* /*Payload*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnTriggerEndAnimationEvent, const FBoardActorAnimationContext& /*Context*/, UAnimMontage* /*EndAnim*/, bool /*IsInterrupted*/);

/**
 * @brief 방향 별 애님 몽타쥬 묶음
 */
USTRUCT(BlueprintType)
struct FTagMontageAnimationSet
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Montage", EditAnywhere, meta = (DisplayName = "WorldWidgetClasses", ArraySizeEnum = "ETileActorDirection"))
	TObjectPtr<UAnimMontage> mAnimMontages[static_cast<uint8>(ETileActorDirection::Count)];

	UPROPERTY(Category = "Montage", EditAnywhere, meta = (DisplayName = "IsAdditive"))
	bool mIsAdditive = false;
};

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
 * @brief 모든 시기에 호출할 애님 이벤트
 */
USTRUCT()
struct FBoardActorAllAnimationEvent
{
	GENERATED_BODY()

public:
	FOnTriggerAnimationEvent OnTriggerAnimationEvent;
};

/**
 * @brief 보드 액터 애니메이션 재생과 함께 전달되는 메타 데이터
 */
USTRUCT()
struct FBoardActorAnimationMetaData
{
	GENERATED_BODY()
};

/**
 * @brief 보드 액터 애니메이션 컨텍스트
 */
USTRUCT()
struct FBoardActorAnimationContext
{
	GENERATED_BODY()

public:
	FBoardActorAnimationContext() = default;
	FBoardActorAnimationContext(FGameplayTag MontageTag, ETileActorDirection MontageDir);

public:
	bool IsValid() const;
	void Clear();

public:
	TInstancedStruct<FBoardActorAnimationMetaData> mMetaData;

public:
	TMap<FGameplayTag, FBoardActorAnimationEvent> mMontageEvents;
	FOnTriggerEndAnimationEvent mMontageEndEvent;

public:
	FGameplayTag mMontageTag = FGameplayTag::EmptyTag;
	ETileActorDirection mMontageDir = ETileActorDirection::Count;
};

