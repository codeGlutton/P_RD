#include "Animation/BoardActorAnimInstance.h"
#include "Animation/Notify/EventTriggerPayload.h"

#include "ObjectView.h"
#include "ObjectModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

void UBoardActorAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (IsPlayingMontageUsingTag() == true)
	{
		mUseAdditiveMontage = mTagAnimMontageSets[mActiveAnimationContext.mMontageTag].mIsAdditive;
	}
	else
	{
		mUseAdditiveMontage = false;
	}

	APawn* OwningPawn = Cast<APawn>(GetOwningActor());
	if (OwningPawn != nullptr)
	{
		const FVector ForwardNorDir = OwningPawn->GetActorForwardVector();
		const FVector RightNorDir = OwningPawn->GetActorRightVector();
		const FVector Velocity = OwningPawn->GetVelocity();

		mPawnVelocity.X = FVector::DotProduct(Velocity, ForwardNorDir);
		mPawnVelocity.Y = FVector::DotProduct(Velocity, RightNorDir);
	}
}

bool UBoardActorAnimInstance::PlayMontageUsingTag(const FGameplayTag& MontageTag, ETileActorDirection LocalDirection)
{
	return PlayMontageUsingTag_Internal(FBoardActorAnimationContext(MontageTag, LocalDirection));
}

bool UBoardActorAnimInstance::PlayMontageUsingTag(const FBoardActorAnimationContext& Context)
{
	return PlayMontageUsingTag_Internal(Context);
}

bool UBoardActorAnimInstance::PlayMontageUsingTag_Internal(FBoardActorAnimationContext Context)
{
	if (mTagAnimMontageSets.Contains(Context.mMontageTag) == false)
	{
		// 존재하지 않는 애님 몽타쥬
		return false;
	}

	TObjectPtr<UAnimMontage>& TargetAnimMontage = mTagAnimMontageSets[Context.mMontageTag].mAnimMontages[StaticCast<int32>(Context.mMontageDir)];
	if (TargetAnimMontage == nullptr)
	{
		// nullptr 애님 몽타쥬
		return false;
	}

	if (Montage_Play(TargetAnimMontage) <= 0.f)
	{
		// 애님 몽타쥬 실행 시간이 0초 이하
		return false;
	}

	/* 성공적으로 몽타쥬 실행 시 */

	mActiveAnimationContext = MoveTemp(Context);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UBoardActorAnimInstance::OnEndMontageUsingTag);
	Montage_SetEndDelegate(EndDelegate, GetPlayingMontageUsingTag());

	return true;
}

void UBoardActorAnimInstance::OnEndMontageUsingTag(UAnimMontage* EndAnim, bool IsInterrupted)
{
	mActiveAnimationContext.mMontageEndEvent.Broadcast(mActiveAnimationContext, EndAnim, IsInterrupted);
	mActiveAnimationContext.Clear();
}

bool UBoardActorAnimInstance::TriggerMontageTagEvent(const FGameplayTag& EventTag, const FEventTriggerPayload* Payload)
{
	if (IsPlayingMontageUsingTag() == false)
	{
		// 태그 기반 몽타쥬 실행 중이 아님
		return false;
	}

	const bool HasMontageEvent = mActiveAnimationContext.mMontageEvents.Contains(EventTag);
	const bool HasAllMontageEvent = mAllMontageEvents.Contains(EventTag);
	if (HasMontageEvent == false || HasAllMontageEvent == false)
	{
		// 존재하지 않는 이벤트
		return false;
	}

	if (HasMontageEvent == false)
	{
		/* 현 몽타쥬 대상 이벤트 실행 */

		FBoardActorAnimationEvent& TargetEvent = mActiveAnimationContext.mMontageEvents[EventTag];
		if (TargetEvent.OnTriggerAnimationEvent.IsBound() == true)
		{
			TargetEvent.OnTriggerAnimationEvent.Broadcast(mActiveAnimationContext, GetPlayingMontageUsingTag(), Payload);
		}

		if (TargetEvent.mIsOneTimeEvent == true)
		{
			UnregisterTagEventOnMontage(EventTag);
		}
	}
	if (HasAllMontageEvent == false)
	{
		/* 모든 몽타쥬 대상 이벤트 실행 */

		if (mAllMontageEvents[EventTag].OnTriggerAnimationEvent.IsBound() == true)
		{
			mAllMontageEvents[EventTag].OnTriggerAnimationEvent.Broadcast(mActiveAnimationContext, GetPlayingMontageUsingTag(), Payload);
		}
	}

	return true;
}

bool UBoardActorAnimInstance::RegisterTagEventOnMontage(const FGameplayTag& EventTag, FBoardActorAnimationEvent&& Event)
{
	if (IsPlayingMontageUsingTag() == false)
	{
		// 태그 기반 몽타쥬 실행 중이 아님
		return false;
	}

	// 이벤트 덮어씌우기
	mActiveAnimationContext.mMontageEvents.FindOrAdd(EventTag) = MoveTemp(Event);
	return true;
}

bool UBoardActorAnimInstance::UnregisterTagEventOnMontage(const FGameplayTag& EventTag)
{
	if (IsPlayingMontageUsingTag() == false)
	{
		// 태그 기반 몽타쥬 실행 중이 아님
		return false;
	}

	if (mActiveAnimationContext.mMontageEvents.Contains(EventTag) == false)
	{
		// 존재하지 않는 이벤트
		return false;
	}

	mActiveAnimationContext.mMontageEvents.Remove(EventTag);
	return true;
}

bool UBoardActorAnimInstance::RegisterTagEventOnAllMontage(const FGameplayTag& EventTag, FBoardActorAllAnimationEvent&& Event)
{
	// 이벤트 덮어씌우기
	mAllMontageEvents.FindOrAdd(EventTag) = MoveTemp(Event);
	return true;
}

bool UBoardActorAnimInstance::UnregisterTagEventOnAllMontage(const FGameplayTag& EventTag)
{
	if (mAllMontageEvents.Contains(EventTag) == false)
	{
		// 존재하지 않는 이벤트
		return false;
	}

	mAllMontageEvents.Remove(EventTag);
	return true;
}

bool UBoardActorAnimInstance::IsPlayingMontageUsingTag() const
{
	return mActiveAnimationContext.IsValid() == true;
}

UAnimMontage* UBoardActorAnimInstance::GetPlayingMontageUsingTag() const
{
	return mTagAnimMontageSets[mActiveAnimationContext.mMontageTag].mAnimMontages[StaticCast<int32>(mActiveAnimationContext.mMontageDir)];
}

void UCombatTargetAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	IObjectView* ObjectView = Cast<IObjectView>(GetOwningActor());
	if (ObjectView != nullptr)
	{
		UObjectModel* ObjectModel = ObjectView->GetModel();
		if (ObjectModel != nullptr)
		{
			IBoardCombatTarget* OwningCombatTarget = Cast<IBoardCombatTarget>(ObjectModel);
			if (OwningCombatTarget != nullptr)
			{
				mCombatTargetAliveState = OwningCombatTarget->IsDead() == true ? ECombatTargetAliveState::Dead : ECombatTargetAliveState::Alive;
			}
		}
	}
}

void UUnitAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
}


