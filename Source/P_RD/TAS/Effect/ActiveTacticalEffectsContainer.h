/*****************************************************************//**
 * @file   ActiveTacticalEffectsContainer.h
 * @brief  현재 활성화 이펙트들과 그에 따른 결과를 보유한 객체 정의 헤더
 * @author 모호재
 * @date   2026-06-23
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "TAS/Effect/ActiveTacticalEffect.h"
#include "ActiveTacticalEffectsContainer.generated.h"

struct FTacticalAttributeChangeData;
class UAttributeSetComponentModel;
struct FTacticalAggregator;
class UTacticalEffect;
struct FTacticalEffectSpec;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChangeAttributeValue, const FTacticalAttributeChangeData& /*ChangeData*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGivenActiveTacticalEffectRemoved, const FActiveTacticalEffect& /*TacticalEffect*/);

/**
 * @brief 수치 변경 결과 데이터
 */
USTRUCT()
struct FTacticalAttributeChangeData
{
    GENERATED_BODY()

public:
    FTacticalAttribute mAttribute = nullptr;
    float mNewValue = 0.f;
    float mOldValue = 0.f;
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

template<typename ElementType, typename ContainerType>
class FActiveTacticalEffectIterator
{
public:
    FActiveTacticalEffectIterator(const ContainerType& Container, int32 StartIdx = 0) :
        mContainer(const_cast<ContainerType&>(Container)),
        mIndex(StartIdx),
        mPending(nullptr),
        mCurrent(nullptr)
    {
        mContainer.IncrementLock();
        if (mIndex >= 0)
        {
            UpdateCurrent();
        }
    }

    ~FActiveTacticalEffectIterator()
    {
        mContainer.DecrementLock();
    }

public:
    void operator++()
    {
        Next();
    }

    ElementType& operator*() const
    {
        check(mCurrent != nullptr);
        return *mCurrent;
    }
    ElementType* operator->() const
    {
        check(mCurrent != nullptr);
        return mCurrent;
    }

    explicit operator bool() const
    {
        return (mCurrent != nullptr);
    }

    friend bool operator==(const FActiveTacticalEffectIterator& Lhs, const FActiveTacticalEffectIterator& Rhs)
    {
        return Lhs.mCurrent == Rhs.mCurrent;
    }
    friend bool operator!=(const FActiveTacticalEffectIterator& Lhs, const FActiveTacticalEffectIterator& Rhs)
    {
        return Lhs.mCurrent != Rhs.mCurrent;
    }

private:
    ElementType* AdvancePending(ElementType** Next)
    {
        return (Next != const_cast<ElementType**>(mContainer.mPendingTacticalEffectTail)) ? *Next : nullptr;
    }

    void Next()
    {
        if (mIndex >= 0)
        {
            // 일반 이펙트는 해당 경로로 Iter 증가
            ++mIndex;
        }
        else if (mPending != nullptr)
        {
            // 증감 이펙트는 해당 경로로 Iter 증가
            mPending = AdvancePending(const_cast<ElementType**>(&mPending->mPendingNext));
        }
        UpdateCurrent();
    }

    void UpdateCurrent()
    {
        if (mIndex < 0)
        {
            // 이미 추가 예약된 객체 순회로 전환됨. Linked-List에 따라서 Pending을 현재 탐색 노드로 지정
            mCurrent = mPending;
        }
        else if (mIndex >= mContainer.mTacticalEffects.Num())
        {
            // 기존 Effect를 모두 살폈다면, 추가 예약된 객체도 탐색
            mPending = AdvancePending(const_cast<ElementType**>(&mContainer.mPendingTacticalEffectHead));
            mCurrent = mPending;
            mIndex = INDEX_NONE;
        }
        else
        {
            // 우선적으로 기존 Effect부터 살피기
            mCurrent = &mContainer.mTacticalEffects[mIndex];
        }

        if (mCurrent != nullptr && mCurrent->mIsPendingRemove == true)
        {
            // 삭제될 객체는 반복에서 빠지기
            Next();
        }
    }

private:
    ContainerType& mContainer;

private:
    // @brief 현재 Effect 배열 내 인덱스
    int32 mIndex;
    // @brief 예약 Effect의 노드
    ElementType* mPending;
    // @brief 현재 노드
    ElementType* mCurrent;
};

/**
 * @brief 현재 활성화 이펙트들과 그에 따른 결과를 보유한 객체
 */
USTRUCT()
struct FActiveTacticalEffectsContainer
{
    GENERATED_BODY()

    friend class UAttributeSetComponentModel;
    friend struct FScopedActiveTacticalEffectLock;
    friend class FActiveTacticalEffectIterator<const FActiveTacticalEffect, FActiveTacticalEffectsContainer>;
    friend class FActiveTacticalEffectIterator<FActiveTacticalEffect, FActiveTacticalEffectsContainer>;

    typedef FActiveTacticalEffectIterator<const FActiveTacticalEffect, FActiveTacticalEffectsContainer> ConstIterator;
    typedef FActiveTacticalEffectIterator<FActiveTacticalEffect, FActiveTacticalEffectsContainer> Iterator;

public:
    FActiveTacticalEffectsContainer();
    ~FActiveTacticalEffectsContainer();

    /* 초기 등록 */
public:
    void RegisterWithOwnerModel(UAttributeSetComponentModel* Owner);
    void PostDuplicate(bool DuplicateForPIE);

    /* 이펙트 증감 락 카운팅 */
private:
    void IncrementLock();
    void DecrementLock();

    /* 변경에 따른 후속 조치 */
private:
    TSharedPtr<FTacticalAggregator>& FindOrCreateAttributeAggregator(const FTacticalAttribute& Attribute);
    void CleanupAttributeAggregator(const FTacticalAttribute& Attribute);
    void OnAttributeAggregatorDirty(FTacticalAggregator* Aggregator, FTacticalAttribute Attribute);
    void OnMagnitudeDependencyChange(FActiveTacticalEffectHandle Handle, const FTacticalAggregator* ChangedAgg);

    void OnStackCountChange(FActiveTacticalEffect& ActiveEffect, int32 OldStackCount, int32 NewStackCount);

    /* 속성 값 변화 */
public:
    void SetAttributeBaseValue(FTacticalAttribute Attribute, float BaseValue);
    float GetAttributeBaseValue(FTacticalAttribute Attribute) const;

    void ApplyModToAttribute(const FTacticalAttribute& Attribute, TEnumAsByte<ETacticalModOp::Type> ModifierOp, float ModifierMagnitude);
    FActiveTacticalEffect* ApplyTacticalEffectSpec(const FTacticalEffectSpec& Spec, bool& FoundExistingStackableGE);
    void ExecuteActiveEffectsFrom(FTacticalEffectSpec& Spec);
    bool RemoveActiveTacticalEffect(FActiveTacticalEffectHandle Handle, int32 StacksToRemove);

    FOnChangeAttributeValue& GetTacticalAttributeValueChangeDelegate(FTacticalAttribute Attribute);

private:
    void InternalOnActiveTacticalEffectAdded(FActiveTacticalEffect& Effect);
    void InternalOnActiveTacticalEffectRemoved(FActiveTacticalEffect& Effect, const FTacticalEffectRemovalInfo& TacticalEffectRemovalInfo);
    
    bool InternalExecuteMod(FTacticalEffectSpec& Spec, FTacticalModifierEvaluatedData& ModEvalData);

    bool InternalRemoveActiveTacticalEffect(int32 Idx, int32 StacksToRemove, bool bPrematureRemoval);

    void AddActiveTacticalEffectGrantedTagsAndModifiers(FActiveTacticalEffect& Effect);
    void RemoveActiveTacticalEffectGrantedTagsAndModifiers(const FActiveTacticalEffect& Effect);

private:
    void UpdateAttributeCurrentValue(FTacticalAttribute Attribute, float CurrentValue);
    void UpdateAllAggregatorModMagnitudes(FActiveTacticalEffect& ActiveEffect);
    void UpdateAggregatorModMagnitudes(const TSet<FTacticalAttribute>& AttributesToUpdate, FActiveTacticalEffect& ActiveEffect);

public:
    FActiveTacticalEffect* GetActiveTacticalEffect(const FActiveTacticalEffectHandle Handle);
    const FActiveTacticalEffect* GetActiveTacticalEffect(const FActiveTacticalEffectHandle Handle) const;
    FActiveTacticalEffect* GetActiveTacticalEffect(int32 Index);
    const FActiveTacticalEffect* GetActiveTacticalEffect(int32 Index) const;

    int32 GetNumTacticalEffects() const;

private:
    FActiveTacticalEffect* FindStackableActiveTacticalEffect(const FTacticalEffectSpec& Spec);

    /* 반복자 정의 */
public:
    inline ConstIterator CreateConstIterator() const 
    { 
        return ConstIterator(*this); 
    }
    inline Iterator CreateIterator() 
    { 
        return Iterator(*this); 
    }

public:
    inline friend Iterator begin(FActiveTacticalEffectsContainer* Container) 
    { 
        return Container->CreateIterator();
    }
    inline friend Iterator end(FActiveTacticalEffectsContainer* Container) 
    { 
        return Iterator(*Container, -1); 
    }

public:
    inline friend ConstIterator begin(const FActiveTacticalEffectsContainer* Container)
    { 
        return Container->CreateConstIterator();
    }
    inline friend ConstIterator end(const FActiveTacticalEffectsContainer* Container) 
    {
        return ConstIterator(*Container, -1); 
    }

public:
    FOnGivenActiveTacticalEffectRemoved	OnGivenActiveTacticalEffectRemovedDelegate;

public:
    // @brief 해당 객체를 소유한 컴포넌트 모델
    UPROPERTY(Category = "Owner", VisibleAnywhere, meta = (DisplayName = "Owner"))
    TWeakObjectPtr<UAttributeSetComponentModel> mOwner;

private:
    // @brief 활성화 중인 이펙트들
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "AttributeEffects"))
    TArray<FActiveTacticalEffect> mTacticalEffects;

    /* 이펙트를 확인하여 복구 가능한 객체들 */
private:
    // @brief 속성에 따른 값 계산기
    TMap<FTacticalAttribute, TSharedPtr<FTacticalAggregator>> mAttributeAggregatorMap;

    // @brief 값 변경에 따른 대리자
    TMap<FTacticalAttribute, FOnChangeAttributeValue> mAttributeValueChangeDelegates;

    // @brief 누적된 이펙트들
    TMap<TWeakObjectPtr<UTacticalEffect>, TArray<FActiveTacticalEffectHandle>> mSourceStackingMap;

    /* 활성 이펙트 증감 락 연관 */
private:
    // @brief 현재 이펙트 증감 락 카운팅
    mutable int32 mScopedLockCount;

    // @brief 삭제 예약 이펙트 카운트
    int32 mPendingRemoveCount;

    // @brief Lock에 의해 미루어진 추가 이펙트들 Linked-List 방식 자료구조
    FActiveTacticalEffect* mPendingTacticalEffectHead;
    FActiveTacticalEffect** mPendingTacticalEffectTail;
};

