/*****************************************************************//**
 * @file   BoardEventTrackEditor.h
 * @brief  보드 이벤트 트랙의 시퀀서 에디터 UI 및 메뉴 연동을 담당하는 클래스 정의 헤더
 * @author 모호재
 * @date   2026-08-19
 *********************************************************************/

#pragma once

#include "RDEditorMinimal.h"
#include "MovieSceneTrackEditor.h"

class FMenuBuilder;

/**
 * @brief 보드 이벤트 트랙의 시퀀서 에디터 연동 관리자
 */
class FBoardEventTrackEditor : public FMovieSceneTrackEditor
{
public:
	FBoardEventTrackEditor(TSharedRef<ISequencer> InSequencer);

	/* FMovieSceneTrackEditor 상속 */
public:
	TSharedRef<ISequencerSection> MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding) override;

	bool SupportsType(TSubclassOf<UMovieSceneTrack> TrackClass) const override;
	bool SupportsSequence(UMovieSceneSequence* InSequence) const override;

	void BuildAddTrackMenu(FMenuBuilder& MenuBuilder) override;
	TSharedPtr<SWidget> BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params) override;

	FText GetDisplayName() const override;
	const FSlateBrush* GetIconBrush() const override;

public:
	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor(TSharedRef<ISequencer> InSequencer);

private:
	void AddEventSubMenu(FMenuBuilder& MenuBuilder, TArray<FGuid> ObjectBindings);
	void HandleAddEventTrackMenuEntryExecute(TArray<FGuid> InObjectBindingIDs, UClass* SectionType);
	void CreateNewSection(UMovieSceneTrack* Track, int32 RowIndex, UClass* SectionType, bool IsSelected);
};
