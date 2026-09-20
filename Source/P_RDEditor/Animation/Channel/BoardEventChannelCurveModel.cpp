#include "Animation/Channel/BoardEventChannelCurveModel.h"
#include "Animation/Section/BoardEventSectionBase.h"

#include "InvertedCurveModel.h"

#include "Algo/BinarySearch.h"
#include "Channels/MovieSceneChannelData.h"
#include "CurveDataAbstraction.h"
#include "CurveDrawInfo.h"
#include "Curves/RealCurve.h"
#include "HAL/PlatformCrt.h"
#include "Misc/AssertionMacros.h"
#include "Misc/FrameNumber.h"
#include "Misc/FrameRate.h"
#include "Misc/FrameTime.h"
#include "MovieScene.h"
#include "MovieSceneSection.h"
#include "MovieSceneTrack.h"
#include "SequencerSectionPainter.h"
#include "Styling/AppStyle.h"
#include "Styling/ISlateStyle.h"

class FCurveEditor;
struct FCurveEditorScreenSpace;

ECurveEditorViewID FBoardEventChannelCurveModel::EventView = ECurveEditorViewID::Invalid;

FBoardEventChannelCurveModel::FBoardEventChannelCurveModel(TMovieSceneChannelHandle<FBoardEventTriggerChannel> InChannel, UMovieSceneSection* InOwningSection, TWeakPtr<ISequencer> InWeakSequencer)
{
	mChannelHandle = InChannel;
	mWeakSection = InOwningSection;
	mWeakSequencer = InWeakSequencer;
	SupportedViews = EventView;

	Color = FSequencerSectionPainter::BlendColor(InOwningSection->GetTypedOuter<UMovieSceneTrack>()->GetColorTint());
}

const void* FBoardEventChannelCurveModel::GetCurve() const
{
	return mChannelHandle.Get();
}

void FBoardEventChannelCurveModel::Modify()
{
	if (UMovieSceneSection* Section = mWeakSection.Get())
	{
		Section->Modify();
	}
}

void FBoardEventChannelCurveModel::DrawCurve(const FCurveEditor& CurveEditor, const FCurveEditorScreenSpace& ScreenSpace, TArray<TTuple<double, double>>& InterpolatingPoints) const
{
}

void FBoardEventChannelCurveModel::GetKeys(double MinTime, double MaxTime, double MinValue, double MaxValue, TArray<FKeyHandle>& OutKeyHandles) const
{
	FBoardEventTriggerChannel* Channel = mChannelHandle.Get();
	UMovieSceneSection* Section = mWeakSection.Get();

	if (Channel && Section)
	{
		FFrameRate TickResolution = Section->GetTypedOuter<UMovieScene>()->GetTickResolution();

		TMovieSceneChannelData<FBoardEventTriggerData> ChannelData = Channel->GetData();
		TArrayView<const FFrameNumber> Times = ChannelData.GetTimes();

		const FFrameNumber StartFrame = MinTime <= MIN_int32 ? MIN_int32 : (MinTime * TickResolution).CeilToFrame();
		const FFrameNumber EndFrame = MaxTime >= MAX_int32 ? MAX_int32 : (MaxTime * TickResolution).FloorToFrame();

		const int32 StartingIndex = Algo::LowerBound(Times, StartFrame);
		const int32 EndingIndex = Algo::UpperBound(Times, EndFrame);

		for (int32 KeyIndex = StartingIndex; KeyIndex < EndingIndex; ++KeyIndex)
		{
			OutKeyHandles.Add(ChannelData.GetHandle(KeyIndex));
		}
	}
}

void FBoardEventChannelCurveModel::GetKeyDrawInfo(ECurvePointType PointType, const FKeyHandle InKeyHandle, FKeyDrawInfo& OutDrawInfo) const
{
	OutDrawInfo.Brush = FAppStyle::Get().GetBrush("Sequencer.KeyDiamond");
	OutDrawInfo.ScreenSize = FVector2D(10, 10);
}

void FBoardEventChannelCurveModel::GetKeyPositions(TArrayView<const FKeyHandle> InKeys, TArrayView<FKeyPosition> OutKeyPositions) const
{
	FBoardEventTriggerChannel* Channel = mChannelHandle.Get();
	UMovieSceneSection* Section = mWeakSection.Get();

	if (Channel && Section)
	{
		FFrameRate TickResolution = Section->GetTypedOuter<UMovieScene>()->GetTickResolution();

		TMovieSceneChannelData<FBoardEventTriggerData> ChannelData = Channel->GetData();
		TArrayView<const FFrameNumber> Times = ChannelData.GetTimes();
		TArrayView<const FBoardEventTriggerData> Values = ChannelData.GetValues();

		for (int32 Index = 0; Index < InKeys.Num(); ++Index)
		{
			int32 KeyIndex = ChannelData.GetIndex(InKeys[Index]);
			if (KeyIndex != INDEX_NONE)
			{
				OutKeyPositions[Index].InputValue = Times[KeyIndex] / TickResolution;
				OutKeyPositions[Index].OutputValue = 0.0;
			}
		}
	}
}

void FBoardEventChannelCurveModel::SetKeyPositions(TArrayView<const FKeyHandle> InKeys, TArrayView<const FKeyPosition> InKeyPositions, EPropertyChangeType::Type ChangeType)
{
	FBoardEventTriggerChannel* Channel = mChannelHandle.Get();
	UMovieSceneSection* Section = mWeakSection.Get();

	if (Channel && Section)
	{
		Section->MarkAsChanged();

		FFrameRate TickResolution = Section->GetTypedOuter<UMovieScene>()->GetTickResolution();

		TMovieSceneChannelData<FBoardEventTriggerData> ChannelData = Channel->GetData();
		for (int32 Index = 0; Index < InKeys.Num(); ++Index)
		{
			int32 KeyIndex = ChannelData.GetIndex(InKeys[Index]);
			if (KeyIndex != INDEX_NONE)
			{
				FFrameNumber NewTime = (InKeyPositions[Index].InputValue * TickResolution).FloorToFrame();

				const bool bRemoveDuplicates = ChangeType == EPropertyChangeType::ValueSet;
				KeyIndex = ChannelData.MoveKey(KeyIndex, NewTime, bRemoveDuplicates);
				Section->ExpandToFrame(NewTime);
			}
		}
	}
}

void FBoardEventChannelCurveModel::GetKeyAttributes(TArrayView<const FKeyHandle> InKeys, TArrayView<FKeyAttributes> OutAttributes) const
{
}

void FBoardEventChannelCurveModel::SetKeyAttributes(TArrayView<const FKeyHandle> InKeys, TArrayView<const FKeyAttributes> InAttributes, EPropertyChangeType::Type ChangeType)
{
}

void FBoardEventChannelCurveModel::GetCurveAttributes(FCurveAttributes& OutCurveAttributes) const
{
	OutCurveAttributes.SetPreExtrapolation(RCCE_None);
	OutCurveAttributes.SetPostExtrapolation(RCCE_None);
}

void FBoardEventChannelCurveModel::SetCurveAttributes(const FCurveAttributes& InCurveAttributes)
{
}

void FBoardEventChannelCurveModel::GetTimeRange(double& MinTime, double& MaxTime) const
{
	FBoardEventTriggerChannel* Channel = mChannelHandle.Get();
	UMovieSceneSection* Section = mWeakSection.Get();

	if (Channel && Section)
	{
		TArrayView<const FFrameNumber> Times = Channel->GetData().GetTimes();
		if (Times.Num() == 0)
		{
			MinTime = 0.f;
			MaxTime = 0.f;
		}
		else
		{
			FFrameRate TickResolution = Section->GetTypedOuter<UMovieScene>()->GetTickResolution();
			double ToTime = TickResolution.AsInterval();
			MinTime = static_cast<double> (Times[0].Value) * ToTime;
			MaxTime = static_cast<double>(Times[Times.Num() - 1].Value) * ToTime;
		}
	}
}

void FBoardEventChannelCurveModel::GetValueRange(double& MinValue, double& MaxValue) const
{
	MinValue = MaxValue = 0.0;
}

int32 FBoardEventChannelCurveModel::GetNumKeys() const
{
	FBoardEventTriggerChannel* Channel = mChannelHandle.Get();

	if (Channel != nullptr)
	{
		return Channel->GetData().GetTimes().Num();
	}
	return 0;
}

bool FBoardEventChannelCurveModel::Evaluate(double ProspectiveTime, double& OutValue) const
{
	OutValue = 0.0;
	return false;
}

void FBoardEventChannelCurveModel::AddKeys(TArrayView<const FKeyPosition> InKeyPositions, TArrayView<const FKeyAttributes> InAttributes, TArrayView<TOptional<FKeyHandle>>* OutKeyHandles)
{
	check(InKeyPositions.Num() == InAttributes.Num() && (!OutKeyHandles || OutKeyHandles->Num() == InKeyPositions.Num()));

	FBoardEventTriggerChannel* Channel = mChannelHandle.Get();
	UMovieSceneSection* Section = mWeakSection.Get();
	if (Channel && Section)
	{
		Section->Modify();
		TMovieSceneChannelData<FBoardEventTriggerData> ChannelData = Channel->GetData();
		FFrameRate TickResolution = Section->GetTypedOuter<UMovieScene>()->GetTickResolution();

		for (int32 Index = 0; Index < InKeyPositions.Num(); ++Index)
		{
			FKeyPosition   Position = InKeyPositions[Index];
			FKeyAttributes Attributes = InAttributes[Index];

			FFrameNumber Time = (Position.InputValue * TickResolution).RoundToFrame();
			Section->ExpandToFrame(Time);

			FBoardEventTriggerData Value;
			FKeyHandle NewHandle = ChannelData.UpdateOrAddKey(Time, Value);
			if (NewHandle != FKeyHandle::Invalid())
			{
				if (OutKeyHandles)
				{
					(*OutKeyHandles)[Index] = NewHandle;
				}
			}
		}
	}
}

void FBoardEventChannelCurveModel::RemoveKeys(TArrayView<const FKeyHandle> InKeys, double InCurrentTime)
{
	FBoardEventTriggerChannel* Channel = mChannelHandle.Get();
	UMovieSceneSection* Section = mWeakSection.Get();
	if (Channel && Section)
	{
		Section->Modify();

		TMovieSceneChannelData<FBoardEventTriggerData> ChannelData = Channel->GetData();

		for (FKeyHandle Handle : InKeys)
		{
			int32 KeyIndex = ChannelData.GetIndex(Handle);
			if (KeyIndex != INDEX_NONE)
			{
				ChannelData.RemoveKey(KeyIndex);
			}
		}
	}
}

void FBoardEventChannelCurveModel::CreateKeyProxies(TWeakPtr<FCurveEditor> InWeakCurveEditor, FCurveModelID InCurveModelID, TArrayView<const FKeyHandle> InKeyHandles, TArrayView<UObject*> OutObjects)
{
}

bool SupportsCurveEditorModels(const TMovieSceneChannelHandle<FBoardEventTriggerChannel>& InChannelHandle)
{
	return true;
}

TUniquePtr<FCurveModel> CreateCurveEditorModel(const TMovieSceneChannelHandle<FBoardEventTriggerChannel>& InChannelHandle, const UE::Sequencer::FCreateCurveEditorModelParams& InParams)
{
	if (InChannelHandle.GetMetaData() != nullptr && InChannelHandle.GetMetaData()->bInvertValue == true)
	{
		return MakeUnique<UE::CurveEditor::TInvertedCurveModel<FBoardEventChannelCurveModel>>(InChannelHandle, InParams.OwningSection, InParams.Sequencer);
	}
	return MakeUnique<FBoardEventChannelCurveModel>(InChannelHandle, InParams.OwningSection, InParams.Sequencer);
}

void DrawKeys(FBoardEventTriggerChannel* InChannel, TArrayView<const FKeyHandle> InKeyHandles, const UMovieSceneSection* InOwner, TArrayView<FKeyDrawParams> OutKeyDrawParams)
{
	UBoardEventSectionBase* EventSection = CastChecked<UBoardEventSectionBase>(const_cast<UMovieSceneSection*>(InOwner));

	FKeyDrawParams ValidEventParams, InvalidEventParams;

	ValidEventParams.BorderBrush = ValidEventParams.FillBrush = FAppStyle::Get().GetBrush("Sequencer.KeyDiamond");

	TMovieSceneChannelData<FBoardEventTriggerData> ChannelData = InChannel->GetData();
	TArrayView<FBoardEventTriggerData> Events = ChannelData.GetValues();

	for (int32 Index = 0; Index < InKeyHandles.Num(); ++Index)
	{
		OutKeyDrawParams[Index] = ValidEventParams;
	}
}
