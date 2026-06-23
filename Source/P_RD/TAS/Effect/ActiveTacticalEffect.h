/*****************************************************************//**
 * @file   ActiveTacticalEffect.h
 * @brief  현재 활성화 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-06-23
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "ActiveTacticalEffect.generated.h"

class UAttributeSetComponentModel;

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

/**
 * @brief 현재 활성화 이펙트
 */
USTRUCT()
struct FActiveTacticalEffect
{
    GENERATED_BODY()

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
    // UPROPERTY()
    // FGameplayEffectSpec mSpec;

public:
    // @brief 추가 예약된 다음 이펙트 포인터
    FActiveTacticalEffect* mPendingNext = nullptr;
    // @brief 삭제 예약된 이펙트 여부
    bool mIsPendingRemove = false;
};

