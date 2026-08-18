#include "Animation/Section/BoardEventSectionEditor.h"
#include "Animation/Section/BoardEventSectionBase.h"
#include "Animation/Section/BoardEventTriggerSection.h"
#include "Animation/Section/BoardEventDurationSection.h"
#include "Animation/Channel/BoardEventChannel.h"

#include "SequencerSectionPainter.h"
#include "MovieSceneSequence.h"

#include "TimeToPixel.h"

#define LOCTEXT_NAMESPACE "BoardEventSectionEditor"

FBoardEventSectionEditorBase::FBoardEventSectionEditorBase(UMovieSceneSection& InSectionObject, TWeakPtr<ISequencer> InSequencer) : FSequencerSection(InSectionObject), mSequencer(InSequencer)
{
}

void FBoardEventSectionEditorBase::PaintEventName(FSequencerSectionPainter& Painter, int32 LayerId, const FString& InEventString, float PixelPos, bool IsEventValid) const
{
	static const int32 FontSize = 10;
	static const float BoxOffsetPx = 10.f;
	static const TCHAR* WarningString = TEXT("\xf071");

	const FSlateFontInfo FontAwesomeFont = FAppStyle::Get().GetFontStyle("FontAwesome.10");
	const FSlateFontInfo SmallLayoutFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);
	const FLinearColor DrawColor = FAppStyle::GetSlateColor("SelectionColor").GetColor(FWidgetStyle());

	TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	static FVector2D WarningSize = FontMeasureService->Measure(WarningString, FontAwesomeFont);
	const FMargin WarningPadding = (IsEventValid || InEventString.Len() == 0) ? FMargin(0.f) : FMargin(0.f, 0.f, 4.f, 0.f);
	const FMargin BoxPadding = FMargin(4.0f, 2.0f);

	const FVector2D TextSize = FontMeasureService->Measure(InEventString, SmallLayoutFont);
	const FVector2D IconSize = IsEventValid ? FVector2D::ZeroVector : WarningSize;
	const FVector2D PaddedIconSize = IconSize + WarningPadding.GetDesiredSize();
	const FVector2D BoxSize = FVector2D(TextSize.X + PaddedIconSize.X, FMath::Max(TextSize.Y, PaddedIconSize.Y)) + BoxPadding.GetDesiredSize();

	bool IsDrawLeft = (Painter.SectionGeometry.Size.X - PixelPos) < (BoxSize.X + 22.f) - BoxOffsetPx;
	float BoxPositionX = IsDrawLeft ? PixelPos - BoxSize.X - BoxOffsetPx : PixelPos + BoxOffsetPx;
	if (BoxPositionX < 0.f)
	{
		BoxPositionX = 0.f;
	}

	FVector2D BoxOffset = FVector2D(BoxPositionX, Painter.SectionGeometry.Size.Y * .5f - BoxSize.Y * .5f);
	FVector2D IconOffset = FVector2D(BoxPadding.Left, BoxSize.Y * .5f - IconSize.Y * .5f);
	FVector2D TextOffset = FVector2D(IconOffset.X + PaddedIconSize.X, BoxSize.Y * .5f - TextSize.Y * .5f);

	/* 배경 */

	FSlateDrawElement::MakeBox(
		Painter.DrawElements,
		LayerId + 1,
		Painter.SectionGeometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(BoxOffset)),
		FAppStyle::GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		FLinearColor::Black.CopyWithNewOpacity(0.5f)
	);

	if (IsEventValid == false)
	{
		/* 경고 메세지 */

		FSlateDrawElement::MakeText(
			Painter.DrawElements,
			LayerId + 2,
			Painter.SectionGeometry.ToPaintGeometry(IconSize, FSlateLayoutTransform(BoxOffset + IconOffset)),
			WarningString,
			FontAwesomeFont,
			Painter.bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect,
			FAppStyle::GetWidgetStyle<FTextBlockStyle>("Log.Warning").ColorAndOpacity.GetSpecifiedColor()
		);
	}

	/* 이벤트 메세지 */

	FSlateDrawElement::MakeText(
		Painter.DrawElements,
		LayerId + 2,
		Painter.SectionGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(BoxOffset + TextOffset)),
		InEventString,
		SmallLayoutFont,
		Painter.bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect,
		DrawColor
	);
}

bool FBoardEventSectionEditorBase::IsSectionSelected() const
{
	TSharedPtr<ISequencer> SequencerPtr = mSequencer.Pin();

	TArray<UMovieSceneTrack*> SelectedTracks;
	SequencerPtr->GetSelectedTracks(SelectedTracks);

	UMovieSceneSection* Section = WeakSection.Get();
	UMovieSceneTrack* Track = Section ? CastChecked<UMovieSceneTrack>(Section->GetOuter()) : nullptr;
	return Track && SelectedTracks.Contains(Track);
}

FBoardEventTriggerSectionEditor::FBoardEventTriggerSectionEditor(UMovieSceneSection& InSectionObject, TWeakPtr<ISequencer> InSequencer) : FBoardEventSectionEditorBase(InSectionObject, InSequencer)
{
}

int32 FBoardEventTriggerSectionEditor::OnPaintSection(FSequencerSectionPainter& Painter) const
{
	int32 LayerId = Painter.PaintSectionBackground();

	UBoardEventTriggerSection* EventTriggerSection = Cast<UBoardEventTriggerSection>(WeakSection.Get());
	if (EventTriggerSection == nullptr || IsSectionSelected() == false)
	{
		return LayerId;
	}

	const FTimeToPixel& TimeToPixelConverter = Painter.GetTimeConverter();
	TArrayView<const FFrameNumber> Times = EventTriggerSection->mEventChannel.GetData().GetTimes();

	TRange<FFrameNumber> EventSectionRange = EventTriggerSection->GetRange();
	for (int32 KeyIndex = 0; KeyIndex < Times.Num(); ++KeyIndex)
	{
		FFrameNumber EventTime = Times[KeyIndex];
		if (EventSectionRange.Contains(EventTime))
		{
			const float PixelPos = TimeToPixelConverter.FrameToPixel(EventTime);
			PaintEventName(Painter, LayerId, TEXT("TriggerEvent"), PixelPos, true);
		}
	}

	return LayerId + 3;
}

FBoardEventDurationSectionEditor::FBoardEventDurationSectionEditor(UMovieSceneSection& InSectionObject, TWeakPtr<ISequencer> InSequencer) : FBoardEventSectionEditorBase(InSectionObject, InSequencer)
{
}

int32 FBoardEventDurationSectionEditor::OnPaintSection(FSequencerSectionPainter& Painter) const
{
	int32 LayerId = Painter.PaintSectionBackground();

	UBoardEventDurationSection* EventDurationSection = Cast<UBoardEventDurationSection>(WeakSection.Get());
	if (EventDurationSection == nullptr)
	{
		return LayerId;
	}

	FString EventName = EventDurationSection->mEvent.mEventPayload.IsValid() == true ? EventDurationSection->mEvent.mEventPayload.GetScriptStruct()->GetName() : TEXT("DurationEvent");
	float TextOffsetX = EventDurationSection->GetRange().GetLowerBound().IsClosed() ? FMath::Max(0.f, Painter.GetTimeConverter().FrameToPixel(EventDurationSection->GetRange().GetLowerBoundValue())) : 0.f;
	PaintEventName(Painter, LayerId, EventName, TextOffsetX, true);

	return LayerId + 1;
}

#undef LOCTEXT_NAMESPACE

