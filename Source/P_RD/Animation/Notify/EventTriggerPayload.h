/*****************************************************************//**
 * @file   EventTriggerPayload.h
 * @brief  이벤트 호출 애님 노티파이의 추가 데이터 정의 헤더
 * @author 모호재
 * @date   2026-07-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Setting/GamePlayType.h"
#include "EventTriggerPayload.generated.h"

class UCameraShakeBase;

DECLARE_DELEGATE(FOnEndDurationEventTrigger);

UENUM(BlueprintType)
enum class ECameraZoomTargetType : uint8
{
    SelfActor,
    SelfLocation,
    TargetLocation,
    AllLocation,
};

USTRUCT(BlueprintType)
struct FCameraZoomEventData
{
    GENERATED_BODY()

public:
    void CalculateCameraZoomScaleAndLocation(const TArray<FVector>& Locations, OUT float& OutZoomScale, OUT FVector& OutLocation) const;

public:
    UPROPERTY(Category = "Camera", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TargetType"))
    ECameraZoomTargetType mTargetType = ECameraZoomTargetType::SelfActor;

    UPROPERTY(Category = "Camera", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "WorldLocationOffset", EditCondition = "mTargetType!=ECameraZoomTargetType::SelfActor", EditConditionHides))
    FVector mWorldLocationOffset = FVector::ZeroVector;

    UPROPERTY(Category = "Camera", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "ZoomDefaultScale"))
    float mZoomDefaultScale = 1250.f;
    UPROPERTY(Category = "Camera", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "ZoomScaleRatio"))
    float mZoomScaleRatio = 0.5f;

    UPROPERTY(Category = "Camera", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "MinZoomScale"))
    float mMinZoomScale = 1250.f;
    UPROPERTY(Category = "Camera", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "MaxZoomScale"))
    float mMaxZoomScale = 2000.f;
};

/**
 * @brief  이벤트 호출 애님 노티파이의 추가 데이터 베이스
 */
USTRUCT(BlueprintType)
struct FEventTriggerPayloadBase
{
    GENERATED_BODY()

public:
    virtual ~FEventTriggerPayloadBase() = default;

public:
    virtual UScriptStruct* GetScriptStruct() const
    {
        return FEventTriggerPayloadBase::StaticStruct();
    }

    virtual FLinearColor GetEditorColor() const
    {
        return FColor(200, 200, 200, 255);
    }
};

/**
 * @brief  이벤트 호출 애님 노티파이의 추가 데이터
 */
USTRUCT(BlueprintType)
struct FEventTriggerPayload : public FEventTriggerPayloadBase
{
    GENERATED_BODY()

public:
    UScriptStruct* GetScriptStruct() const override
    {
        return FEventTriggerPayload::StaticStruct();
    }
};

/**
 * @brief 시간을 가지는 이벤트 호출 애님 노티파이의 추가 데이터
 */
USTRUCT(BlueprintType)
struct FDurationEventTriggerPayload : public FEventTriggerPayloadBase
{
    GENERATED_BODY()

public:
    UScriptStruct* GetScriptStruct() const override
    {
        return FDurationEventTriggerPayload::StaticStruct();
    }

public:
    mutable FOnEndDurationEventTrigger OnEndDurationEventTrigger;
};

/**
 * @brief  적용 스킬 로직 이벤트 호출 애님 노티파이의 추가 데이터
 */
USTRUCT(BlueprintType)
struct FApplySkillEventTriggerPayload : public FEventTriggerPayload
{
    GENERATED_BODY()

public:
    UScriptStruct* GetScriptStruct() const override
    {
        return FApplySkillEventTriggerPayload::StaticStruct();
    }

    FLinearColor GetEditorColor() const override
    {
        return FColor(245, 150, 40, 255);
    }
};

/**
 * @brief  적용 애님 이벤트 호출 애님 노티파이의 추가 데이터
 */
USTRUCT(BlueprintType)
struct FApplyAnimationEventTriggerPayload : public FEventTriggerPayload
{
    GENERATED_BODY()

public:
    UScriptStruct* GetScriptStruct() const override
    {
        return FApplyAnimationEventTriggerPayload::StaticStruct();
    }

    FLinearColor GetEditorColor() const override
    {
        return FColor(170, 90, 240, 255);
    }

public:
    UPROPERTY(Category = "Animation", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "AnimationTag"))
    FGameplayTag mAnimationTag;
};

UENUM(BlueprintType)
enum class EApplyNiagaraTargetType : uint8
{
    Actor,
    EffectTile,
    TargetTile,
};

/**
 * @brief  적용 VFX 이벤트 호출 애님 노티파이의 추가 데이터
 */
USTRUCT(BlueprintType)
struct FApplyVFXEventTriggerPayload : public FEventTriggerPayload
{
    GENERATED_BODY()

public:
    UScriptStruct* GetScriptStruct() const override
    {
        return FApplyVFXEventTriggerPayload::StaticStruct();
    }

    FLinearColor GetEditorColor() const override
    {
        return FColor(60, 220, 150, 255);
    }

public:
    UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TargetType"))
    EApplyNiagaraTargetType mTargetType = EApplyNiagaraTargetType::Actor;

    UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "VFXSpawnData"))
    FVFXSpawnData mVFXSpawnData;
};

/**
 * @brief  카메라 쉐이크 애님 노티파이의 추가 데이터
 */
USTRUCT(BlueprintType)
struct FCameraShakeEventTriggerPayload : public FEventTriggerPayload
{
    GENERATED_BODY()

public:
    UScriptStruct* GetScriptStruct() const override
    {
        return FCameraShakeEventTriggerPayload::StaticStruct();
    }

    FLinearColor GetEditorColor() const override
    {
        return FColor(235, 75, 75, 255);
    }

public:
    UPROPERTY(Category = "Camera", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "CameraShakeClass"))
    TSubclassOf<UCameraShakeBase> mCameraShakeClass;
};

/**
 * @brief  카메라 줌 애님 노티파이 스테이트의 추가 데이터
 */
USTRUCT(BlueprintType)
struct FCameraZoomEventTriggerPayload : public FDurationEventTriggerPayload
{
    GENERATED_BODY()

public:
    UScriptStruct* GetScriptStruct() const override
    {
        return FCameraZoomEventTriggerPayload::StaticStruct();
    }

    FLinearColor GetEditorColor() const override
    {
        return FColor(50, 200, 230, 255);
    }

public:
    UPROPERTY(Category = "Camera", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "CameraZoomEventData"))
    FCameraZoomEventData mCameraZoomEventData;
};

/**
 * @brief  시간 속도 조정 애님 노티파이 스테이트의 추가 데이터
 */
USTRUCT(BlueprintType)
struct FTimeScaleEventTriggerPayload : public FDurationEventTriggerPayload
{
    GENERATED_BODY()

public:
    UScriptStruct* GetScriptStruct() const override
    {
        return FTimeScaleEventTriggerPayload::StaticStruct();
    }

    FLinearColor GetEditorColor() const override
    {
        return FColor(240, 90, 150, 255);
    }

public:
    UPROPERTY(Category = "Time", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TimeScale"))
    float mTimeScale = 1.f;
    UPROPERTY(Category = "Time", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "BlendSpeed"))
    float mBlendSpeed = 3.f;
};

