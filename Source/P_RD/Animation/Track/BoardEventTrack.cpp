#include "Animation/Track/BoardEventTrack.h"
#include "Animation/Section/BoardEventSectionBase.h"
#include "Animation/Section/BoardEventTriggerSection.h"
#include "Animation/Section/BoardEventDurationSection.h"

#include "Animation/Track/BoardEventTrackInstance.h"

#define LOCTEXT_NAMESPACE "BoardEventTrack"

UBoardEventTrack::UBoardEventTrack() : mFireEventsWhenForwards(1), mFireEventsWhenBackwards(0)
{
#if WITH_EDITORONLY_DATA
	TrackTint = FColor(180, 100, 255);
#endif
}

bool UBoardEventTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
	if (SectionClass == nullptr)
	{
		return false;
	}

	return SectionClass->IsChildOf(UBoardEventSectionBase::StaticClass());
}

void UBoardEventTrack::AddSection(UMovieSceneSection& Section)
{
	mSections.Add(&Section);
}

void UBoardEventTrack::RemoveSection(UMovieSceneSection& Section)
{
	mSections.Remove(&Section);
}

void UBoardEventTrack::RemoveSectionAt(int32 SectionIndex)
{
	if (mSections.IsValidIndex(SectionIndex) == true)
	{
		mSections.RemoveAt(SectionIndex);
	}
}

UMovieSceneSection* UBoardEventTrack::CreateNewSection()
{
	/* 기본 생성 섹션은 단발성 이벤트를 기본값으로 생성 */
	return NewObject<UBoardEventTriggerSection>(this, NAME_None, RF_Transactional);
}

const TArray<UMovieSceneSection*>& UBoardEventTrack::GetAllSections() const
{
	return reinterpret_cast<const TArray<UMovieSceneSection*>&>(mSections);
}

bool UBoardEventTrack::HasSection(const UMovieSceneSection& Section) const
{
	return mSections.Contains(&Section);
}

bool UBoardEventTrack::IsEmpty() const
{
	return mSections.IsEmpty();
}

bool UBoardEventTrack::SupportsMultipleRows() const
{
	return true;
}

#if WITH_EDITORONLY_DATA
void UBoardEventTrack::PostRename(UObject* OldOuter, const FName OldName)
{
	if (OldOuter != GetOuter())
	{
		Super::PostRename(OldOuter, OldName);
	}
}

FText UBoardEventTrack::GetDefaultDisplayName() const
{
	return LOCTEXT("TrackName", "Board Event Track");
}
#endif

void UBoardEventTrack::PopulateDeterminismData(FMovieSceneDeterminismData& OutData, const TRange<FFrameNumber>& Range) const
{
	OutData.bParentSequenceRequiresLowerFence = true;
	OutData.bParentSequenceRequiresUpperFence = true;
}

#undef LOCTEXT_NAMESPACE
