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
class IBoardCombatTargetView;

USTRUCT(BlueprintType)
struct FTimelineEntry
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName mKeyName;

	UPROPERTY()
	FTimeline mTimeline;
};

/**
 * @brief 메시 CPD와 Niagara 파라미터를 타임라인에 커플링하여 실행해주는 컴포넌트
 */
UCLASS(ClassGroup = (VFXTimeline), meta = (BlueprintSpawnableComponent))
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

	/* 타임라인 조정 */
public:
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool Play(const FName& KeyName, FVFXTimelineEventTarget Target);
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool PlayFromStart(const FName& KeyName, FVFXTimelineEventTarget Target);

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool Reverse(const FName& KeyName, FVFXTimelineEventTarget Target);
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool ReverseFromEnd(const FName& KeyName, FVFXTimelineEventTarget Target);

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	int32 StopAll();
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool Stop(const FName& KeyName);

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool IsAnyPlaying() const;
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool IsPlaying(const FName& KeyName) const;
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool IsReversing(const FName& KeyName) const;

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline", meta = (AdvancedDisplay = "FireUpdate"))
	bool SetPlaybackPosition(const FName& KeyName, float NewPosition, bool FireEvents, bool FireUpdate = true);
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	float GetPlaybackPosition(const FName& KeyName) const;

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool SetLooping(const FName& KeyName, bool NewLooping);
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool IsLooping(const FName& KeyName) const;

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool SetPlayRate(const FName& KeyName, float NewRate);
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	float GetPlayRate(const FName& KeyName) const;

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool SetNewTime(const FName& KeyName, float NewTime);

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	float GetTimelineLength(const FName& KeyName) const;
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	float GetScaledTimelineLength(const FName& KeyName) const;
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool SetTimelineLength(const FName& KeyName, float NewLength);

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool SetTimelineLengthMode(const FName& KeyName, ETimelineLengthMode NewLengthMode);

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	void SetIgnoreTimeDilation(bool NewIgnoreTimeDilation);
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool GetIgnoreTimeDilation() const;

	/* 커브 등록 */
public:
	void AddInterpFloat(const FName& KeyName, UCurveFloat* FloatCurve, FVFXTimelineTrackEvent Event);
	void AddInterpVector(const FName& KeyName, UCurveVector* VectorCurve, FVFXTimelineTrackEvent Event);
	void AddInterpLinearColor(const FName& KeyName, UCurveLinearColor* LinearColorCurve, FVFXTimelineTrackEvent Event);

	bool SetPropertySetObject(const FName& KeyName, UObject* NewPropertySetObject);

	/* 대리자 등록 */
public:
	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool SetTimelinePostUpdateFunc(const FName& KeyName, FOnTimelineEvent NewTimelinePostUpdateFunc);

	UFUNCTION(BlueprintCallable, Category = "Components|VFXTimeline")
	bool SetTimelineFinishedFunc(const FName& KeyName, FOnTimelineEvent NewTimelineFinishedFunc);
	bool SetTimelineFinishedFunc(const FName& KeyName, FOnTimelineEventStatic NewTimelineFinishedFunc);

	bool SetDirectionPropertyName(const FName& KeyName, FName DirectionPropertyName);

protected:
	FTimelineEntry* AddTimelineEntry(const FName& KeyName);
	FTimelineEntry* FindTimelineEntry(const FName& KeyName);
	const FTimelineEntry* FindTimelineEntry(const FName& KeyName) const;

private:
	UPROPERTY()
	TArray<FTimelineEntry> mTimelineEntries;
	TMap<FName, FVFXTimelineEventTarget> mTimelineTargets;

	UPROPERTY()
	uint32 mIgnoreTimeDilation : 1;
};

/**
 * @brief 전투 대상 VFX 이펙트를 실행해주는 컴포넌트
 */
UCLASS(ClassGroup = (VFXTimeline), meta = (BlueprintSpawnableComponent))
class P_RD_API UCombatTargetVFXTimelineComponent : public UVFXTimelineComponent, public IComponentView
{
	GENERATED_BODY()

	/* IComponentView 상속 */
public:
	void BindOwnerModel(UObjectModel* Model) override;
	void UnbindOwnerModel(UObjectModel* Model) override;

protected:
	void PlayRemoveVFX();

protected:
	// @brief 소유 모델 객체
	TWeakObjectPtr<UBoardActorModel> mOwnerModel = nullptr;
};

