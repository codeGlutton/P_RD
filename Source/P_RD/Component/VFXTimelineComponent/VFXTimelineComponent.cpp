#include "Component/VFXTimelineComponent/VFXTimelineComponent.h"

#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "Curves/CurveLinearColor.h"

#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTargetView.h"
#include "Animation/BoardActorAnimType.h"

#include "NiagaraComponent.h"
#include "FunctionLibrary/VFXFunctionLibrary.h"

#include "Setting/GamePlaySettings.h"

#include "Net/UnrealNetwork.h"

UVFXTimelineComponent::UVFXTimelineComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UVFXTimelineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (mIgnoreTimeDilation == true)
	{
		DeltaTime = FApp::GetDeltaTime();
		const UWorld* World = GetWorld();
		if (const AWorldSettings* WorldSettings = World ? World->GetWorldSettings() : nullptr)
		{
			DeltaTime = FMath::Clamp(DeltaTime, WorldSettings->MinUndilatedFrameTime, WorldSettings->MaxUndilatedFrameTime);
		}
	}

	for (FTimelineEntry& TimelineEntry: mTimelineEntries)
	{
		TimelineEntry.mTimeline.TickTimeline(DeltaTime);
	}

	if (IsNetSimulating() == false)
	{
		// 재생 중인 타임라인이 없다면 Component 비활성화
		if (IsAnyPlaying() == false)
		{
			Deactivate();
		}
	}
}

void UVFXTimelineComponent::Activate(bool Reset)
{
	Super::Activate(Reset);
	PrimaryComponentTick.SetTickFunctionEnable(true);
}

void UVFXTimelineComponent::Deactivate()
{
	Super::Deactivate();
	PrimaryComponentTick.SetTickFunctionEnable(false);
}

bool UVFXTimelineComponent::IsReadyForOwnerToAutoDestroy() const
{
	return IsAnyPlaying() == false;
}

bool UVFXTimelineComponent::IsPostLoadThreadSafe() const
{
	return true;
}

bool UVFXTimelineComponent::Play(const FName& KeyName, FVFXTimelineEventTarget Target)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}

	Activate();
	
	mTimelineTargets.Remove(KeyName);
	mTimelineTargets.Add(KeyName, MoveTemp(Target));
	TimelineEntry->mTimeline.Play();
	return true;
}

bool UVFXTimelineComponent::PlayFromStart(const FName& KeyName, FVFXTimelineEventTarget Target)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}

	Activate();

	mTimelineTargets.Remove(KeyName);
	mTimelineTargets.Add(KeyName, MoveTemp(Target));
	TimelineEntry->mTimeline.PlayFromStart();
	return true;
}

bool UVFXTimelineComponent::Reverse(const FName& KeyName, FVFXTimelineEventTarget Target)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}

	Activate();

	mTimelineTargets.Remove(KeyName);
	mTimelineTargets.Add(KeyName, MoveTemp(Target));
	TimelineEntry->mTimeline.Reverse();
	return true;
}

bool UVFXTimelineComponent::ReverseFromEnd(const FName& KeyName, FVFXTimelineEventTarget Target)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}

	Activate();

	mTimelineTargets.Remove(KeyName);
	mTimelineTargets.Add(KeyName, MoveTemp(Target));
	TimelineEntry->mTimeline.ReverseFromEnd();
	return true;
}

int32 UVFXTimelineComponent::StopAll()
{
	int32 Count = 0;
	for (FTimelineEntry& TimelineEntry : mTimelineEntries)
	{
		if (TimelineEntry.mTimeline.IsPlaying() == true)
		{
			TimelineEntry.mTimeline.Stop();
			++Count;
		}
	}
	return Count;
}

bool UVFXTimelineComponent::Stop(const FName& KeyName)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}

	TimelineEntry->mTimeline.Stop();
	return true;
}

bool UVFXTimelineComponent::IsAnyPlaying() const
{
	bool IsAnyTimelineActive = false;
	for (const FTimelineEntry& TimelineEntry : mTimelineEntries)
	{
		if (TimelineEntry.mTimeline.IsPlaying() == true)
		{
			IsAnyTimelineActive = true;
		}
	}
	return IsAnyTimelineActive;
}

bool UVFXTimelineComponent::IsPlaying(const FName& KeyName) const
{
	const FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}
	return TimelineEntry->mTimeline.IsPlaying();
}

bool UVFXTimelineComponent::IsReversing(const FName& KeyName) const
{
	const FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}
	return TimelineEntry->mTimeline.IsReversing();
}

bool UVFXTimelineComponent::SetPlaybackPosition(const FName& KeyName, float NewPosition, bool FireEvents, bool FireUpdate)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}
	TimelineEntry->mTimeline.SetPlaybackPosition(NewPosition, FireEvents, FireUpdate);
	return true;
}

float UVFXTimelineComponent::GetPlaybackPosition(const FName& KeyName) const
{
	const FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return 0.f;
	}
	return TimelineEntry->mTimeline.GetPlaybackPosition();
}

bool UVFXTimelineComponent::SetLooping(const FName& KeyName, bool NewLooping)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}
	TimelineEntry->mTimeline.SetLooping(NewLooping);
	return true;
}

bool UVFXTimelineComponent::IsLooping(const FName& KeyName) const
{
	const FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}
	return TimelineEntry->mTimeline.IsLooping();
}

bool UVFXTimelineComponent::SetPlayRate(const FName& KeyName, float NewRate)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}
	TimelineEntry->mTimeline.SetPlayRate(NewRate);
	return true;
}

float UVFXTimelineComponent::GetPlayRate(const FName& KeyName) const
{
	const FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return 0.f;
	}
	return TimelineEntry->mTimeline.GetPlayRate();
}

bool UVFXTimelineComponent::SetNewTime(const FName& KeyName, float NewTime)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}
	TimelineEntry->mTimeline.SetNewTime(NewTime);
	return true;
}

float UVFXTimelineComponent::GetTimelineLength(const FName& KeyName) const
{
	const FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return 0.f;
	}
	return TimelineEntry->mTimeline.GetTimelineLength();
}

float UVFXTimelineComponent::GetScaledTimelineLength(const FName& KeyName) const
{
	const FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return 0.f;
	}
	return TimelineEntry->mTimeline.GetScaledTimelineLength();
}

bool UVFXTimelineComponent::SetTimelineLength(const FName& KeyName, float NewLength)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}
	TimelineEntry->mTimeline.SetTimelineLength(NewLength);
	return true;
}

bool UVFXTimelineComponent::SetTimelineLengthMode(const FName& KeyName, ETimelineLengthMode NewLengthMode)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}
	TimelineEntry->mTimeline.SetTimelineLengthMode(NewLengthMode);
	return true;
}

void UVFXTimelineComponent::SetIgnoreTimeDilation(bool NewIgnoreTimeDilation)
{
	mIgnoreTimeDilation = NewIgnoreTimeDilation;
}

bool UVFXTimelineComponent::GetIgnoreTimeDilation() const
{
	return mIgnoreTimeDilation;
}

void UVFXTimelineComponent::AddInterpFloat(const FName& KeyName, UCurveFloat* FloatCurve, FVFXTimelineTrackEvent Event)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		TimelineEntry = AddTimelineEntry(KeyName);
	}

	FOnTimelineFloatStatic InterpFunc;
	InterpFunc.BindWeakLambda(this, [this, KeyName, MovedEvent = MoveTemp(Event)](float Value) {
		MovedEvent.Trigger(Value, mTimelineTargets[KeyName].mMeshComps, mTimelineTargets[KeyName].mNiagaraComps);
		});
	TimelineEntry->mTimeline.AddInterpFloat(FloatCurve, InterpFunc);
}

void UVFXTimelineComponent::AddInterpVector(const FName& KeyName, UCurveVector* VectorCurve, FVFXTimelineTrackEvent Event)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		TimelineEntry = AddTimelineEntry(KeyName);
	}

	FOnTimelineVectorStatic InterpFunc;
	InterpFunc.BindWeakLambda(this, [this, KeyName, MovedEvent = MoveTemp(Event)](FVector Value) {
		MovedEvent.Trigger(Value, mTimelineTargets[KeyName].mMeshComps, mTimelineTargets[KeyName].mNiagaraComps);
		});
	TimelineEntry->mTimeline.AddInterpVector(VectorCurve, InterpFunc);
}

void UVFXTimelineComponent::AddInterpLinearColor(const FName& KeyName, UCurveLinearColor* LinearColorCurve, FVFXTimelineTrackEvent Event)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		TimelineEntry = AddTimelineEntry(KeyName);
	}

	FOnTimelineLinearColorStatic InterpFunc;
	InterpFunc.BindWeakLambda(this, [this, KeyName, MovedEvent = MoveTemp(Event)](FLinearColor Value) {
		MovedEvent.Trigger(Value, mTimelineTargets[KeyName].mMeshComps, mTimelineTargets[KeyName].mNiagaraComps);
		});
	TimelineEntry->mTimeline.AddInterpLinearColor(LinearColorCurve, InterpFunc);
}

bool UVFXTimelineComponent::SetPropertySetObject(const FName& KeyName, UObject* NewPropertySetObject)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}
	TimelineEntry->mTimeline.SetPropertySetObject(NewPropertySetObject);
	return true;
}

bool UVFXTimelineComponent::SetTimelinePostUpdateFunc(const FName& KeyName, FOnTimelineEvent NewTimelinePostUpdateFunc)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}
	TimelineEntry->mTimeline.SetTimelinePostUpdateFunc(NewTimelinePostUpdateFunc);
	return true;
}

bool UVFXTimelineComponent::SetTimelineFinishedFunc(const FName& KeyName, FOnTimelineEvent NewTimelineFinishedFunc)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}
	TimelineEntry->mTimeline.SetTimelineFinishedFunc(NewTimelineFinishedFunc);
	return true;
}

bool UVFXTimelineComponent::SetTimelineFinishedFunc(const FName& KeyName, FOnTimelineEventStatic NewTimelineFinishedFunc)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}
	TimelineEntry->mTimeline.SetTimelineFinishedFunc(NewTimelineFinishedFunc);
	return true;
}

bool UVFXTimelineComponent::SetDirectionPropertyName(const FName& KeyName, FName DirectionPropertyName)
{
	FTimelineEntry* TimelineEntry = FindTimelineEntry(KeyName);
	if (TimelineEntry == nullptr)
	{
		return false;
	}
	TimelineEntry->mTimeline.SetDirectionPropertyName(DirectionPropertyName);
	return true;
}

FTimelineEntry* UVFXTimelineComponent::AddTimelineEntry(const FName& KeyName)
{
	FTimelineEntry Entry;
	Entry.mKeyName = KeyName;

	mTimelineEntries.Add(MoveTemp(Entry));
	return &mTimelineEntries.Last();
}

FTimelineEntry* UVFXTimelineComponent::FindTimelineEntry(const FName& KeyName)
{
	for (FTimelineEntry& TimelineEntry : mTimelineEntries)
	{
		if (TimelineEntry.mKeyName == KeyName)
		{
			return &TimelineEntry;
		}
	}
	return nullptr;
}

const FTimelineEntry* UVFXTimelineComponent::FindTimelineEntry(const FName& KeyName) const
{
	for (const FTimelineEntry& TimelineEntry : mTimelineEntries)
	{
		if (TimelineEntry.mKeyName == KeyName)
		{
			return &TimelineEntry;
		}
	}
	return nullptr;
}

void UCombatTargetVFXTimelineComponent::BindOwnerModel(UObjectModel* Model)
{
	UBoardActorModel* BoardActorModel = Cast<UBoardActorModel>(Model);
	if (BoardActorModel == nullptr)
	{
		return;
	}

	mOwnerModel = BoardActorModel;

	/* 연출 커브 추가 */

	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
	for (const FCombatTargetVFXTimelineSetting& CombatTargetVFXTimelineSetting : GamePlaySettings->mCombatTargetVFXTimelineSettings)
	{
		UCurveBase* Curve = CombatTargetVFXTimelineSetting.mTimelineCurve.LoadSynchronous();
		if (UCurveFloat* FloatCurve = Cast<UCurveFloat>(Curve))
		{
			AddInterpFloat(CombatTargetVFXTimelineSetting.mTimelineKeyName, FloatCurve, CombatTargetVFXTimelineSetting.mTimelineEvent);
		}
		else if (UCurveVector* VectorCurve = Cast<UCurveVector>(Curve))
		{
			AddInterpVector(CombatTargetVFXTimelineSetting.mTimelineKeyName, VectorCurve, CombatTargetVFXTimelineSetting.mTimelineEvent);
		}
		else if (UCurveLinearColor* ColorCurve = Cast<UCurveLinearColor>(Curve))
		{
			AddInterpLinearColor(CombatTargetVFXTimelineSetting.mTimelineKeyName, ColorCurve, CombatTargetVFXTimelineSetting.mTimelineEvent);
		}
	}

	/* Remove VFX 연결 */

	if (mOwnerModel.IsValid() == true)
	{
		mOwnerModel->OnRemoveTileTransform.AddUObject(this, &UCombatTargetVFXTimelineComponent::PlayRemoveVFX);
	}
}

void UCombatTargetVFXTimelineComponent::UnbindOwnerModel(UObjectModel* Model)
{
	if (mOwnerModel.IsValid() == true)
	{
		mOwnerModel->OnRemoveTileTransform.RemoveAll(this);
	}
}

void UCombatTargetVFXTimelineComponent::PlayRemoveVFX()
{
	/* 타겟 메시 채우기 */

	FVFXTimelineEventTarget EventTarget;
	UPrimitiveComponent* TargetMeshComp = nullptr;
	if (MakeTargetMeshEventTarget(OUT EventTarget, OUT TargetMeshComp) == false)
	{
		return;
	}

	/* 실행 */

	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
	UVFXFunctionLibrary::SpawnAndExecuteVFX(GamePlaySettings->mCombatTargetRemoveVFX, TargetMeshComp, this, EventTarget);
}

bool UCombatTargetVFXTimelineComponent::MakeTargetMeshEventTarget(OUT FVFXTimelineEventTarget& EventTarget, OUT UPrimitiveComponent*& TargetMeshComp) const
{
	IBoardCombatTargetView* BoardCombatTargetView = Cast<IBoardCombatTargetView>(GetOwner());
	if (BoardCombatTargetView == nullptr)
	{
		return false;
	}

	/* 본체 메시와 거기 붙어 있는 자식 메시(무기/장비 등)를 모두 담음 */

	TargetMeshComp = BoardCombatTargetView->GetTargetMeshComponent();
	EventTarget = UVFXFunctionLibrary::MakeTimelineEventTarget(TargetMeshComp);

	return true;
}

