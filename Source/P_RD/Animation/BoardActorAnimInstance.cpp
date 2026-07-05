#include "Animation/BoardActorAnimInstance.h"

bool UBoardActorAnimInstance::PlayMontageUsingTag(const FGameplayTag& MontageTag)
{
	if (PlayMontageUsingTag_Internal(MontageTag) == false)
	{
		return false;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UBoardActorAnimInstance::OnEndMontageUsingTag);
	Montage_SetEndDelegate(EndDelegate, mAnimMontageTags[mActiveMontageTag]);
	return true;
}

bool UBoardActorAnimInstance::PlayMontageUsingTag(const FGameplayTag& MontageTag, FOnTriggerEndAnimationEvent&& EndEvent)
{
	if (PlayMontageUsingTag_Internal(MontageTag) == false)
	{
		return false;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindWeakLambda(this, [this, Callback = MoveTemp(EndEvent)](UAnimMontage* EndAnim, bool IsInterrupted) {
		Callback.Broadcast(mActiveMontageTag, EndAnim, IsInterrupted);
		OnEndMontageUsingTag(EndAnim, IsInterrupted);
		});
	Montage_SetEndDelegate(EndDelegate, mAnimMontageTags[mActiveMontageTag]);
	return true;
}

bool UBoardActorAnimInstance::PlayMontageUsingTag_Internal(const FGameplayTag& MontageTag)
{
	if (mAnimMontageTags.Contains(MontageTag) == false)
	{
		// 존재하지 않는 애님 몽타쥬
		return false;
	}

	TObjectPtr<UAnimMontage>& TargetAnimMontage = mAnimMontageTags[MontageTag];
	if (TargetAnimMontage == nullptr)
	{
		// nullptr 애님 몽타쥬
		return false;
	}

	if (Montage_Play(mAnimMontageTags[MontageTag]) <= 0.f)
	{
		// 애님 몽타쥬 실행 시간이 0초 이하
		return false;
	}

	mActiveMontageTag = MontageTag;
	return true;
}

void UBoardActorAnimInstance::OnEndMontageUsingTag(UAnimMontage* EndAnim, bool IsInterrupted)
{
	mActiveMontageTag = FGameplayTag::EmptyTag;
	mActiveMontageEvents.Reset();
}

bool UBoardActorAnimInstance::RegisterEventOnMontage(const FGameplayTag& EventTag, FBoardActorAnimationEvent&& Event)
{
	if (mActiveMontageTag.IsValid() == false)
	{
		// 태그 기반 몽타쥬 실행 중이 아님
		return false;
	}

	// 이벤트 덮어씌우기
	mActiveMontageEvents.FindOrAdd(EventTag) = MoveTemp(Event);
	return true;
}

bool UBoardActorAnimInstance::TriggerMontageEvent(const FGameplayTag& EventTag)
{
	if (mActiveMontageTag.IsValid() == false)
	{
		// 태그 기반 몽타쥬 실행 중이 아님
		return false;
	}

	if (mActiveMontageEvents.Contains(EventTag) == false)
	{
		// 존재하지 않는 이벤트
		return false;
	}

	FBoardActorAnimationEvent& TargetEvent = mActiveMontageEvents[EventTag];
	if (TargetEvent.OnTriggerAnimationEvent.IsBound() == true)
	{
		TargetEvent.OnTriggerAnimationEvent.Broadcast(mActiveMontageTag, mAnimMontageTags[mActiveMontageTag]);
	}

	if (TargetEvent.mIsOneTimeEvent == true)
	{
		UnregisterEventOnMontage(EventTag);
	}

	return true;
}

bool UBoardActorAnimInstance::UnregisterEventOnMontage(const FGameplayTag& EventTag)
{
	if (mActiveMontageTag.IsValid() == false)
	{
		// 태그 기반 몽타쥬 실행 중이 아님
		return false;
	}

	if (mActiveMontageEvents.Contains(EventTag) == false)
	{
		// 존재하지 않는 이벤트
		return false;
	}

	mActiveMontageEvents.Remove(EventTag);
	return true;
}



