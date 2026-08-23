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
class UBoardCombatTargetSnapshotData;
struct FTacticalEffectQuery;

// 속성 수치가 변경되었을 때 변경 전/후 값을 브로드캐스트하는 멀티캐스트 대리자
DECLARE_MULTICAST_DELEGATE_OneParam(FOnChangeAttributeValue, const FTacticalAttributeChangeData& /*ChangeData*/);
// 특정 활성 이펙트가 컨테이너에서 제거되었을 때 브로드캐스트하는 멀티캐스트 대리자
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGivenActiveTacticalEffectRemoved, const FActiveTacticalEffect& /*TacticalEffect*/);

/**
 * @brief  속성 수치 변경 결과 데이터
 * @details 
 * 어떤 속성이 어떤 값에서 어떤 값으로 바뀌었는지를 담아 FOnChangeAttributeValue 대리자에 실어 전달한다.
 */
USTRUCT()
struct FTacticalAttributeChangeData
{
    GENERATED_BODY()

public:
    // @brief 변경 대상 속성 핸들
    FTacticalAttribute mAttribute = nullptr;
    // @brief 변경 후 값
    float mNewValue = 0.f;
    // @brief 변경 전 값
    float mOldValue = 0.f;
};

/**
 * @brief  새로운 이펙트의 추가와 기존 이펙트의 삭제를 지연하기 위한 스코프 락 객체
 * @details 
 * 순회 중(반복자 사용 중)에 배열을 직접 수정하면 반복자가 무효화되므로,
 * 이 스코프가 살아 있는 동안의 추가/삭제는 보류(pending) 처리되고
 * 스코프가 끝나는 시점(소멸자)에 일괄 반영된다. RAII 패턴.
 */
struct FScopedActiveTacticalEffectLock
{
    FScopedActiveTacticalEffectLock(FActiveTacticalEffectsContainer& Container);
    ~FScopedActiveTacticalEffectLock();

private:
    FActiveTacticalEffectsContainer& mContainer;
};

// 현재 스코프(*this 컨테이너)에 대해 스코프 락을 거는 편의 매크로
#define TACTICAL_EFFECT_SCOPE_LOCK() FScopedActiveTacticalEffectLock __ActiveScopeLock(*this);

/**
 * @brief  활성 이펙트 컨테이너 전용 반복자
 * @tparam ElementType   순회 요소 타입
 * @tparam ContainerType 대상 컨테이너 타입
 * 
 * @details 
 * 두 자료구조를 하나의 순회로 이어 붙인다.
 * (1) mTacticalEffects 배열(이미 활성화된 이펙트)
 * (2) 락에 의해 보류된 mPending* Linked-List(추가 예약 이펙트)
 */
template<typename ElementType, typename ContainerType>
class FActiveTacticalEffectIterator
{
public:
    /**
     * @brief  반복자 생성. 락을 잡고 시작 위치를 가리키도록 초기화한다.
     * @param  Container 순회 대상 컨테이너
     * @param  StartIdx  시작 인덱스(음수면 end 반복자로 취급)
     */
    FActiveTacticalEffectIterator(const ContainerType& Container, int32 StartIdx = 0) :
        mContainer(const_cast<ContainerType&>(Container)),
        mIndex(StartIdx),
        mPending(nullptr),
        mCurrent(nullptr)
    {
        // 순회 동안 컨테이너 변형을 막기 위해 락 카운트 증가
        mContainer.IncrementLock();
        if (mIndex >= 0)
        {
            UpdateCurrent();
        }
    }

    ~FActiveTacticalEffectIterator()
    {
        // 락 카운트를 해제하여 보류된 증감을 반영
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
    /**
     * @brief  보류 Linked-List에서 다음 노드를 안전하게 전진시킨다.
     * @param  Next 다음 노드 포인터의 주소(현재 노드의 mPendingNext 등)
     * @return tail 경계에 도달하지 않았으면 다음 노드, 도달했으면 nullptr
     */
    ElementType* AdvancePending(ElementType** Next)
    {
        // tail은 "마지막 다음" 슬롯을 가리키므로, 거기에 닿으면 순회 종료(nullptr)
        return (Next != const_cast<ElementType**>(mContainer.mPendingTacticalEffectTail)) ? *Next : nullptr;
    }

    /**
     * @brief 현재 위치를 한 칸 전진시키고 가리킬 노드를 갱신한다.
     * @details 인덱스가 음수면 보류 Linked-List 순회 단계, 0 이상이면 배열 순회 단계.
     */
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

    /**
     * @brief 현재 인덱스/보류 노드 상태에 맞춰 mCurrent를 결정한다.
     * @details 
     * 배열을 끝까지 살핀 뒤에는 보류 Linked-List로 자연스럽게 넘어가며,
     * 삭제 예약(mIsPendingRemove) 노드는 건너뛴다.
     */
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
 * @brief  현재 활성화 이펙트 컨테이너
 * @details 
 * 활성 이펙트 배열을 보관하고 속성별 Aggregator를 통해 최종 수치를 산출한다.
 */
USTRUCT()
struct FActiveTacticalEffectsContainer
{
    GENERATED_BODY()

    friend class UAttributeSetComponentModel;
    friend struct FScopedActiveTacticalEffectLock;
    friend class FActiveTacticalEffectIterator<const FActiveTacticalEffect, FActiveTacticalEffectsContainer>;
    friend class FActiveTacticalEffectIterator<FActiveTacticalEffect, FActiveTacticalEffectsContainer>;

    // 반복자 별칭
    typedef FActiveTacticalEffectIterator<const FActiveTacticalEffect, FActiveTacticalEffectsContainer> ConstIterator;
    typedef FActiveTacticalEffectIterator<FActiveTacticalEffect, FActiveTacticalEffectsContainer> Iterator;

public:
    FActiveTacticalEffectsContainer();
    ~FActiveTacticalEffectsContainer();

    /* 초기 등록 */
public:
    /**
     * @brief 이 컨테이너를 소유하는 컴포넌트 모델을 등록한다.
     * @param Owner 소유 컴포넌트 모델
     */
    void RegisterWithOwnerModel(UAttributeSetComponentModel* Owner);
    /**
     * @brief 객체 복제(PostDuplicate) 후 활성 이펙트를 가지고 Aggregator들을 복구한다.
     * @param DuplicateForPIE PIE용 복제 여부
     */
    void PostDuplicate(bool DuplicateForPIE);

    /* 이펙트 증감 락 카운팅 */
private:
    void IncrementLock();
    void DecrementLock();

    /* 변경에 따른 후속 조치 */
private:
    /**
     * @brief 속성에 대응하는 Aggregator를 찾거나 없으면 생성한다.
     * @param Attribute 대상 속성
     * @return 해당 속성의 Aggregator 공유 포인터 참조
     */
    TSharedPtr<FTacticalAggregator>& FindOrCreateAttributeAggregator(const FTacticalAttribute& Attribute);
    /**
     * @brief 더 이상 모디파이어가 없는 속성의 Aggregator를 정리(제거)한다.
     * @param Attribute 대상 속성
     */
    void CleanupAttributeAggregator(const FTacticalAttribute& Attribute);
    /**
     * @brief Aggregator가 dirty 표시되었을 때 호출되어 현재 값을 재계산한다
     * @param Aggregator Dirty 상태의 Aggregator
     * @param Attribute 대상 속성
     */
    void OnAttributeAggregatorDirty(FTacticalAggregator* Aggregator, FTacticalAttribute Attribute);
    /**
     * @brief 모디파이어 크기가 의존하는 다른 Aggregator 변경 시 재평가를 유발한다. 
     * @param Handle 변경된 Effect 핸들
     * @param ChangedAgg 변경된 Aggregator
     * 
     * @details
     * 현재는 사용하지 않아 내부 구현은 생략
     */
    void OnMagnitudeDependencyChange(FActiveTacticalEffectHandle Handle, const FTacticalAggregator* ChangedAgg);

    /**
     * @brief 스택 수가 변할 때 호출되어 스택 의존 모디파이어 크기를 갱신한다.
     */
    void OnStackCountChange(FActiveTacticalEffect& ActiveEffect, int32 OldStackCount, int32 NewStackCount);
    /**
     * @brief 기간이 변할 때 호출되어 대리자를 호출한다.
     */
    void OnDurationChange(FActiveTacticalEffect& ActiveEffect);

    /* 속성 값 변화 */
public:
    void SetAttributeBaseValue(FTacticalAttribute Attribute, float BaseValue);
    float GetAttributeBaseValue(FTacticalAttribute Attribute) const;

    /**
     * @brief  속성에 단일 모디파이어를 즉시 적용한다.
     * @param  Attribute 적용 대상 속성
     * @param  ModifierOp 모디파이어 연산 종류
     * @param  ModifierMagnitude 적용할 크기 값
     */
    void ApplyModToAttribute(const FTacticalAttribute& Attribute, TEnumAsByte<ETacticalModOp::Type> ModifierOp, float ModifierMagnitude);
    /**
     * @brief 이펙트 스펙을 컨테이너에 적용
     * @param Spec 적용할 이펙트 스펙
     * @param FoundExistingStackableGE 스택 가능한 기존 이펙트인지 여부
     * @return 생성/갱신된 활성 이펙트 포인터
     */
    FActiveTacticalEffect* ApplyTacticalEffectSpec(const FTacticalEffectSpec& Spec, OUT bool& FoundExistingStackableGE);
    /**
     * @brief Instant Effect와 같이 즉발 변화를 처리하는 함수
     * @param Spec 적용할 이펙트 스펙
     */
    void ExecuteActiveEffectsFrom(FTacticalEffectSpec& Spec);
    /**
     * @brief 핸들로 지정한 활성 이펙트의 스택을 제거(0이 되면 이펙트 자체 제거).
     * @param Handle 대상 이펙트 핸들
     * @param StacksToRemove 제거할 스택 수
     * @return 제거가 수행되었으면 true
     */
    bool RemoveActiveTacticalEffect(FActiveTacticalEffectHandle Handle, int32 StacksToRemove);
    int32 RemoveActiveEffects(const FTacticalEffectQuery& Query, int32 StacksToRemove);

    /**
     * @brief 특정 속성의 값 변경 대리자를 반환한다(없으면 생성).
     * @param Attribute 대상 속성
     * @return 해당 속성에 바인딩할 수 있는 값 변경 대리자 참조
     */
    FOnChangeAttributeValue& GetTacticalAttributeValueChangeDelegate(FTacticalAttribute Attribute);

public:
    /**
     * @brief 라운드 만기 체크
     * @param Time 현재 단위 시간
     * @param UnitType 시간 단위
     */
    void CheckDurationExpired(const int32 Time, const ETacticalEffectDurationUnitType UnitType);

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

private:
    void RestartActiveTacticalEffectDuration(FActiveTacticalEffect& ActiveTacticalEffect);

public:
    void CaptureAllEffectStacks(UBoardCombatTargetSnapshotData* Snapshot) const;

    /**
     * @brief 핸들로 활성 이펙트를 조회(없으면 nullptr)
     * @param Handle 탐색 핸들
     * @return 발견된 활성 Effect 객체
     */
    FActiveTacticalEffect* GetActiveTacticalEffect(const FActiveTacticalEffectHandle Handle);
    const FActiveTacticalEffect* GetActiveTacticalEffect(const FActiveTacticalEffectHandle Handle) const;
    /**
     * @brief 배열 인덱스로 활성 이펙트를 조회
     * @param Index 활성 Effect 인덱스
     * @return 발견된 활성 Effect 객체
     */
    FActiveTacticalEffect* GetActiveTacticalEffect(int32 Index);
    const FActiveTacticalEffect* GetActiveTacticalEffect(int32 Index) const;

    TArray<FActiveTacticalEffectHandle> GetActiveEffects(const FTacticalEffectQuery& Query) const;

    TArray<float> GetActiveEffectsTimeRemaining(const FTacticalEffectQuery& Query, ETacticalEffectDurationUnitType UnitType) const;
    TArray<float> GetActiveEffectsDuration(const FTacticalEffectQuery& Query) const;
    TArray<TPair<float, float>> GetActiveEffectsTimeRemainingAndDuration(const FTacticalEffectQuery& Query, ETacticalEffectDurationUnitType UnitType) const;

    float GetTacticalEffectMagnitude(FActiveTacticalEffectHandle Handle, const FTacticalAttribute& Attribute) const;
    int32 GetActiveEffectCount(const FTacticalEffectQuery& Query) const;
    int32 GetNumTacticalEffects() const;
    int32 GetWorldTime(ETacticalEffectDurationUnitType UnitType) const;

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
    // @brief 임의의 활성 이펙트가 제거될 때 외부에 알리는 대리자
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

