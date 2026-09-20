#include "Animation/Track/BoardEventTrackInstance.h"
#include "Animation/BoardActorSequencePlayer.h"
#include "LevelSequencePlayer.h"
#include "IMovieScenePlayer.h"
#include "Evaluation/IMovieScenePlaybackCapability.h"

DEFINE_LOG_CATEGORY(LogBoardEventTrack)

UBoardEventTrackInstance::UBoardEventTrackInstance()
{
}

void UBoardEventTrackInstance::OnInputAdded(const FMovieSceneTrackInstanceInput& InInput)
{
	if (InInput.Section == nullptr)
	{
		return;
	}

	if (const UBoardEventTriggerSection* TriggerSection = Cast<UBoardEventTriggerSection>(InInput.Section))
	{
		TArrayView<const FFrameNumber> KeyTimes = TriggerSection->mEventChannel.GetKeyTimes();
		TArrayView<const FBoardEventTriggerData> KeyValues = TriggerSection->mEventChannel.GetKeyValues();

		FBoardSceneEventInstanceContext& Context = mActiveTriggerEvents.FindOrAdd(TriggerSection);
		Context.mInstanceHandle = InInput.InstanceHandle;

		const int32 NumKeys = KeyTimes.Num();
		for (int32 Index = 0; Index < NumKeys; ++Index)
		{
			FBoardSceneEventInstanceData EventData;
			EventData.mKeyTime = KeyTimes[Index];
			EventData.mEvent.mEventTag = KeyValues[Index].mEventTag; 
			EventData.mEvent.mEventPayload = KeyValues[Index].mEventPayload;

			Context.mDatas.Add(MoveTemp(EventData));
		}
	}
	else if (const UBoardEventDurationSection* DurationSection = Cast<UBoardEventDurationSection>(InInput.Section))
	{
		FBoardSceneEventInstanceContext& Context = mActiveDurationEvents.FindOrAdd(DurationSection);
		Context.mInstanceHandle = InInput.InstanceHandle;

		FBoardSceneEventInstanceData EventData;
		EventData.mEvent.mEventTag = DurationSection->mEvent.mEventTag;
		EventData.mEvent.mEventPayload = DurationSection->mEvent.mEventPayload;

		Context.mDatas.Add(MoveTemp(EventData));

		TriggerBoardEvent(Context.mDatas[0].mEvent, InInput.InstanceHandle);
	}
}

void UBoardEventTrackInstance::OnInputRemoved(const FMovieSceneTrackInstanceInput& InInput)
{
	if (InInput.Section == nullptr)
	{
		return;
	}

	if (mActiveTriggerEvents.Contains(InInput.Section) == true)
	{
		mActiveTriggerEvents.Remove(InInput.Section);
	}
	else if (mActiveDurationEvents.Contains(InInput.Section) == true)
	{
		FBoardSceneEventInstanceContext& Context = mActiveDurationEvents[InInput.Section];
		TriggerBoardEventEnd(Context.mDatas[0].mEvent, InInput.InstanceHandle);

		mActiveDurationEvents.Remove(InInput.Section);
	}
}

void UBoardEventTrackInstance::OnAnimate()
{
	using namespace UE::MovieScene;

	FInstanceRegistry* InstanceRegistry = GetLinker()->GetInstanceRegistry();
	for (auto& ActiveTriggerEventPair : mActiveTriggerEvents)
	{
		FBoardSceneEventInstanceContext& EventInstanceContext = ActiveTriggerEventPair.Value;
		TRange<FFrameNumber> CurFrameNumberRange = GetCurrentFrameNumberRange(EventInstanceContext.mInstanceHandle);

		for (int32 DataIndex = 0; DataIndex < EventInstanceContext.mDatas.Num(); ++DataIndex)
		{
			if (CurFrameNumberRange.Contains(EventInstanceContext.mDatas[DataIndex].mKeyTime) == true)
			{
				TriggerBoardEvent(EventInstanceContext.mDatas[DataIndex].mEvent, EventInstanceContext.mInstanceHandle);

				EventInstanceContext.mDatas.RemoveAtSwap(DataIndex);
				--DataIndex;
			}
		}
	}
}

TRange<FFrameNumber> UBoardEventTrackInstance::GetCurrentFrameNumberRange(UE::MovieScene::FInstanceHandle Handle) const
{
	using namespace UE::MovieScene;

	FInstanceRegistry* InstanceRegistry = GetLinker()->GetInstanceRegistry();
	const FSequenceInstance& Instance = InstanceRegistry->GetInstance(Handle);
	return Instance.GetContext().GetFrameNumberRange();
}

UBoardActorSequencePlayer* UBoardEventTrackInstance::GetBoardActorSequencePlayer(UE::MovieScene::FInstanceHandle Handle) const
{
	using namespace UE::MovieScene;

	FInstanceRegistry* InstanceRegistry = GetLinker()->GetInstanceRegistry();
	const FSequenceInstance& Instance = InstanceRegistry->GetInstance(Handle);
	IMovieScenePlayer* Player = FPlayerIndexPlaybackCapability::GetPlayer(Instance.GetSharedPlaybackState());
	if (Player == nullptr)
	{
		return nullptr;
	}
	ULevelSequencePlayer* SequencePlayer = Cast<ULevelSequencePlayer>(Player->AsUObject());
	if (SequencePlayer == nullptr)
	{
		return nullptr;
	}

	return Cast<UBoardActorSequencePlayer>(SequencePlayer->GetOuter());
}

void UBoardEventTrackInstance::TriggerBoardEvent(const FBoardSceneEvent& Event, UE::MovieScene::FInstanceHandle Handle)
{
	UE_LOG(LogBoardEventTrack, Log, TEXT("보드 이벤트 시도"));

	UBoardActorSequencePlayer* BoardActorSequencePlayer = GetBoardActorSequencePlayer(Handle);
	if (BoardActorSequencePlayer == nullptr)
	{
		return;
	}

	if (Event.mEventTag.IsValid() == true)
	{
		BoardActorSequencePlayer->TriggerMontageTagEvent(Event.mEventTag, Event.mEventPayload.GetPtr());
		UE_LOG(LogBoardEventTrack, Log, TEXT("보드 이벤트 실행"));
	}
}

void UBoardEventTrackInstance::TriggerBoardEventEnd(const FBoardSceneEvent& Event, UE::MovieScene::FInstanceHandle Handle)
{
	UE_LOG(LogBoardEventTrack, Log, TEXT("보드 이벤트 종료 콜백 시도"));

	UBoardActorSequencePlayer* BoardActorSequencePlayer = GetBoardActorSequencePlayer(Handle);
	if (BoardActorSequencePlayer == nullptr)
	{
		return;
	}

	const FDurationEventTriggerPayload* Payload = Event.mEventPayload.GetPtr<FDurationEventTriggerPayload>();
	if (Event.mEventTag.IsValid() == true && Payload != nullptr)
	{
		Payload->OnEndDurationEventTrigger.ExecuteIfBound();
		UE_LOG(LogBoardEventTrack, Log, TEXT("보드 이벤트 종료 콜백 실행"));
	}
}
