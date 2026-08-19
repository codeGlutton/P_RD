#include "Animation/Section/BoardEventDurationSection.h"
#include "Animation/Track/BoardEventTrack.h"
#include "Animation/Track/BoardEventTrackInstance.h"

#include "EntitySystem/MovieSceneEntityBuilder.h"
#include "EntitySystem/MovieSceneEntityManager.h"
#include "EntitySystem/BuiltInComponentTypes.h"
#include "EntitySystem/MovieSceneSequenceInstance.h"
#include "EntitySystem/MovieSceneEntitySystemLinker.h"
#include "MovieSceneTracksComponentTypes.h"

UBoardEventDurationSection::UBoardEventDurationSection()
{
}

void UBoardEventDurationSection::ImportEntityImpl(UMovieSceneEntitySystemLinker* EntityLinker, const FEntityImportParams& Params, FImportedEntity* OutImportedEntity)
{
	using namespace UE::MovieScene;

	if (mEvent.mEventTag.IsValid() == false)
	{
		return;
	}

	UBoardEventTrack* EventTrack = GetTypedOuter<UBoardEventTrack>();
	const FSequenceInstance& ThisInstance = EntityLinker->GetInstanceRegistry()->GetInstance(Params.Sequence.InstanceHandle);
	FMovieSceneContext Context = ThisInstance.GetContext();

	if (Context.IsSilent() == true)
	{
		return;
	}
	else if (Context.GetDirection() == EPlayDirection::Forwards && EventTrack->mFireEventsWhenForwards == false)
	{
		return;
	}
	else if (Context.GetDirection() == EPlayDirection::Backwards && EventTrack->mFireEventsWhenBackwards == false)
	{
		return;
	}
	else if (GetRange().Contains(Context.GetTime().FrameNumber) == false)
	{
		return;
	}

	FBuiltInComponentTypes* BuiltInComponents = FBuiltInComponentTypes::Get();
	FMovieSceneTrackInstanceComponent TrackInstance{ decltype(FMovieSceneTrackInstanceComponent::Owner)(this), UBoardEventTrackInstance::StaticClass() };

	FGuid ObjectBindingID = Params.GetObjectBindingID();
	OutImportedEntity->AddBuilder(
		FEntityBuilder()
		.Add(BuiltInComponents->TrackInstance, TrackInstance)
		.AddTag(BuiltInComponents->Tags.Root)
	);
}


