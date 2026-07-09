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
		mUseAdditiveMontage = mTagAnimMontageSets[mActiveMontageTag].mIsAdditive;
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
	if (PlayMontageUsingTag_Internal(MontageTag, LocalDirection) == false)
	{
		return false;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UBoardActorAnimInstance::OnEndMontageUsingTag);
	Montage_SetEndDelegate(EndDelegate, GetPlayingMontageUsingTag());
	return true;
}

bool UBoardActorAnimInstance::PlayMontageUsingTag(const FGameplayTag& MontageTag, ETileActorDirection LocalDirection, FOnTriggerEndAnimationEvent&& EndEvent)
{
	if (PlayMontageUsingTag_Internal(MontageTag, LocalDirection) == false)
	{
		return false;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindWeakLambda(this, [this, Callback = MoveTemp(EndEvent)](UAnimMontage* EndAnim, bool IsInterrupted) {
		Callback.Broadcast(mActiveMontageTag, mActiveMontageDir, EndAnim, IsInterrupted);
		OnEndMontageUsingTag(EndAnim, IsInterrupted);
		});
	Montage_SetEndDelegate(EndDelegate, GetPlayingMontageUsingTag());
	return true;
}

bool UBoardActorAnimInstance::PlayMontageUsingTag_Internal(const FGameplayTag& MontageTag, ETileActorDirection LocalDirection)
{
	if (mTagAnimMontageSets.Contains(MontageTag) == false)
	{
		// 존재하지 않는 애님 몽타쥬
		return false;
	}

	TObjectPtr<UAnimMontage>& TargetAnimMontage = mTagAnimMontageSets[MontageTag].mAnimMontages[StaticCast<int32>(LocalDirection)];
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

	mActiveMontageTag = MontageTag;
	mActiveMontageDir = LocalDirection;
	return true;
}

void UBoardActorAnimInstance::OnEndMontageUsingTag(UAnimMontage* EndAnim, bool IsInterrupted)
{
	mActiveMontageTag = FGameplayTag::EmptyTag;
	mActiveMontageDir = ETileActorDirection::Count;
	mActiveMontageEvents.Reset();
}

bool UBoardActorAnimInstance::RegisterTagEventOnMontage(const FGameplayTag& EventTag, FBoardActorAnimationEvent&& Event)
{
	if (IsPlayingMontageUsingTag() == false)
	{
		// 태그 기반 몽타쥬 실행 중이 아님
		return false;
	}

	// 이벤트 덮어씌우기
	mActiveMontageEvents.FindOrAdd(EventTag) = MoveTemp(Event);
	return true;
}

bool UBoardActorAnimInstance::TriggerMontageTagEvent(const FGameplayTag& EventTag, const FEventTriggerPayload* Payload)
{
	if (IsPlayingMontageUsingTag() == false)
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
		TargetEvent.OnTriggerAnimationEvent.Broadcast(mActiveMontageTag, mActiveMontageDir, GetPlayingMontageUsingTag(), Payload);
	}

	if (TargetEvent.mIsOneTimeEvent == true)
	{
		UnregisterTagEventOnMontage(EventTag);
	}

	return true;
}

bool UBoardActorAnimInstance::UnregisterTagEventOnMontage(const FGameplayTag& EventTag)
{
	if (IsPlayingMontageUsingTag() == false)
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

bool UBoardActorAnimInstance::IsPlayingMontageUsingTag() const
{
	return mActiveMontageTag.IsValid() == true && mActiveMontageDir != ETileActorDirection::Count;
}

UAnimMontage* UBoardActorAnimInstance::GetPlayingMontageUsingTag() const
{
	return mTagAnimMontageSets[mActiveMontageTag].mAnimMontages[StaticCast<int32>(mActiveMontageDir)];
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


