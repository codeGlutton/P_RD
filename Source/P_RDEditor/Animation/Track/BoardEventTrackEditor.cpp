#include "Animation/Track/BoardEventTrackEditor.h"
#include "Animation/Track/BoardEventTrack.h"

#include "Animation/Section/BoardEventSectionEditor.h"
#include "Animation/Section/BoardEventSectionBase.h"
#include "Animation/Section/BoardEventTriggerSection.h"
#include "Animation/Section/BoardEventDurationSection.h"

#include "ISequencer.h"
#include "SequencerUtilities.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"

#include "MVVM/Views/ViewUtilities.h"
#include "MovieSceneSequenceEditor.h"

#define LOCTEXT_NAMESPACE "BoardEventTrackEditor"

FBoardEventTrackEditor::FBoardEventTrackEditor(TSharedRef<ISequencer> InSequencer) : FMovieSceneTrackEditor(InSequencer)
{
}

TSharedRef<ISequencerSection> FBoardEventTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
	if (SectionObject.IsA<UBoardEventTriggerSection>())
	{
		return MakeShared<FBoardEventTriggerSectionEditor>(SectionObject, GetSequencer());
	}
	else if (SectionObject.IsA<UBoardEventDurationSection>())
	{
		return MakeShared<FBoardEventDurationSectionEditor>(SectionObject, GetSequencer());
	}

	return MakeShared<FSequencerSection>(SectionObject);
}

bool FBoardEventTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> TrackClass) const
{
	if (TrackClass == nullptr)
	{
		return false;
	}

	return TrackClass == UBoardEventTrack::StaticClass();
}

bool FBoardEventTrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
{
	//ETrackSupport TrackSupported = InSequence ? InSequence->IsTrackSupported(UBoardEventTrack::StaticClass()) : ETrackSupport::NotSupported;
	//return TrackSupported == ETrackSupport::Supported;
	return InSequence != nullptr;
}

void FBoardEventTrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
	UMovieSceneSequence* RootMovieSceneSequence = GetSequencer()->GetRootMovieSceneSequence();
	FMovieSceneSequenceEditor* SequenceEditor = FMovieSceneSequenceEditor::Find(RootMovieSceneSequence);

	if (SequenceEditor && SequenceEditor->SupportsEvents(RootMovieSceneSequence))
	{
		MenuBuilder.AddSubMenu(
			LOCTEXT("AddBoardEventTrack", "Board Event Track"),
			LOCTEXT("AddBoardEventTrackToolTip", "Add a new board event track for trigger and duration events."),
			FNewMenuDelegate::CreateRaw(this, &FBoardEventTrackEditor::AddEventSubMenu, TArray<FGuid>()),
			false,
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Sequencer.Tracks.Event")
		);
	}
}

TSharedPtr<SWidget> FBoardEventTrackEditor::BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params)
{
	check(Track != nullptr);

	TSharedPtr<ISequencer> SequencerPtr = GetSequencer();
	if (SequencerPtr.IsValid() == false)
	{
		return SNullWidget::NullWidget;
	}

	TWeakObjectPtr<UMovieSceneTrack> WeakTrack = Track;
	const int32 RowIndex = Params.TrackInsertRowIndex;
	auto SubMenuCallback = [this, WeakTrack, RowIndex]
		{
			FMenuBuilder MenuBuilder(true, nullptr);

			UMovieSceneTrack* TrackPtr = WeakTrack.Get();
			if (TrackPtr)
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("AddNewTriggerSection", "Trigger"),
					LOCTEXT("AddNewTriggerSectionTooltip", "Adds a new section that can trigger a specific event at a specific time"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &FBoardEventTrackEditor::CreateNewSection, TrackPtr, RowIndex + 1, UBoardEventTriggerSection::StaticClass(), true))
				);

				MenuBuilder.AddMenuEntry(
					LOCTEXT("AddNewDurationSection", "Duration"),
					LOCTEXT("AddNewDurationSectionTooltip", "Adds a new section that triggers an event at start time and end time"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &FBoardEventTrackEditor::CreateNewSection, TrackPtr, RowIndex + 1, UBoardEventDurationSection::StaticClass(), true))
				);
			}
			else
			{
				MenuBuilder.AddWidget(SNew(STextBlock).Text(LOCTEXT("InvalidTrack", "Track is no longer valid")), FText(), true);
			}

			return MenuBuilder.MakeWidget();
		};

	return UE::Sequencer::MakeAddButton(LOCTEXT("AddSection", "Section"), FOnGetContent::CreateLambda(SubMenuCallback), Params.ViewModel);
}

FText FBoardEventTrackEditor::GetDisplayName() const
{
	return LOCTEXT("BoardEventTrackEditor_DisplayName", "BoardEvent");
}

const FSlateBrush* FBoardEventTrackEditor::GetIconBrush() const
{
	return FAppStyle::GetBrush("Sequencer.Tracks.Event");
}

TSharedRef<ISequencerTrackEditor> FBoardEventTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> InSequencer)
{
	return MakeShareable(new FBoardEventTrackEditor(InSequencer));
}

void FBoardEventTrackEditor::AddEventSubMenu(FMenuBuilder& MenuBuilder, TArray<FGuid> ObjectBindings)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddNewTriggerSection", "Trigger"),
		LOCTEXT("AddNewTriggerSectionTooltip", "Adds a new section that can trigger a specific event at a specific time"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FBoardEventTrackEditor::HandleAddEventTrackMenuEntryExecute, ObjectBindings, UBoardEventTriggerSection::StaticClass())
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddNewDurationSection", "Duration"),
		LOCTEXT("AddNewDurationSectionTooltip", "Adds a new section that triggers an event at start time and end time"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FBoardEventTrackEditor::HandleAddEventTrackMenuEntryExecute, ObjectBindings, UBoardEventDurationSection::StaticClass())
		)
	);
}

void FBoardEventTrackEditor::HandleAddEventTrackMenuEntryExecute(TArray<FGuid> InObjectBindingIDs, UClass* SectionType)
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();
	if (FocusedMovieScene == nullptr)
	{
		return;
	}
	if (FocusedMovieScene->IsReadOnly())
	{
		return;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("Sequencer", "AddBoardEventTrack_Transaction", "Add Board Event Track"));
	FocusedMovieScene->Modify();

	TArray<UBoardEventTrack*> NewTracks;
	for (FGuid InObjectBindingID : InObjectBindingIDs)
	{
		if (InObjectBindingID.IsValid() == true)
		{
			UBoardEventTrack* NewObjectTrack = FocusedMovieScene->AddTrack<UBoardEventTrack>(InObjectBindingID);
			NewTracks.Add(NewObjectTrack);

			if (GetSequencer().IsValid() == true)
			{
				GetSequencer()->OnAddTrack(NewObjectTrack, InObjectBindingID);
			}
		}
	}

	if (NewTracks.IsEmpty() == true)
	{
		UBoardEventTrack* NewTrack = FocusedMovieScene->AddTrack<UBoardEventTrack>();
		NewTracks.Add(NewTrack);

		if (GetSequencer().IsValid() == true)
		{
			GetSequencer()->OnAddTrack(NewTrack, FGuid());
		}
	}

	check(NewTracks.Num() != 0);
	for (UBoardEventTrack* NewTrack : NewTracks)
	{
		CreateNewSection(NewTrack, 0, SectionType, false);
		NewTrack->SetDisplayName(LOCTEXT("TrackName", "BoardEvents"));
	}
}

void FBoardEventTrackEditor::CreateNewSection(UMovieSceneTrack* Track, int32 RowIndex, UClass* SectionType, bool IsSelected)
{
	TSharedPtr<ISequencer> SequencerPtr = GetSequencer();
	if (SequencerPtr.IsValid() == true)
	{
		UMovieScene* FocusedMovieScene = GetFocusedMovieScene();
		FQualifiedFrameTime CurrentTime = SequencerPtr->GetLocalTime();

		FScopedTransaction Transaction(LOCTEXT("CreateNewSectionTransactionText", "Add Section"));

		UMovieSceneSection* NewSection = NewObject<UMovieSceneSection>(Track, SectionType, NAME_None, RF_Transactional);
		check(NewSection);

		int32 OverlapPriority = 0;
		for (UMovieSceneSection* Section : Track->GetAllSections())
		{
			if (Section->GetRowIndex() >= RowIndex)
			{
				Section->SetRowIndex(Section->GetRowIndex() + 1);
			}
			OverlapPriority = FMath::Max(Section->GetOverlapPriority() + 1, OverlapPriority);
		}

		Track->Modify();

		if (SectionType == UBoardEventTriggerSection::StaticClass())
		{
			NewSection->SetRange(TRange<FFrameNumber>::All());
		}
		else
		{
			TRange<FFrameNumber> NewSectionRange;

			if (CurrentTime.Time.FrameNumber < FocusedMovieScene->GetPlaybackRange().GetUpperBoundValue())
			{
				NewSectionRange = TRange<FFrameNumber>(CurrentTime.Time.FrameNumber, FocusedMovieScene->GetPlaybackRange().GetUpperBoundValue());
			}
			else
			{
				const float DefaultLengthInSeconds = 5.f;
				NewSectionRange = TRange<FFrameNumber>(CurrentTime.Time.FrameNumber, CurrentTime.Time.FrameNumber + (DefaultLengthInSeconds * SequencerPtr->GetFocusedTickResolution()).FloorToFrame());
			}

			NewSection->SetRange(NewSectionRange);
		}

		NewSection->SetOverlapPriority(OverlapPriority);
		NewSection->SetRowIndex(RowIndex);

		Track->AddSection(*NewSection);
		Track->UpdateEasing();

		if (IsSelected == true)
		{
			SequencerPtr->EmptySelection();
			SequencerPtr->SelectSection(NewSection);
			SequencerPtr->ThrobSectionSelection();
		}

		SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
	}
}

#undef LOCTEXT_NAMESPACE
