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
    static FActiveTacticalEffectHandle GenerateNewHandle(UWorld* World, UAttributeSetComponentModel* OwningModel);
    static void ResetGlobalHandleMap(UWorld* World);

public:
    UAttributeSetComponentModel* GetOwningAttributeSetComponentModel() const;
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
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "Index"))
    int32 mIndex = INDEX_NONE;

    UPROPERTY(Category = "World", VisibleAnywhere, meta = (DisplayName = "World"))
    TWeakObjectPtr<UWorld> mWorld;
};

USTRUCT(BlueprintType)
struct FTacticalModifierEvaluatedData
{
    GENERATED_USTRUCT_BODY()

    FTacticalModifierEvaluatedData() :
        mAttribute(),
        mModifierOp(EGameplayModOp::Additive),
        mMagnitude(0.f),
        mIsValid(false)
    {
    }

    FTacticalModifierEvaluatedData(const FTacticalAttribute& InAttribute, TEnumAsByte<EGameplayModOp::Type> InModOp, float InMagnitude, FActiveTacticalEffectHandle InHandle = FActiveTacticalEffectHandle()) :
        mAttribute(InAttribute),
        mModifierOp(InModOp),
        mMagnitude(InMagnitude),
        mHandle(InHandle),
        mIsValid(true)
    {
    }

    FString ToSimpleString() const
    {
        return FString::Printf(TEXT("%s %s EvalMag: %f"), *mAttribute.GetName(), *EGameplayModOpToString(mModifierOp), mMagnitude);
    }

    UPROPERTY()
    FTacticalAttribute mAttribute;

    UPROPERTY()
    TEnumAsByte<EGameplayModOp::Type> mModifierOp;

    UPROPERTY()
    float mMagnitude;

    UPROPERTY()
    FActiveTacticalEffectHandle	mHandle;

    UPROPERTY()
    bool mIsValid;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnActiveTacticalEffectRemoved_Info, const FTacticalEffectRemovalInfo&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnActiveTacticalEffectStackChange, FActiveTacticalEffectHandle, int32 /*NewStackCount*/, int32 /*PreviousStackCount*/);

struct FActiveTacticalEffectEvents
{
    FOnActiveTacticalEffectRemoved_Info OnEffectRemoved;
    FOnActiveTacticalEffectStackChange OnStackChanged;
};

/**
 * @brief 현재 활성화 이펙트
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
    // @brief 이펙트 ID
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "Handle"))
    FActiveTacticalEffectHandle mHandle;

    // @brief 이펙트 런타임 구성 데이터
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "Spec"))
    FTacticalEffectSpec mSpec;

public:
    // @brief 추가 예약된 다음 이펙트 포인터
    FActiveTacticalEffect* mPendingNext = nullptr;
    // @brief 삭제 예약된 이펙트 여부
    bool mIsPendingRemove = false;

public:
    // @brief 각 상황에 맞는 대리자
    FActiveTacticalEffectEvents mEventSet;
};

