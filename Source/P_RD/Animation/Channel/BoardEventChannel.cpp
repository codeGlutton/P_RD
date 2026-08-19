#include "Animation/Channel/BoardEventChannel.h"

FBoardSceneEvent::FBoardSceneEvent(FGameplayTag EventTag) : mEventTag(EventTag)
{
}

FBoardEventTriggerData::FBoardEventTriggerData(FGameplayTag EventTag) : mEventTag(EventTag)
{
}

FBoardEventDurationData::FBoardEventDurationData(FGameplayTag EventTag) : mEventTag(EventTag)
{
}

void FBoardEventTriggerChannel::GetKeys(const TRange<FFrameNumber>& WithinRange, TArray<FFrameNumber>* OutKeyTimes, TArray<FKeyHandle>* OutKeyHandles)
{
	GetData().GetKeys(WithinRange, OutKeyTimes, OutKeyHandles);
}

void FBoardEventTriggerChannel::GetKeyTimes(TArrayView<const FKeyHandle> InHandles, TArrayView<FFrameNumber> OutKeyTimes)
{
	GetData().GetKeyTimes(InHandles, OutKeyTimes);
}

void FBoardEventTriggerChannel::SetKeyTimes(TArrayView<const FKeyHandle> InHandles, TArrayView<const FFrameNumber> InKeyTimes)
{
	GetData().SetKeyTimes(InHandles, InKeyTimes);
}

void FBoardEventTriggerChannel::DuplicateKeys(TArrayView<const FKeyHandle> InHandles, TArrayView<FKeyHandle> OutNewHandles)
{
	GetData().DuplicateKeys(InHandles, OutNewHandles);
}

void FBoardEventTriggerChannel::DeleteKeys(TArrayView<const FKeyHandle> InHandles)
{
	GetData().DeleteKeys(InHandles);
}

void FBoardEventTriggerChannel::DeleteKeysFrom(FFrameNumber InTime, bool bDeleteForward)
{
	GetData().DeleteKeysFrom(InTime, bDeleteForward);
}

void FBoardEventTriggerChannel::RemapTimes(const UE::MovieScene::IRetimingInterface& Retimer)
{
	GetData().RemapTimes(Retimer);
}

TRange<FFrameNumber> FBoardEventTriggerChannel::ComputeEffectiveRange() const
{
	return GetData().GetTotalRange();
}

int32 FBoardEventTriggerChannel::GetNumKeys() const
{
	return mKeyTimes.Num();
}

void FBoardEventTriggerChannel::Reset()
{
	mKeyTimes.Reset();
	mKeyValues.Reset();
	mKeyHandles.Reset();
}

void FBoardEventTriggerChannel::Offset(FFrameNumber DeltaPosition)
{
	GetData().Offset(DeltaPosition);
}

FKeyHandle FBoardEventTriggerChannel::GetHandle(int32 Index)
{
	return GetData().GetHandle(Index);
}

int32 FBoardEventTriggerChannel::GetIndex(FKeyHandle Handle)
{
	return GetData().GetIndex(Handle);
}

TMovieSceneChannelData<FBoardEventTriggerData> FBoardEventTriggerChannel::GetData()
{
	return TMovieSceneChannelData<FBoardEventTriggerData>(&mKeyTimes, &mKeyValues, this, &mKeyHandles);
}

TMovieSceneChannelData<const FBoardEventTriggerData> FBoardEventTriggerChannel::GetData() const
{
	return TMovieSceneChannelData<const FBoardEventTriggerData>(&mKeyTimes, &mKeyValues);
}

