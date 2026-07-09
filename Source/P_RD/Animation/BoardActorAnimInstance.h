/*****************************************************************//**
 * @file   BoardActorAnimInstance.h
 * @brief  보드 액터의 애님 인스턴스 정의 헤더
 * @author 모호재
 * @date   2026-06-29
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Animation/AnimInstance.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "BoardActorAnimInstance.generated.h"

struct FEventTriggerPayload;

DECLARE_MULTICAST_DELEGATE_FourParams(FOnTriggerAnimationEvent, FGameplayTag /*Tag*/, ETileActorDirection /*LocalDir*/, UAnimMontage* /*EndAnim*/, const FEventTriggerPayload* /*Payload*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnTriggerEndAnimationEvent, FGameplayTag /*Tag*/, ETileActorDirection /*LocalDir*/, UAnimMontage* /*EndAnim*/, bool /*IsInterrupted*/);

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
 * @brief  보드 액터의 애님 인스턴스
 */
UCLASS()
class P_RD_API UBoardActorAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	/* UAnimInstance 상속 */
public:
	void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	/**
	 * @brief 특정 태그의 애니메이션을 실행
	 * @param MontageTag 애님 몽타쥬 태그
	 * @return 
	 */
	bool PlayMontageUsingTag(const FGameplayTag& MontageTag, ETileActorDirection LocalDirection);
	bool PlayMontageUsingTag(const FGameplayTag& MontageTag, ETileActorDirection LocalDirection, FOnTriggerEndAnimationEvent&& EndEvent);

protected:
	bool PlayMontageUsingTag_Internal(const FGameplayTag& MontageTag, ETileActorDirection LocalDirection);
	void OnEndMontageUsingTag(UAnimMontage* EndAnim, bool IsInterrupted);

public:
	bool RegisterTagEventOnMontage(const FGameplayTag& EventTag, FBoardActorAnimationEvent&& Event);
	bool TriggerMontageTagEvent(const FGameplayTag& EventTag, const FEventTriggerPayload* Payload);
	bool UnregisterTagEventOnMontage(const FGameplayTag& EventTag);

public:
	bool IsPlayingMontageUsingTag() const;
	UAnimMontage* GetPlayingMontageUsingTag() const;

	/* 애니메이션 등록 */
protected:
	UPROPERTY(Category = Animation, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "AnimMontageTags"))
	TMap<FGameplayTag, FTagMontageAnimationSet> mTagAnimMontageSets;

	TMap<FGameplayTag, FBoardActorAnimationEvent> mActiveMontageEvents;
	FGameplayTag mActiveMontageTag = FGameplayTag::EmptyTag;
	ETileActorDirection mActiveMontageDir = ETileActorDirection::Count;

protected:
	UPROPERTY(Category = Animation, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "DefaultWalkAnim"))
	TObjectPtr<UBlendSpace> mDefaultWalkAnim;

	UPROPERTY(Category = Animation, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "DefaultIdleAnim"))
	TObjectPtr<UAnimSequenceBase> mDefaultIdleAnim;

	/* 계산 값 */
protected:
	UPROPERTY(Category = Data, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "UseAdditiveMontage"))
	bool mUseAdditiveMontage = false;

protected:
	UPROPERTY(Category = Data, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "PawnVelocity"))
	FVector2D mPawnVelocity = FVector2D::ZeroVector;
	UPROPERTY(Category = Data, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "MinWalkSquareVelocity"))
	float mMinWalkSquareVelocity = 10.f;
};

UENUM(BlueprintType)
enum class ECombatTargetAliveState : uint8
{
	Alive,
	Dead,
};

/**
 * @brief  전투 대상의 애님 인스턴스
 */
UCLASS()
class P_RD_API UCombatTargetAnimInstance : public UBoardActorAnimInstance
{
	GENERATED_BODY()

	/* UBoardActorAnimInstance 상속 */
public:
	void NativeUpdateAnimation(float DeltaSeconds) override;

	/* 애니메이션 등록 */
protected:
	UPROPERTY(Category = Animation, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "DefaultAppearAnim"))
	TObjectPtr<UAnimSequenceBase> mDefaultAppearAnim;

	UPROPERTY(Category = Animation, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "DefaultDeathAnim"))
	TObjectPtr<UAnimSequenceBase> mDefaultDeathAnim;

	/* 계산 값 */
protected:
	UPROPERTY(Category = Data, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatTargetAliveState"))
	ECombatTargetAliveState mCombatTargetAliveState = ECombatTargetAliveState::Alive;
};

/**
 * @brief  유닛의 애님 인스턴스
 */
UCLASS()
class P_RD_API UUnitAnimInstance : public UCombatTargetAnimInstance
{
	GENERATED_BODY()

	/* UCombatTargetAnimInstance 상속 */
public:
	void NativeUpdateAnimation(float DeltaSeconds) override;
};

