/*****************************************************************//**
 * @file   BoardEventSectionEditor.h
 * @brief  보드 이벤트 시퀀스 섹션의 에디터 타임라인 렌더링 클래스 정의 헤더
 * @author 모호재
 * @date   2026-08-19
 *********************************************************************/

#pragma once

#include "RDEditorMinimal.h"
#include "ISequencerSection.h"
#include "ISequencer.h"

class FSequencerSectionPainter;

/**
 * @brief 보드 이벤트 시퀀스 섹션의 타임라인 렌더링 및 UI 연동 관리자
 */
class FBoardEventSectionEditorBase : public FSequencerSection
{
public:
	FBoardEventSectionEditorBase(UMovieSceneSection& InSectionObject, TWeakPtr<ISequencer> InSequencer);

public:
	void PaintEventName(FSequencerSectionPainter& Painter, int32 LayerId, const FString& EventString, float PixelPosition, bool IsEventValid = true) const;
	bool IsSectionSelected() const;

protected:
	TWeakPtr<ISequencer> mSequencer;
};

class FBoardEventTriggerSectionEditor : public FBoardEventSectionEditorBase
{
public:
	FBoardEventTriggerSectionEditor(UMovieSceneSection& InSectionObject, TWeakPtr<ISequencer> InSequencer);

public:
	int32 OnPaintSection(FSequencerSectionPainter& Painter) const override;
};

class FBoardEventDurationSectionEditor : public FBoardEventSectionEditorBase
{
public:
	FBoardEventDurationSectionEditor(UMovieSceneSection& InSectionObject, TWeakPtr<ISequencer> InSequencer);

public:
	int32 OnPaintSection(FSequencerSectionPainter& Painter) const override;
};

