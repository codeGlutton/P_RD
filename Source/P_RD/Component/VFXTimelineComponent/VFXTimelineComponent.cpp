#include "Component/VFXTimelineComponent/VFXTimelineComponent.h"

#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "Curves/CurveLinearColor.h"

#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTargetView.h"

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

	mTimeline.TickTimeline(DeltaTime);

	if (IsNetSimulating() == false)
	{
		if (mTimeline.IsPlaying() == false)
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
	return IsPlaying() == false;
}

bool UVFXTimelineComponent::IsPostLoadThreadSafe() const
{
	return true;
}

void UVFXTimelineComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UVFXTimelineComponent, mTimeline);
}

void UVFXTimelineComponent::Play()
{
	Activate();
	mTimeline.Play();
}

void UVFXTimelineComponent::PlayFromStart()
{
	Activate();
	mTimeline.PlayFromStart();
}

void UVFXTimelineComponent::Reverse()
{
	Activate();
	mTimeline.Reverse();
}

void UVFXTimelineComponent::ReverseFromEnd()
{
	Activate();
	mTimeline.ReverseFromEnd();
}

void UVFXTimelineComponent::Stop()
{
	mTimeline.Stop();
}

bool UVFXTimelineComponent::IsPlaying() const
{
	return mTimeline.IsPlaying();
}

bool UVFXTimelineComponent::IsReversing() const
{
	return mTimeline.IsReversing();
}

void UVFXTimelineComponent::SetPlaybackPosition(float NewPosition, bool FireEvents, bool FireUpdate)
{
	mTimeline.SetPlaybackPosition(NewPosition, FireEvents, FireUpdate);
}

float UVFXTimelineComponent::GetPlaybackPosition() const
{
	return mTimeline.GetPlaybackPosition();
}

void UVFXTimelineComponent::SetLooping(bool NewLooping)
{
	mTimeline.SetLooping(NewLooping);
}

bool UVFXTimelineComponent::IsLooping() const
{
	return mTimeline.IsLooping();
}

void UVFXTimelineComponent::SetPlayRate(float NewRate)
{
	mTimeline.SetPlayRate(NewRate);
}

float UVFXTimelineComponent::GetPlayRate() const
{
	return mTimeline.GetPlayRate();
}

void UVFXTimelineComponent::SetNewTime(float NewTime)
{
	mTimeline.SetNewTime(NewTime);
}

float UVFXTimelineComponent::GetTimelineLength() const
{
	return mTimeline.GetTimelineLength();
}

float UVFXTimelineComponent::GetScaledTimelineLength() const
{
	return mTimeline.GetScaledTimelineLength();
}

void UVFXTimelineComponent::SetTimelineLength(float NewLength)
{
	return mTimeline.SetTimelineLength(NewLength);
}

void UVFXTimelineComponent::SetTimelineLengthMode(ETimelineLengthMode NewLengthMode)
{
	mTimeline.SetTimelineLengthMode(NewLengthMode);
}

void UVFXTimelineComponent::SetIgnoreTimeDilation(bool NewIgnoreTimeDilation)
{
	mIgnoreTimeDilation = NewIgnoreTimeDilation;
}

bool UVFXTimelineComponent::GetIgnoreTimeDilation() const
{
	return mIgnoreTimeDilation;
}

void UVFXTimelineComponent::SetFloatCurve(UCurveFloat* NewFloatCurve, FName FloatTrackName)
{
	mTimeline.SetFloatCurve(NewFloatCurve, FloatTrackName);
}

void UVFXTimelineComponent::SetVectorCurve(UCurveVector* NewVectorCurve, FName VectorTrackName)
{
	mTimeline.SetVectorCurve(NewVectorCurve, VectorTrackName);
}

void UVFXTimelineComponent::SetLinearColorCurve(UCurveLinearColor* NewLinearColorCurve, FName LinearColorTrackName)
{
	mTimeline.SetLinearColorCurve(NewLinearColorCurve, LinearColorTrackName);
}

void UVFXTimelineComponent::AddEvent(float Time, FOnTimelineEvent Event)
{
	mTimeline.AddEvent(Time, Event);
}

void UVFXTimelineComponent::AddInterpFloat(UCurveFloat* FloatCurve, FOnTimelineFloat InterpFunc, FName PropertyName, FName TrackName)
{
	mTimeline.AddInterpFloat(FloatCurve, InterpFunc, PropertyName, TrackName);
}

void UVFXTimelineComponent::AddInterpVector(UCurveVector* VectorCurve, FOnTimelineVector InterpFunc, FName PropertyName, FName TrackName)
{
	mTimeline.AddInterpVector(VectorCurve, InterpFunc, PropertyName, TrackName);
}

void UVFXTimelineComponent::AddInterpLinearColor(UCurveLinearColor* LinearColorCurve, FOnTimelineLinearColor InterpFunc, FName PropertyName, FName TrackName)
{
	mTimeline.AddInterpLinearColor(LinearColorCurve, InterpFunc, PropertyName, TrackName);
}

void UVFXTimelineComponent::AddInterpFloat(UCurveFloat* FloatCurve, const FOnTimelineFloatStatic InterpFunc)
{
	mTimeline.AddInterpFloat(FloatCurve, InterpFunc);
}

void UVFXTimelineComponent::AddInterpVector(UCurveVector* VectorCurve, const FOnTimelineVectorStatic InterpFunc)
{
	mTimeline.AddInterpVector(VectorCurve, InterpFunc);
}

void UVFXTimelineComponent::AddInterpLinearColor(UCurveLinearColor* LinearColorCurve, const FOnTimelineLinearColorStatic InterpFunc)
{
	mTimeline.AddInterpLinearColor(LinearColorCurve, InterpFunc);
}

void UVFXTimelineComponent::AddVFXInterpFloat(UCurveFloat* FloatCurve, FVFXTimelineTrackEvent Event, FOnVFXTimelineFloatStatic VFXInterpFunc)
{
	FOnTimelineFloatStatic OnTimelineStatic;
	OnTimelineStatic.BindLambda([MovedEvent = MoveTemp(Event), MovedFunc = MoveTemp(VFXInterpFunc)](float Value) {
		MovedFunc.ExecuteIfBound(MovedEvent, Value);
		});

	AddInterpFloat(FloatCurve, MoveTemp(OnTimelineStatic));
}

void UVFXTimelineComponent::AddVFXInterpVector(UCurveVector* VectorCurve, FVFXTimelineTrackEvent Event, FOnVFXTimelineVectorStatic VFXInterpFunc)
{
	FOnTimelineVectorStatic OnTimelineStatic;
	OnTimelineStatic.BindLambda([MovedEvent = MoveTemp(Event), MovedFunc = MoveTemp(VFXInterpFunc)](FVector Value) {
		MovedFunc.ExecuteIfBound(MovedEvent, Value);
		});

	AddInterpVector(VectorCurve, MoveTemp(OnTimelineStatic));
}

void UVFXTimelineComponent::AddVFXInterpLinearColor(UCurveLinearColor* LinearColorCurve, FVFXTimelineTrackEvent Event, FOnVFXTimelineLinearColorStatic VFXInterpFunc)
{
	FOnTimelineLinearColorStatic OnTimelineStatic;
	OnTimelineStatic.BindLambda([MovedEvent = MoveTemp(Event), MovedFunc = MoveTemp(VFXInterpFunc)](FLinearColor Value) {
		MovedFunc.ExecuteIfBound(MovedEvent, Value);
		});

	AddInterpLinearColor(LinearColorCurve, MoveTemp(OnTimelineStatic));
}

void UVFXTimelineComponent::SetPropertySetObject(UObject* NewPropertySetObject)
{
	mTimeline.SetPropertySetObject(NewPropertySetObject);
}

void UVFXTimelineComponent::SetTimelinePostUpdateFunc(FOnTimelineEvent NewTimelinePostUpdateFunc)
{
	mTimeline.SetTimelinePostUpdateFunc(NewTimelinePostUpdateFunc);
}

void UVFXTimelineComponent::SetTimelineFinishedFunc(FOnTimelineEvent NewTimelineFinishedFunc)
{
	mTimeline.SetTimelineFinishedFunc(NewTimelineFinishedFunc);
}

void UVFXTimelineComponent::SetTimelineFinishedFunc(FOnTimelineEventStatic NewTimelineFinishedFunc)
{
	mTimeline.SetTimelineFinishedFunc(NewTimelineFinishedFunc);
}

void UVFXTimelineComponent::SetDirectionPropertyName(FName DirectionPropertyName)
{
	mTimeline.SetDirectionPropertyName(DirectionPropertyName);
}

void UVFXTimelineComponent::OnRep_Timeline(FTimeline& OldTimeline)
{
	if (mTimeline.IsPlaying() == false && OldTimeline.GetPlaybackPosition() != mTimeline.GetPlaybackPosition())
	{
		mTimeline.SetPlaybackPosition(mTimeline.GetPlaybackPosition(), false, true);
	}
}

UFunction* UVFXTimelineComponent::GetTimelineEventSignature()
{
	UFunction* TimelineEventSig = FindObject<UFunction>(FindPackage(nullptr, TEXT("/Script/Engine")), TEXT("OnTimelineEvent__DelegateSignature"));
	check(TimelineEventSig != NULL);
	return TimelineEventSig;
}

UFunction* UVFXTimelineComponent::GetTimelineFloatSignature()
{
	UFunction* TimelineFloatSig = FindObject<UFunction>(FindPackage(nullptr, TEXT("/Script/Engine")), TEXT("OnTimelineFloat__DelegateSignature"));
	check(TimelineFloatSig != NULL);
	return TimelineFloatSig;
}

UFunction* UVFXTimelineComponent::GetTimelineVectorSignature()
{
	UFunction* TimelineVectorSig = FindObject<UFunction>(FindPackage(nullptr, TEXT("/Script/Engine")), TEXT("OnTimelineVector__DelegateSignature"));
	check(TimelineVectorSig != NULL);
	return TimelineVectorSig;
}

UFunction* UVFXTimelineComponent::GetTimelineLinearColorSignature()
{
	UFunction* TimelineVectorSig = FindObject<UFunction>(FindPackage(nullptr, TEXT("/Script/Engine")), TEXT("OnTimelineLinearColor__DelegateSignature"));
	check(TimelineVectorSig != NULL);
	return TimelineVectorSig;
}

ETimelineSigType UVFXTimelineComponent::GetTimelineSignatureForFunction(const UFunction* Func)
{
	if (Func != NULL)
	{
		if (Func->IsSignatureCompatibleWith(GetTimelineEventSignature()))
		{
			return ETS_EventSignature;
		}
		else if (Func->IsSignatureCompatibleWith(GetTimelineFloatSignature()))
		{
			return ETS_FloatSignature;
		}
		else if (Func->IsSignatureCompatibleWith(GetTimelineVectorSignature()))
		{
			return ETS_VectorSignature;
		}
		else if (Func->IsSignatureCompatibleWith(GetTimelineLinearColorSignature()))
		{
			return ETS_LinearColorSignature;
		}
	}

	return ETS_InvalidSignature;
}

const FName UDissolveVFXTimelineComponent::DISSOLVE_PARAM_NAME = TEXT("User.Dissolve");
const int32 UDissolveVFXTimelineComponent::DISSOLVE_PARAM_INDEX = 0;

void UDissolveVFXTimelineComponent::BindOwnerModel(UObjectModel* Model)
{
	UBoardActorModel* BoardActorModel = Cast<UBoardActorModel>(Model);
	if (BoardActorModel == nullptr)
	{
		return;
	}

	mOwnerModel = BoardActorModel;

	/* 디졸브 연출 요청 대리자 구독 */

	FVFXTimelineTrackEvent TrackEvent;
	TrackEvent.mSyncTarget = StaticCast<int32>(EVFXTimelineSyncTarget::PrimitiveData | EVFXTimelineSyncTarget::NiagaraUserParameter);
	TrackEvent.mParameterName = DISSOLVE_PARAM_NAME;
	TrackEvent.mCPDIndex = DISSOLVE_PARAM_INDEX;

	FOnVFXTimelineFloatStatic InterpFunc;
	InterpFunc.BindUObject(this, &UDissolveVFXTimelineComponent::OnUpdateDissolveVFX);

	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
	AddVFXInterpFloat(GamePlaySettings->mCombatTargetDissolveVFXSetting.mCombatTargetDissolveCurve.LoadSynchronous(), MoveTemp(TrackEvent), InterpFunc);

	if (mOwnerModel.IsValid() == true)
	{
		mOwnerModel->OnRemoveTileTransform.AddUObject(this, &UDissolveVFXTimelineComponent::Dissolve);
	}
}

void UDissolveVFXTimelineComponent::UnbindOwnerModel(UObjectModel* Model)
{
}

void UDissolveVFXTimelineComponent::Dissolve()
{
	IBoardCombatTargetView* BoardCombatTargetView = Cast<IBoardCombatTargetView>(GetOwner());
	if (BoardCombatTargetView == nullptr)
	{
		return;
	}

	mDissolveMeshCompCaches.Empty(1);
	mDissolveNiagaraCompCaches.Empty(1);

	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
	UNiagaraComponent* SpawnedNiagaraComp = UVFXFunctionLibrary::SpawnNiagaraEffect(
		GamePlaySettings->mCombatTargetDissolveVFXSetting.mCombatTargetDissolveVFX, 
		BoardCombatTargetView->GetTargetMeshComponent()
	);

	mDissolveMeshCompCaches.Add(BoardCombatTargetView->GetTargetMeshComponent());
	mDissolveNiagaraCompCaches.Add(SpawnedNiagaraComp);

	PlayFromStart();
}

void UDissolveVFXTimelineComponent::OnUpdateDissolveVFX(const FVFXTimelineTrackEvent& Event, float Value) const
{
	Event.Trigger(Value, mDissolveMeshCompCaches, mDissolveNiagaraCompCaches);
}
