/*****************************************************************//**
 * @file   ActiveTacticalEffect.h
 * @brief  현재 활성화 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-06-23
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "TAS/Effect/TacticalEffect.h"
#include "ActiveTacticalEffect.generated.h"

class UAttributeSetComponentModel;
struct FTacticalEffectRemovalInfo;

/**
 * @brief 활성화된 이펙트를 쉽게 탐색하기 위한 핸들
 */
USTRUCT()
struct FActiveTacticalEffectHandle
{
    GENERATED_BODY()

public:
    FActiveTacticalEffectHandle() = default;
    FActiveTacticalEffectHandle(int32 Index, UWorld* World);

public:
    /**
     * @brief 새 활성 이펙트용 핸들을 발급하고 전역 맵에 등록한다.
     * @param World TacticalFramework 서브시스템 모델을 탐색하기 위한 World 객체
     * @param OwningModel 이 이펙트를 소유한 AttributeSetComponentModel
     * @return 새로 발급된 유효 핸들
     */
    static FActiveTacticalEffectHandle GenerateNewHandle(UWorld* World, UAttributeSetComponentModel* OwningModel);

    /**
     * @brief 전역 핸들 맵을 초기화
     * @param World TacticalFramework 서브시스템 모델을 탐색하기 위한 World 객체
     */
    static void ResetGlobalHandleMap(UWorld* World);

public:
    /**
     * @brief 이 핸들이 가리키는 이펙트를 소유한 AttributeSetComponentModel 을 전역 맵에서 역추적
     * @return 소유 모델 포인터. 핸들이 무효하거나 이미 제거되었으면 nullptr.
     */
    UAttributeSetComponentModel* GetOwningAttributeSetComponentModel() const;

    /**
     * @brief 이 핸들 엔트리를 전역 맵에서 제거
     */
    void RemoveFromGlobalMap();

public:
    bool operator==(const FActiveTacticalEffectHandle& Other) const
    {
        return mIndex == Other.mIndex;
    }

    bool operator!=(const FActiveTacticalEffectHandle& Other) const
    {
        return mIndex != Other.mIndex;
    }

    friend uint32 GetTypeHash(const FActiveTacticalEffectHandle& Other)
    {
        return Other.mIndex;
    }

    bool IsValid() const
    {
        return mIndex != INDEX_NONE;
    }

    void Reset()
    {
        mIndex = INDEX_NONE;
    }

private:
    // @brief 전역 핸들 맵에서의 인덱스(키). INDEX_NONE = 무효.
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "Index"))
    int32 mIndex = INDEX_NONE;

    // @brief 핸들이 속한 월드
    UPROPERTY(Category = "World", VisibleAnywhere, meta = (DisplayName = "World"))
    TWeakObjectPtr<UWorld> mWorld;
};

/**
 * @brief 평가가 끝난(=실제 적용 직전 상태의) 단일 모디파이어 데이터
 */
USTRUCT(BlueprintType)
struct FTacticalModifierEvaluatedData
{
    GENERATED_USTRUCT_BODY()

    FTacticalModifierEvaluatedData() :
        mAttribute(),
        mModifierOp(ETacticalModOp::AddBase),
        mMagnitude(0.f),
        mIsValid(false)
    {
    }

    FTacticalModifierEvaluatedData(const FTacticalAttribute& InAttribute, TEnumAsByte<ETacticalModOp::Type> InModOp, float InMagnitude, FActiveTacticalEffectHandle InHandle = FActiveTacticalEffectHandle()) :
        mAttribute(InAttribute),
        mModifierOp(InModOp),
        mMagnitude(InMagnitude),
        mHandle(InHandle),
        mIsValid(true)
    {
    }

    FString ToSimpleString() const
    {
        return FString::Printf(TEXT("%s %s EvalMag: %f"), *mAttribute.GetName(), *TacticalEffectUtilities::TacticalModOpToString(mModifierOp), mMagnitude);
    }

    UPROPERTY()
    FTacticalAttribute mAttribute;

    UPROPERTY()
    TEnumAsByte<ETacticalModOp::Type> mModifierOp;

    UPROPERTY()
    float mMagnitude;

    UPROPERTY()
    FActiveTacticalEffectHandle	mHandle;

    UPROPERTY()
    bool mIsValid;
};

// 이펙트 제거 시 대리자
DECLARE_MULTICAST_DELEGATE_OneParam(FOnActiveTacticalEffectRemoved_Info, const FTacticalEffectRemovalInfo&);
// 이펙트 스택 수 변경 시 대리자
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnActiveTacticalEffectStackChange, FActiveTacticalEffectHandle, int32 /*NewStackCount*/, int32 /*PreviousStackCount*/);
// Duration 종료 Time 변경 시 대리자
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnActiveTacticalEffectTimeChange, FActiveTacticalEffectHandle, int32 /*NewStartTime*/, int32 /*NewDuration*/);

/**
 * @brief 활성 이펙트 하나가 발생시키는 이벤트(대리자) 묶음.
 * @details 
 * 이펙트 본체(FActiveTacticalEffect)에 값으로 포함되어, 제거/스택변경 시점을 외부에 통지
 */
struct FActiveTacticalEffectEvents
{
    FOnActiveTacticalEffectRemoved_Info OnEffectRemoved;
    FOnActiveTacticalEffectStackChange OnStackChanged;
    FOnActiveTacticalEffectTimeChange OnTimeChanged;
};

/**
 * @brief 현재 활성화 이펙트의 모든 런타임 정보
 */
USTRUCT()
struct FActiveTacticalEffect
{
    GENERATED_BODY()

public:
    FActiveTacticalEffect() = default;
    FActiveTacticalEffect(FActiveTacticalEffectHandle Handle, const FTacticalEffectSpec& Spec);

public:
    bool operator==(const FActiveTacticalEffect& Other)
    {
        return mHandle == Other.mHandle;
    }

public:
    int32 GetTimeRemaining(int32 WorldTime) const;
    int32 GetDuration() const;
    ETacticalEffectDurationUnitType GetDurationUnit() const;
    int32 GetEndTime() const;

public:
    // @brief 이펙트 ID
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "Handle"))
    FActiveTacticalEffectHandle mHandle;

    // @brief 이펙트 런타임 구성 데이터
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "Spec"))
    FTacticalEffectSpec mSpec;

    // @brief 이펙트 시작 타이밍
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "StartTime"))
    int32 mStartTime = 0;

public:
    // @brief 추가 예약된 다음 이펙트 포인터
    FActiveTacticalEffect* mPendingNext = nullptr;
    // @brief 삭제 예약된 이펙트 여부
    bool mIsPendingRemove = false;

public:
    // @brief 각 상황에 맞는 대리자
    FActiveTacticalEffectEvents mEventSet;
};

