#include "Animation/Section/BoardEventTriggerSection.h"
#include "Animation/Track/BoardEventTrack.h"
#include "Animation/Track/BoardEventTrackInstance.h"

#include "Channels/MovieSceneChannelProxy.h"
#include "MovieSceneTracksComponentTypes.h"

#include "EntitySystem/MovieSceneEntityBuilder.h"
#include "EntitySystem/MovieSceneInstanceRegistry.h"
#include "EntitySystem/MovieSceneEntitySystemLinker.h"

UBoardEventTriggerSection::UBoardEventTriggerSection(const FObjectInitializer& ObjInit)
{
	bSupportsInfiniteRange = true;
	SetRange(TRange<FFrameNumber>::All());

#if WITH_EDITOR

	ChannelProxy = MakeShared<FMovieSceneChannelProxy>(mEventChannel, FMovieSceneChannelMetaData());

#endif
}

void UBoardEventTriggerSection::ImportEntityImpl(UMovieSceneEntitySystemLinker* EntityLinker, const FEntityImportParams& Params, FImportedEntity* OutImportedEntity)
{
	using namespace UE::MovieScene;

	UBoardEventTrack* EventTrack = GetTypedOuter<UBoardEventTrack>();
	const FSequenceInstance& ThisInstance = EntityLinker->GetInstanceRegistry()->GetInstance(Params.Sequence.InstanceHandle);
	FMovieSceneContext Context = ThisInstance.GetContext();

	if (Context.GetStatus() == EMovieScenePlayerStatus::Stopped || Context.IsSilent() == true)
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

	FBuiltInComponentTypes* BuiltInComponents = FBuiltInComponentTypes::Get();
	FMovieSceneTrackInstanceComponent TrackInstance{ decltype(FMovieSceneTrackInstanceComponent::Owner)(this), UBoardEventTrackInstance::StaticClass() };

	FGuid ObjectBindingID = Params.GetObjectBindingID();
	OutImportedEntity->AddBuilder(
		FEntityBuilder()
		.Add(BuiltInComponents->TrackInstance, TrackInstance)
		.AddTag(BuiltInComponents->Tags.Root)
	);
}
