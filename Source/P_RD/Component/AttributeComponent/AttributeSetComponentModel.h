/*****************************************************************//**
 * @file   AttributeSetComponentModel.h
 * @brief  속성 컴포넌트 모델 정의 헤더
 * @author 모호재, 김준형
 * @date   2026-06-19
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "Component/ComponentModel.h"
#include "AttributeSetComponentModel.generated.h"

class UAttributeSet;
class UAttributeSetComponentModel;
struct FTacticalAttributeChangeData;
class UTacticalEffect;

// USaveGameSubsystem 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogAttributeSetComp, Log, All)

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChangeAttributeValue, const FTacticalAttributeChangeData& /*ChangeData*/);

/**
 * @brief 수치 변경 결과 데이터
 */
USTRUCT()
struct FTacticalAttributeChangeData
{
    GENERATED_BODY()

public:
    FGameplayAttribute Attribute = nullptr;
    float NewValue = 0.f;;
    float OldValue = 0.f;;
};

/**
 * @brief 활성화된 이펙트를 쉽게 탐색하기 위한 핸들
 */
USTRUCT()
struct FActiveTacticalEffectHandle
{
    GENERATED_BODY()

public:
    FActiveTacticalEffectHandle() = default;
    FActiveTacticalEffectHandle(int32 Index, UAttributeSetComponentModel* OwningModel);

public:
    static FActiveTacticalEffectHandle GenerateNewHandle(UAttributeSetComponentModel* OwningModel);

public:
    UAttributeSetComponentModel* GetOwningAttributeSetComponentModel() const;

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

    UPROPERTY(Category = "Owner", VisibleAnywhere, meta = (DisplayName = "OwningModel"))
    TWeakObjectPtr<UAttributeSetComponentModel> mOwningModel;
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

/**
 * @brief 하나의 변화값
 */
struct FTacticalAggregatorMod
{
public:
    // @brief 평가된 값
    float mEvaluatedMagnitude;
    // @brief 스택 갯수
    float mStackCount;
};

/**
 * @brief 속성 값에 대한 모든 변화 계산기 객체
 */
struct FTacticalAggregator : public TSharedFromThis<FTacticalAggregator>
{
    friend struct FActiveTacticalEffectsContainer;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnAttributeAggregatorDirty, FTacticalAggregator*);

public:
    float GetAttributeBaseValue() const;
    void SetAttributeBaseValue(float BaseValue, bool BroadcastDirtyEvent = true);

private:
    void BroadcastOnDirty();

public:
    // @brief 값이 변경됨을 알리는 대리자
    FOnAttributeAggregatorDirty OnDirty;
    // @brief 재귀적 호출 방지용 카운팅
    int32 mDirtyCount;

private:
    // @brief 베이스 값
    float mBaseValue;
    // @brief 각 연산자에 따른 결과값들
    TArray<FTacticalAggregatorMod> Mods[EGameplayModOp::Max];
    // @brief 해당 값에 영향을 받는 외부 이펙트들
    TArray<FActiveTacticalEffectHandle>	mDependentEffects;
};

/**
 * @brief 새로운 이펙트의 추가와 기존 이펙트의 삭제를 지연하기 위한 스코프 락 객체
 */
struct FScopedActiveTacticalEffectLock
{
    FScopedActiveTacticalEffectLock(FActiveTacticalEffectsContainer& Container);
    ~FScopedActiveTacticalEffectLock();

private:
    FActiveTacticalEffectsContainer& mContainer;
};
#define TACTICAL_EFFECT_SCOPE_LOCK() FScopedActiveTacticalEffectLock __ActiveScopeLock(*this);

/**
 * @brief 현재 활성화 이펙트들과 그에 따른 결과를 보유한 객체
 */
USTRUCT()
struct FActiveTacticalEffectsContainer
{
    GENERATED_BODY()

    friend class UAttributeSetComponentModel;
    friend struct FScopedActiveTacticalEffectLock;

    /* 이펙트 증감 락 카운팅 */
private:
    void IncrementLock();
    void DecrementLock();

private:
    void CleanupAttributeAggregator(const FGameplayAttribute& Attribute);
    void OnAttributeAggregatorDirty(FTacticalAggregator* Aggregator, FGameplayAttribute Attribute);
    void OnMagnitudeDependencyChange(FActiveTacticalEffectHandle Handle, const FTacticalAggregator* ChangedAgg);

public:
    // @brief 해당 객체를 소유한 컴포넌트 모델
    UPROPERTY(Category = "Owner", VisibleAnywhere, meta = (DisplayName = "Owner"))
    TWeakObjectPtr<UAttributeSetComponentModel> mOwner;

private:
    // @brief 활성화 중인 이펙트들
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "AttributeEffects"))
    TArray<FActiveTacticalEffect> mAttributeEffects;

    /* 이펙트를 확인하여 복구 가능한 객체들 */
private:
    // @brief 속성에 따른 값 계산기
    TMap<FGameplayAttribute, TSharedPtr<FTacticalAggregator>> mAttributeAggregatorMap;

    // @brief 값 변경에 따른 대리자
    TMap<FGameplayAttribute, FOnChangeAttributeValue> mAttributeValueChangeDelegates;

    // @brief 누적된 이펙트들
    TMap<TWeakObjectPtr<UTacticalEffect>, TArray<FActiveTacticalEffectHandle>> mSourceStackingMap;

    /* 활성 이펙트 증감 락 연관 */
private:
    // @brief 현재 이펙트 증감 락 카운팅
    mutable int32 mScopedLockCount;

    // @brief 삭제 예약 이펙트 카운트
    int32 mPendingRemoveCount;

    // @brief Lock에 의해 미루어진 추가 이펙트들 Linked-List 방식 자료구조
    FActiveTacticalEffect* mPendingGameplayEffectHead;
    FActiveTacticalEffect** mPendingGameplayEffectTail;
};

UCLASS()
class P_RD_API UAttributeSetComponentModel : public UComponentModel
{
	GENERATED_BODY()

    /* UComponentModel 상속 */
public:
    void Initialize() override;
    void Uninitialize() override;

public:
    void BeginPlay() override;
    void EndPlay() override;

    /* AttributeSet 세팅 */
public:
    template <typename T>
    const T* GetAttributeSet() const
    {
        return StaticCast<T*>(GetAttributeSet_Internal(T::StaticClass()));
    }
    template <typename T>
    const T* AddAttributeSet()
    {
        return StaticCast<T*>(GetOrCreateAttributeSet_Internal(T::StaticClass()));
    }

public:
    void AddSpawnedAttribute(UAttributeSet* Attribute);
    void RemoveSpawnedAttribute(UAttributeSet* Attribute);

protected:
    const UAttributeSet* GetAttributeSet_Internal(TSubclassOf<UAttributeSet> Class) const;
    const UAttributeSet* GetOrCreateAttributeSet_Internal(TSubclassOf<UAttributeSet> Class);

    /* 기본값 설정 */
public:
    void SetAttributeBaseValue(const FGameplayAttribute& Attribute, float BaseValue);
    float GetAttributeBaseValue(const FGameplayAttribute& Attribute) const;

    /* 현재값 설정 */
public:
    float GetAttributeCurrentValue(const FGameplayAttribute& Attribute) const;

    /* 변경 알림 */
public:
    /**
     * 속성 값이 변경될 경우 실행
     * @param Aggregator 해당 속성의 계산 객체
     * @param Attribute 변경 속성
     */
    void OnAttributeAggregatorDirty(FTacticalAggregator* Aggregator, FGameplayAttribute Attribute);
    /**
     * 속성 값에 의존하는 Effect에게 전파를 위해 실행
     * @param Handle 대상 Effect 핸들
     * @param ChangedAggregator 해당 속성의 계산 객체
     */
    void OnMagnitudeDependencyChange(FActiveTacticalEffectHandle Handle, const FTacticalAggregator* ChangedAggregator);

protected:
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "ActiveAttributeEffects"))
    FActiveTacticalEffectsContainer mActiveAttributeEffects;

protected:
    UPROPERTY(Category = "AttributeSet", VisibleAnywhere, meta = (DisplayName = "SpawnedAttributes"))
    TArray<TObjectPtr<UAttributeSet>> mSpawnedAttributes;
};
