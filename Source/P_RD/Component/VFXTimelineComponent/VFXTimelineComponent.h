/*****************************************************************//**
 * @file   VFXTimelineComponent.h
 * @brief  메시 Custom Primitive Data(CPD)와 Niagara Effect를 타임라인 커브에 맞춰 연동 제어하는 컴포넌트
 * @author 모호재
 * @date   2026-08-06
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Component/ComponentView.h"
#include "Components/TimelineComponent.h"
#include "Component/VFXTimelineComponent/VFXTimelineTrackEvent.h"
#include "VFXTimelineComponent.generated.h"

class UPrimitiveComponent;
class UNiagaraComponent;

class UBoardActorModel;

DECLARE_DELEGATE_TwoParams(FOnVFXTimelineFloatStatic, const FVFXTimelineTrackEvent&, float);
DECLARE_DELEGATE_TwoParams(FOnVFXTimelineVectorStatic, const FVFXTimelineTrackEvent& , FVector);
DECLARE_DELEGATE_TwoParams(FOnVFXTimelineLinearColorStatic, const FVFXTimelineTrackEvent&, FLinearColor);

/**
 * @brief 메시 CPD와 Niagara 파라미터를 타임라인에 커플링하여 실행해주는 컴포넌트
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class P_RD_API UVFXTimelineComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVFXTimelineComponent();

	/* UTimelineComponent 상속 */
public:
	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void Activate(bool Reset = false) override;
	void Deactivate() override;
	bool IsReadyForOwnerToAutoDestroy() const override;
	bool IsPostLoadThreadSafe() const override;
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/* 타임라인 조정 */
public:
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void Play();
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void PlayFromStart();

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void Reverse();
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void ReverseFromEnd();

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void Stop();

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool IsPlaying() const;
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool IsReversing() const;

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline", meta = (AdvancedDisplay = "FireUpdate"))
	void SetPlaybackPosition(float NewPosition, bool FireEvents, bool FireUpdate = true);
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	float GetPlaybackPosition() const;

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void SetLooping(bool NewLooping);
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool IsLooping() const;

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void SetPlayRate(float NewRate);
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	float GetPlayRate() const;

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void SetNewTime(float NewTime);

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	float GetTimelineLength() const;
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	float GetScaledTimelineLength() const;
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void SetTimelineLength(float NewLength);

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void SetTimelineLengthMode(ETimelineLengthMode NewLengthMode);

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void SetIgnoreTimeDilation(bool NewIgnoreTimeDilation);
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool GetIgnoreTimeDilation() const;

	/* 커브 데이터 수정 */
public:
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void SetFloatCurve(UCurveFloat* NewFloatCurve, FName FloatTrackName);
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void SetVectorCurve(UCurveVector* NewVectorCurve, FName VectorTrackName);
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void SetLinearColorCurve(UCurveLinearColor* NewLinearColorCurve, FName LinearColorTrackName);

public:
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void AddEvent(float Time, FOnTimelineEvent EventFunc);
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void AddInterpFloat(UCurveFloat* FloatCurve, FOnTimelineFloat InterpFunc, FName PropertyName = NAME_None, FName TrackName = NAME_None);
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void AddInterpVector(UCurveVector* VectorCurve, FOnTimelineVector InterpFunc, FName PropertyName = NAME_None, FName TrackName = NAME_None);
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void AddInterpLinearColor(UCurveLinearColor* LinearColorCurve, FOnTimelineLinearColor InterpFunc, FName PropertyName = NAME_None, FName TrackName = NAME_None);

	void AddInterpFloat(UCurveFloat* FloatCurve, FOnTimelineFloatStatic InterpFunc);
	void AddInterpVector(UCurveVector* VectorCurve, FOnTimelineVectorStatic InterpFunc);
	void AddInterpLinearColor(UCurveLinearColor* LinearColorCurve, FOnTimelineLinearColorStatic InterpFunc);

	void AddVFXInterpFloat(UCurveFloat* FloatCurve, FVFXTimelineTrackEvent Event, FOnVFXTimelineFloatStatic VFXInterpFunc);
	void AddVFXInterpVector(UCurveVector* VectorCurve, FVFXTimelineTrackEvent Event, FOnVFXTimelineVectorStatic VFXInterpFunc);
	void AddVFXInterpLinearColor(UCurveLinearColor* LinearColorCurve, FVFXTimelineTrackEvent Event, FOnVFXTimelineLinearColorStatic VFXInterpFunc);

	void SetPropertySetObject(UObject* NewPropertySetObject);

	/* 대리자 등록 */
public:
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void SetTimelinePostUpdateFunc(FOnTimelineEvent NewTimelinePostUpdateFunc);

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void SetTimelineFinishedFunc(FOnTimelineEvent NewTimelineFinishedFunc);
	void SetTimelineFinishedFunc(FOnTimelineEventStatic NewTimelineFinishedFunc);

	void SetDirectionPropertyName(FName DirectionPropertyName);

	/* 멀티 복제 */
public:
	UFUNCTION()
	void OnRep_Timeline(FTimeline& OldTimeline);

	/* 리플렉션 */
public:
	static UFunction* GetTimelineEventSignature();
	static UFunction* GetTimelineFloatSignature();
	static UFunction* GetTimelineVectorSignature();
	static UFunction* GetTimelineLinearColorSignature();

	static ETimelineSigType GetTimelineSignatureForFunction(const UFunction* InFunc);

private:
	UPROPERTY(ReplicatedUsing = OnRep_Timeline)
	FTimeline mTimeline;

	UPROPERTY()
	uint32 mIgnoreTimeDilation : 1;
};

/**
 * @brief 메시 CPD와 Niagara 파라미터를 타임라인에 커플링하여 실행해주는 컴포넌트
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class P_RD_API UDissolveVFXTimelineComponent : public UVFXTimelineComponent, public IComponentView
{
	GENERATED_BODY()

	/* IComponentView 상속 */
public:
	void BindOwnerModel(UObjectModel* Model) override;
	void UnbindOwnerModel(UObjectModel* Model) override;

public:
	void Dissolve();

private:
	void OnUpdateDissolveVFX(const FVFXTimelineTrackEvent& Event, float Value) const;

private:
	static const FName DISSOLVE_PARAM_NAME;
	static const int32 DISSOLVE_PARAM_INDEX;

protected:
	// @brief 소유 모델 객체
	TWeakObjectPtr<UBoardActorModel> mOwnerModel = nullptr;

	// @brief VFX 캐시
	TArray<TWeakObjectPtr<UPrimitiveComponent>> mDissolveMeshCompCaches;
	TArray<TWeakObjectPtr<UNiagaraComponent>> mDissolveNiagaraCompCaches;
};
