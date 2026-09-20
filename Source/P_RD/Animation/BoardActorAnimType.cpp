#include "Animation/BoardActorAnimType.h"
#include "Animation/Notify/EventTriggerPayload.h"

FBoardActorAnimationContext::FBoardActorAnimationContext(FGameplayTag MontageTag, ETileActorDirection MontageDir) :
	mMontageTag(MontageTag),
	mMontageDir(MontageDir)
{
}

bool FBoardActorAnimationContext::IsValid() const
{
	return mMontageTag.IsValid() == true && mMontageDir != ETileActorDirection::Count;
}

void FBoardActorAnimationContext::Clear()
{
	mMetaData.Reset();

	mMontageTag = FGameplayTag::EmptyTag;
	mMontageDir = ETileActorDirection::Count;

	mMontageEvents.Empty();
	mMontageEndEvent.Clear();
}

