/*****************************************************************//**
 * @file   BoardActorAnimInstance.h
 * @brief  보드 액터의 애님 인스턴스 정의 헤더
 * @author 모호재
 * @date   2026-06-29
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/BoardActorAnimType.h"
#include "BoardActorAnimInstance.generated.h"

UENUM(BlueprintType)
enum class ECombatTargetAliveState : uint8
{
	Alive,
	Dead,
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
	bool PlayMontageUsingTag(const FBoardActorAnimationContext& Context);

protected:
	bool PlayMontageUsingTag_Internal(FBoardActorAnimationContext Context);
	void OnEndMontageUsingTag(UAnimMontage* EndAnim, bool IsInterrupted);

public:
	bool TriggerMontageTagEvent(const FGameplayTag& EventTag, const FEventTriggerPayloadBase* Payload);

	bool RegisterTagEventOnMontage(const FGameplayTag& EventTag, FBoardActorAnimationEvent&& Event);
	bool UnregisterTagEventOnMontage(const FGameplayTag& EventTag);

	bool RegisterTagEventOnAllMontage(const FGameplayTag& EventTag, FBoardActorAllAnimationEvent&& Event);
	bool UnregisterTagEventOnAllMontage(const FGameplayTag& EventTag);

public:
	bool IsPlayingMontageUsingTag() const;
	UAnimMontage* GetPlayingMontageUsingTag() const;

	/* 애니메이션 등록 */
protected:
	UPROPERTY(Category = Animation, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "AnimMontageTags"))
	TMap<FGameplayTag, FTagMontageAnimationSet> mTagAnimMontageSets;

protected:
	UPROPERTY(Category = Animation, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "DefaultIdleAnim"))
	TObjectPtr<UAnimSequenceBase> mDefaultIdleAnim;

	/* 애님 실행 정보 */
protected:
	TMap<FGameplayTag, FBoardActorAllAnimationEvent> mAllMontageEvents;
	FBoardActorAnimationContext mActiveAnimationContext;

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
	UPROPERTY(Category = Animation, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "DefaultWalkAnim"))
	TObjectPtr<UBlendSpace> mDefaultWalkAnim;

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

