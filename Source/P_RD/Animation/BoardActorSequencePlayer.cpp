#include "Animation/BoardActorSequencePlayer.h"
#include "Animation/Notify/EventTriggerPayload.h"

#include "LevelSequence.h"
#include "LevelSequencePlayer.h"

#include "DefaultLevelSequenceInstanceData.h"

UBoardActorSequencePlayer::UBoardActorSequencePlayer()
{
	mBindingOverrides = CreateDefaultSubobject<UMovieSceneBindingOverrides>(TEXT("BindingOverrides"));
	mDefaultInstanceData = CreateDefaultSubobject<UDefaultLevelSequenceInstanceData>(TEXT("InstanceData"));

	mLevelSequencePlayer = CreateDefaultSubobject<ULevelSequencePlayer>(TEXT("AnimationPlayer"));
}

UWorld* UBoardActorSequencePlayer::GetWorld() const
{
	return (HasAnyFlags(RF_ClassDefaultObject) == true ? nullptr : GetOwningComponent()->GetWorld());
}

void UBoardActorSequencePlayer::PostInitProperties()
{
	Super::PostInitProperties();

	GetSequencePlayer()->SetPlaybackClient(this);
	GetSequencePlayer()->SetPlaybackSettings(mPlaybackSettings);
}

bool UBoardActorSequencePlayer::RetrieveBindingOverrides(const FGuid& InBindingId, FMovieSceneSequenceID InSequenceID, TArray<UObject*, TInlineAllocator<1>>& OutObjects) const
{
	return mBindingOverrides->LocateBoundObjects(InBindingId, InSequenceID, OutObjects);
}

UObject* UBoardActorSequencePlayer::GetInstanceData() const
{
	return mOverrideInstanceData == true ? mDefaultInstanceData : nullptr;
}

void UBoardActorSequencePlayer::NativeUpdateSequence(float DeltaSeconds)
{
	AActor* OwningActor = Cast<AActor>(GetOwningActor());
	if (OwningActor != nullptr)
	{
		const FVector ForwardNorDir = OwningActor->GetActorForwardVector();
		const FVector RightNorDir = OwningActor->GetActorRightVector();
		const FVector Velocity = OwningActor->GetVelocity();

		mActorVelocity.X = FVector::DotProduct(Velocity, ForwardNorDir);
		mActorVelocity.Y = FVector::DotProduct(Velocity, RightNorDir);
	}
}

bool UBoardActorSequencePlayer::PlaySequenceUsingTag(const FGameplayTag& MontageTag, ETileActorDirection LocalDirection)
{
	return PlaySequenceUsingTag_Internal(FBoardActorAnimationContext(MontageTag, LocalDirection));
}

bool UBoardActorSequencePlayer::PlaySequenceUsingTag(const FBoardActorAnimationContext& Context)
{
	return PlaySequenceUsingTag_Internal(Context);
}

void UBoardActorSequencePlayer::StopSequenceUsingTag()
{
	if (IsPlayingSequenceUsingTag() == false)
	{
		return;
	}

	if (mLevelSequencePlayer != nullptr)
	{
		mLevelSequencePlayer->Stop();
	}

	OnEndSequenceUsingTag(true);
}

bool UBoardActorSequencePlayer::PlaySequenceUsingTag_Internal(FBoardActorAnimationContext Context)
{
	if (mTagSequenceSets.Contains(Context.mMontageTag) == false)
	{
		// 존재하지 않는 레벨 시퀀스 태그
		return false;
	}

	TObjectPtr<ULevelSequence>& TargetLevelSequence = mTagSequenceSets[Context.mMontageTag].mLevelSequences[StaticCast<int32>(Context.mMontageDir)];
	if (TargetLevelSequence == nullptr)
	{
		// nullptr 레벨 시퀀스
		return false;
	}

	if (IsPlayingSequenceUsingTag() == true && GetPlayingSequenceSetUsingTag()->mPlayPriority > mTagSequenceSets[Context.mMontageTag].mPlayPriority)
	{
		// 우선순위가 낮은 시퀀스
		return false;
	}

	StopSequenceUsingTag();

	mLevelSequencePlayer->Initialize(TargetLevelSequence, GetOwningActor()->GetLevel(), mCameraSettings);
	ApplyDynamicBindings(TargetLevelSequence);

	mActiveAnimationContext = MoveTemp(Context);

	mLevelSequencePlayer->OnNativeFinished.BindUObject(this, &UBoardActorSequencePlayer::OnEndSequenceUsingTag, false);
	mLevelSequencePlayer->Play();

	return true;
}

void UBoardActorSequencePlayer::OnEndSequenceUsingTag(bool IsInterrupted)
{
	ULevelSequence* PlayedSequence = GetPlayingSequenceUsingTag();

	mActiveAnimationContext.mMontageEndEvent.Broadcast(mActiveAnimationContext, PlayedSequence, IsInterrupted);
	mActiveAnimationContext.Clear();
}

bool UBoardActorSequencePlayer::TriggerMontageTagEvent(const FGameplayTag& EventTag, const FEventTriggerPayloadBase* Payload)
{
	if (IsPlayingSequenceUsingTag() == false)
	{
		// 태그 기반 시퀀스 실행 중이 아님
		return false;
	}

	const bool HasMontageEvent = mActiveAnimationContext.mMontageEvents.Contains(EventTag);
	const bool HasAllMontageEvent = mAllMontageEvents.Contains(EventTag);
	if (HasMontageEvent == false && HasAllMontageEvent == false)
	{
		// 존재하지 않는 이벤트
		return false;
	}

	if (HasMontageEvent == true)
	{
		/* 현 시퀀스 대상 이벤트 실행 */
		FBoardActorAnimationEvent& TargetEvent = mActiveAnimationContext.mMontageEvents[EventTag];
		if (TargetEvent.OnTriggerAnimationEvent.IsBound() == true)
		{
			TargetEvent.OnTriggerAnimationEvent.Broadcast(mActiveAnimationContext, GetPlayingSequenceUsingTag(), Payload);
		}

		if (TargetEvent.mIsOneTimeEvent == true)
		{
			UnregisterTagEventOnMontage(EventTag);
		}
	}
	if (HasAllMontageEvent == true)
	{
		/* 모든 시퀀스 대상 이벤트 실행 */
		if (mAllMontageEvents[EventTag].OnTriggerAnimationEvent.IsBound() == true)
		{
			mAllMontageEvents[EventTag].OnTriggerAnimationEvent.Broadcast(mActiveAnimationContext, GetPlayingSequenceUsingTag(), Payload);
		}
	}

	return true;
}

bool UBoardActorSequencePlayer::RegisterTagEventOnMontage(const FGameplayTag& EventTag, FBoardActorAnimationEvent&& Event)
{
	if (IsPlayingSequenceUsingTag() == false)
	{
		// 태그 기반 시퀀스 실행 중이 아님
		return false;
	}

	// 이벤트 덮어씌우기
	mActiveAnimationContext.mMontageEvents.FindOrAdd(EventTag) = MoveTemp(Event);
	return true;
}

bool UBoardActorSequencePlayer::UnregisterTagEventOnMontage(const FGameplayTag& EventTag)
{
	if (IsPlayingSequenceUsingTag() == false)
	{
		// 태그 기반 시퀀스 실행 중이 아님
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

bool UBoardActorSequencePlayer::RegisterTagEventOnAllMontage(const FGameplayTag& EventTag, FBoardActorAllAnimationEvent&& Event)
{
	// 이벤트 덮어씌우기
	mAllMontageEvents.FindOrAdd(EventTag) = MoveTemp(Event);
	return true;
}

bool UBoardActorSequencePlayer::UnregisterTagEventOnAllMontage(const FGameplayTag& EventTag)
{
	if (mAllMontageEvents.Contains(EventTag) == false)
	{
		// 존재하지 않는 이벤트
		return false;
	}

	mAllMontageEvents.Remove(EventTag);
	return true;
}

void UBoardActorSequencePlayer::SetDynamicBinding(FName BindingTag, AActor* Actor)
{
	TArray<TWeakObjectPtr<AActor>>& Actors = mDynamicActorBindings.FindOrAdd(BindingTag);
	Actors.Empty();
	if (Actor != nullptr)
	{
		Actors.Add(Actor);
	}
}

void UBoardActorSequencePlayer::SetDynamicBindings(FName BindingTag, const TArray<AActor*>& Actors)
{
	TArray<TWeakObjectPtr<AActor>>& TargetActors = mDynamicActorBindings.FindOrAdd(BindingTag);
	TargetActors.Empty();
	for (AActor* Actor : Actors)
	{
		if (Actor != nullptr)
		{
			TargetActors.Add(Actor);
		}
	}
}

void UBoardActorSequencePlayer::AddDynamicBinding(FName BindingTag, AActor* Actor)
{
	if (Actor == nullptr)
	{
		return;
	}

	TArray<TWeakObjectPtr<AActor>>& Actors = mDynamicActorBindings.FindOrAdd(BindingTag);
	Actors.AddUnique(Actor);
}

void UBoardActorSequencePlayer::RemoveDynamicBinding(FName BindingTag, AActor* Actor)
{
	if (mDynamicActorBindings.Contains(BindingTag) == false)
	{
		return;
	}

	mDynamicActorBindings[BindingTag].Remove(Actor);
}

void UBoardActorSequencePlayer::ClearDynamicBindings(FName BindingTag)
{
	if (mDynamicActorBindings.Contains(BindingTag) == true)
	{
		mDynamicActorBindings.Remove(BindingTag);
	}
}

void UBoardActorSequencePlayer::ClearAllDynamicBindings()
{
	mDynamicActorBindings.Empty();
}

void UBoardActorSequencePlayer::SetBindingOnSequence(FMovieSceneObjectBindingID Binding, const TArray<AActor*>& Actors, bool AllowBindingsFromAsset)
{
	if (Binding.IsValid() == false)
	{
		return;
	}

	mBindingOverrides->SetBinding(Binding, TArray<UObject*>(Actors), AllowBindingsFromAsset);
	if (GetSequencePlayer() != nullptr)
	{
		FMovieSceneSequenceID SequenceID = Binding.ResolveSequenceID(MovieSceneSequenceID::Root, *GetSequencePlayer());
		GetSequencePlayer()->GetEvaluationState()->Invalidate(Binding.GetGuid(), SequenceID);
	}
}

void UBoardActorSequencePlayer::SetBindingOnSequenceByTag(const ULevelSequence* Sequence, FName BindingTag, const TArray<AActor*>& Actors, bool AllowBindingsFromAsset)
{
	const FMovieSceneObjectBindingIDs* Bindings = Sequence ? Sequence->GetMovieScene()->AllTaggedBindings().Find(BindingTag) : nullptr;
	if (Bindings)
	{
		for (FMovieSceneObjectBindingID BindingID : Bindings->IDs)
		{
			SetBindingOnSequence(BindingID, Actors, AllowBindingsFromAsset);
		}
	}
}

void UBoardActorSequencePlayer::ApplyDynamicBindings(const ULevelSequence* Sequence)
{
	if (Sequence == nullptr)
	{
		return;
	}

	if (mAutoBindOwnerActor == true)
	{
		AActor* OwnerActor = GetOwningActor();
		if (OwnerActor != nullptr)
		{
			TArray<AActor*> OwnerActorArray;
			OwnerActorArray.Add(OwnerActor);
			SetBindingOnSequenceByTag(Sequence, mDefaultOwnerActorBindingTag, OwnerActorArray);
		}
	}

	for (const auto& Pair : mDynamicActorBindings)
	{
		TArray<AActor*> ValidActors;
		for (const TWeakObjectPtr<AActor>& WeakActor : Pair.Value)
		{
			if (WeakActor.IsValid() == true)
			{
				ValidActors.Add(WeakActor.Get());
			}
		}

		if (ValidActors.IsEmpty() == true)
		{
			continue;
		}

		SetBindingOnSequenceByTag(Sequence, Pair.Key, ValidActors);
	}
}

bool UBoardActorSequencePlayer::IsPlayingSequenceUsingTag() const
{
	return mActiveAnimationContext.IsValid() == true;
}

ULevelSequence* UBoardActorSequencePlayer::GetPlayingSequenceUsingTag() const
{
	if (IsPlayingSequenceUsingTag() == false)
	{
		return nullptr;
	}

	if (mTagSequenceSets.Contains(mActiveAnimationContext.mMontageTag) == false)
	{
		return nullptr;
	}

	return mTagSequenceSets[mActiveAnimationContext.mMontageTag].mLevelSequences[StaticCast<int32>(mActiveAnimationContext.mMontageDir)];
}

const FTagLevelSequenceSet* UBoardActorSequencePlayer::GetPlayingSequenceSetUsingTag() const
{
	if (IsPlayingSequenceUsingTag() == false)
	{
		return nullptr;
	}

	return mTagSequenceSets.Find(mActiveAnimationContext.mMontageTag);
}

ULevelSequencePlayer* UBoardActorSequencePlayer::GetSequencePlayer() const
{
	return mLevelSequencePlayer;
}

UActorComponent* UBoardActorSequencePlayer::GetOwningComponent() const
{
	return GetOwningComponentChecked();
}

AActor* UBoardActorSequencePlayer::GetOwningActor() const
{
	return GetOwningComponent()->GetOwner();
}

UActorComponent* UBoardActorSequencePlayer::GetOwningComponentChecked() const
{
	return CastChecked<UActorComponent>(GetOuter());
}

UActorComponent* UBoardActorSequencePlayer::GetOwningComponentUnchecked() const
{
	return Cast<UActorComponent>(GetOuter());
}