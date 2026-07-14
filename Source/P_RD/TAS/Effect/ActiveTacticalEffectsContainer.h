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

// 속성 수치가 변경되었을 때 변경 전/후 값을 브로드캐스트하는 멀티캐스트 대리자
DECLARE_MULTICAST_DELEGATE_OneParam(FOnChangeAttributeValue, const FTacticalAttributeChangeData& /*ChangeData*/);
// 특정 활성 이펙트가 컨테이너에서 제거되었을 때 브로드캐스트하는 멀티캐스트 대리자
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGivenActiveTacticalEffectRemoved, const FActiveTacticalEffect& /*TacticalEffect*/);

/**
 * @brief  수치 변경 결과 데이터
 * @details 어떤 속성이 어떤 값에서 어떤 값으로 바뀌었는지를 담아
 *          FOnChangeAttributeValue 대리자에 실어 전달한다.
 */
USTRUCT()
struct FTacticalAttributeChangeData
{
    GENERATED_BODY()

public:
    FTacticalAttribute mAttribute = nullptr; // @brief 변경 대상 속성 핸들
    float mNewValue = 0.f;                   // @brief 변경 후 값
    float mOldValue = 0.f;                   // @brief 변경 전 값
};

/**
 * @brief  새로운 이펙트의 추가와 기존 이펙트의 삭제를 지연하기 위한 스코프 락 객체
 * @details 순회 중(반복자 사용 중)에 배열을 직접 수정하면 반복자가 무효화되므로,
 *          이 스코프가 살아 있는 동안의 추가/삭제는 보류(pending) 처리되고
 *          스코프가 끝나는 시점(소멸자)에 일괄 반영된다. RAII 패턴.
 */
struct FScopedActiveTacticalEffectLock
{
    // 생성자: 컨테이너의 락 카운트를 증가시켜 증감 지연을 시작한다.
    FScopedActiveTacticalEffectLock(FActiveTacticalEffectsContainer& Container);
    // 소멸자: 락 카운트를 감소시키고, 0이 되면 보류된 추가/삭제를 일괄 반영한다.
    ~FScopedActiveTacticalEffectLock();

private:
    FActiveTacticalEffectsContainer& mContainer; // @brief 락이 적용되는 대상 컨테이너
};
// 현재 스코프(*this 컨테이너)에 대해 스코프 락을 거는 편의 매크로
#define TACTICAL_EFFECT_SCOPE_LOCK() FScopedActiveTacticalEffectLock __ActiveScopeLock(*this);

/**
 * @brief  활성 이펙트 컨테이너 전용 반복자
 * @details 두 자료구조를 하나의 순회로 이어 붙인다.
 *          (1) mTacticalEffects 배열(이미 활성화된 이펙트), 그리고
 *          (2) 락에 의해 보류된 mPending* Linked-List(추가 예약 이펙트).
 *          반복자가 살아 있는 동안 컨테이너의 락 카운트를 유지하여 순회 중 배열 변형을 막는다.
 * @tparam ElementType   순회 요소 타입(const/비const FActiveTacticalEffect)
 * @tparam ContainerType 대상 컨테이너 타입
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

    // 소멸자: 락 카운트를 해제하여 보류된 증감을 반영할 수 있게 한다.
    ~FActiveTacticalEffectIterator()
    {
        mContainer.DecrementLock();
    }

public:
    // 전위 증가: 다음 유효 요소로 이동
    void operator++()
    {
        Next();
    }

    // 역참조: 현재 가리키는 요소 반환(유효성 단언)
    ElementType& operator*() const
    {
        check(mCurrent != nullptr);
        return *mCurrent;
    }
    // 멤버 접근: 현재 가리키는 요소 포인터 반환(유효성 단언)
    ElementType* operator->() const
    {
        check(mCurrent != nullptr);
        return mCurrent;
    }

    // bool 변환: 순회가 아직 유효한 요소를 가리키는지 여부
    explicit operator bool() const
    {
        return (mCurrent != nullptr);
    }

    // 동등 비교: 같은 노드를 가리키는지(주로 end 반복자와 비교에 사용)
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
     * @details 배열을 끝까지 살핀 뒤에는 보류 Linked-List로 자연스럽게 넘어가며,
     *          삭제 예약(mIsPendingRemove) 노드는 건너뛴다.
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
 * @brief  현재 활성화 이펙트들과 그에 따른 결과를 보유한 객체
 * @details GAS의 FActiveGameplayEffectsContainer를 대체하는 자체 구현으로,
 *          활성 이펙트 배열을 보관하고 속성별 Aggregator를 통해 최종 수치를 산출한다.
 *          이 PR(#191)에서 GAS의 EGameplayModOp가 자체 ETacticalModOp로 치환되면서,
 *          모디파이어 연산 종류를 받는 진입점(ApplyModToAttribute 등)도 함께 바뀌었다.
 */
USTRUCT()
struct FActiveTacticalEffectsContainer
{
    GENERATED_BODY()

    // 컴포넌트 모델 및 락/반복자 보조 타입에 한해 내부 상태 접근 허용(friend)
    friend class UAttributeSetComponentModel;
    friend struct FScopedActiveTacticalEffectLock;
    friend class FActiveTacticalEffectIterator<const FActiveTacticalEffect, FActiveTacticalEffectsContainer>;
    friend class FActiveTacticalEffectIterator<FActiveTacticalEffect, FActiveTacticalEffectsContainer>;

    // 읽기 전용/수정 가능 반복자 별칭
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
     * @brief 객체 복제(PostDuplicate) 후 내부 포인터/콜백 재배선을 수행한다.
     * @param DuplicateForPIE PIE용 복제 여부
     */
    void PostDuplicate(bool DuplicateForPIE);

    /* 이펙트 증감 락 카운팅 */
private:
    // @brief 증감 지연 락 카운트 +1(반복자/스코프 락 진입 시)
    void IncrementLock();
    // @brief 락 카운트 -1, 0이 되면 보류된 추가/삭제를 일괄 반영
    void DecrementLock();

    /* 변경에 따른 후속 조치 */
private:
    /**
     * @brief  속성에 대응하는 Aggregator를 찾거나 없으면 생성한다.
     * @param  Attribute 대상 속성
     * @return 해당 속성의 Aggregator 공유 포인터 참조
     */
    TSharedPtr<FTacticalAggregator>& FindOrCreateAttributeAggregator(const FTacticalAttribute& Attribute);
    // @brief 더 이상 모디파이어가 없는 속성의 Aggregator를 정리(제거)한다.
    void CleanupAttributeAggregator(const FTacticalAttribute& Attribute);
    // @brief Aggregator가 dirty 표시되었을 때 호출되어 현재 값을 재계산한다.
    void OnAttributeAggregatorDirty(FTacticalAggregator* Aggregator, FTacticalAttribute Attribute);
    // @brief 모디파이어 크기가 의존하는 다른 Aggregator 변경 시 재평가를 유발한다.
    void OnMagnitudeDependencyChange(FActiveTacticalEffectHandle Handle, const FTacticalAggregator* ChangedAgg);

    /**
     * @brief 스택 수가 변할 때 호출되어 스택 의존 모디파이어 크기를 갱신한다.
     * @details MultiplyCompound 등 스택에 따라 크기가 달라지는 연산의 재계산 트리거.
     */
    void OnStackCountChange(FActiveTacticalEffect& ActiveEffect, int32 OldStackCount, int32 NewStackCount);

    /* 속성 값 변화 */
public:
    // @brief 속성의 기본값(Base)을 직접 설정한다(모디파이어 적용 전 원천 값).
    void SetAttributeBaseValue(FTacticalAttribute Attribute, float BaseValue);
    // @brief 속성의 현재 기본값(Base)을 반환한다.
    float GetAttributeBaseValue(FTacticalAttribute Attribute) const;

    /** @brief 실제 상태를 변경하지 않고 지정 모디파이어를 한 건 더 적용한 현재값을 계산한다. */
    float EvaluateAttributeWithAdditionalModifier(
        const FTacticalAttribute& Attribute,
        TEnumAsByte<ETacticalModOp::Type> ModifierOp,
        float ModifierMagnitude) const;

    /**
     * @brief  속성에 단일 모디파이어를 즉시 적용한다.
     * @details [PR #191 핵심 치환] 연산 종류 인자가 GAS의 EGameplayModOp →
     *          자체 ETacticalModOp로 바뀐 진입점. ETacticalModOp의 정수값은 구
     *          EGameplayModOp와 동일하게 유지되므로(직렬화 호환 + Aggregator의
     *          mMods[ETacticalModOp::Max] 배열 인덱싱 + DefaultEngine.ini CoreRedirect),
     *          기존 에셋/세이브에 박힌 op 정수값을 그대로 해석할 수 있다.
     *          연산 의미: AddBase(0)=Base 합산, MultiplyAdditive(1)=배율 가산,
     *          DivideAdditive(2)=나눗셈 가산, Override(3)=덮어쓰기,
     *          MultiplyCompound(4)=거듭제곱 곱, AddFinal(5)=최종 합산.
     * @param  Attribute        적용 대상 속성
     * @param  ModifierOp       모디파이어 연산 종류(ETacticalModOp, 구 EGameplayModOp 대체)
     * @param  ModifierMagnitude 적용할 크기 값
     */
    void ApplyModToAttribute(const FTacticalAttribute& Attribute, TEnumAsByte<ETacticalModOp::Type> ModifierOp, float ModifierMagnitude);
    /**
     * @brief  이펙트 스펙을 컨테이너에 적용(스택 가능 시 기존 이펙트 갱신).
     * @param  Spec                  적용할 이펙트 스펙
     * @param  FoundExistingStackableGE [out] 스택 가능한 기존 이펙트를 찾아 합쳤는지 여부
     * @return 생성/갱신된 활성 이펙트 포인터
     */
    FActiveTacticalEffect* ApplyTacticalEffectSpec(const FTacticalEffectSpec& Spec, bool& FoundExistingStackableGE);
    // @brief 스펙의 즉시(Instant) 실행 모디파이어들을 속성에 곧바로 반영한다.
    void ExecuteActiveEffectsFrom(FTacticalEffectSpec& Spec);
    /**
     * @brief  핸들로 지정한 활성 이펙트의 스택을 제거(0이 되면 이펙트 자체 제거).
     * @param  Handle        대상 이펙트 핸들
     * @param  StacksToRemove 제거할 스택 수
     * @return 제거가 수행되었으면 true
     */
    bool RemoveActiveTacticalEffect(FActiveTacticalEffectHandle Handle, int32 StacksToRemove);

    /**
     * @brief  특정 속성의 값 변경 대리자를 반환한다(없으면 생성).
     * @param  Attribute 대상 속성
     * @return 해당 속성에 바인딩할 수 있는 값 변경 대리자 참조
     */
    FOnChangeAttributeValue& GetTacticalAttributeValueChangeDelegate(FTacticalAttribute Attribute);

private:
    // @brief 활성 이펙트가 추가 확정된 직후의 내부 후속 처리(태그/모디파이어 등록 등)
    void InternalOnActiveTacticalEffectAdded(FActiveTacticalEffect& Effect);
    // @brief 활성 이펙트가 제거 확정된 직후의 내부 후속 처리(부여 태그/모디파이어 해제 등)
    void InternalOnActiveTacticalEffectRemoved(FActiveTacticalEffect& Effect, const FTacticalEffectRemovalInfo& TacticalEffectRemovalInfo);

    /**
     * @brief  평가된 모디파이어 데이터를 실제 속성에 즉시 실행 반영한다.
     * @details ModEvalData에는 적용할 ETacticalModOp 연산과 산출된 크기가 들어 있다.
     * @param  Spec        실행 컨텍스트가 담긴 이펙트 스펙
     * @param  ModEvalData 연산 종류/크기가 평가 완료된 모디파이어 데이터
     * @return 실제로 값이 변경되었으면 true
     */
    bool InternalExecuteMod(FTacticalEffectSpec& Spec, FTacticalModifierEvaluatedData& ModEvalData);

    /**
     * @brief  배열 인덱스로 활성 이펙트의 스택을 제거하는 내부 구현.
     * @param  Idx             대상 이펙트의 배열 인덱스
     * @param  StacksToRemove  제거할 스택 수
     * @param  bPrematureRemoval 만료 전 강제 제거 여부(콜백 분기에 영향)
     * @return 제거가 수행되었으면 true
     */
    bool InternalRemoveActiveTacticalEffect(int32 Idx, int32 StacksToRemove, bool bPrematureRemoval);

    // @brief 이펙트가 부여하는 태그와 지속(Duration) 모디파이어를 Aggregator/태그 집합에 등록
    void AddActiveTacticalEffectGrantedTagsAndModifiers(FActiveTacticalEffect& Effect);
    // @brief 위에서 등록한 부여 태그와 모디파이어를 해제(제거)
    void RemoveActiveTacticalEffectGrantedTagsAndModifiers(const FActiveTacticalEffect& Effect);

private:
    // @brief 속성의 현재값(Current)을 갱신하고 값 변경 대리자를 브로드캐스트한다.
    void UpdateAttributeCurrentValue(FTacticalAttribute Attribute, float CurrentValue);
    // @brief 한 활성 이펙트가 영향을 주는 모든 Aggregator의 모디파이어 크기를 재계산한다.
    void UpdateAllAggregatorModMagnitudes(FActiveTacticalEffect& ActiveEffect);
    /**
     * @brief 지정한 속성 집합에 한해 해당 이펙트의 모디파이어 크기를 재계산한다.
     * @param AttributesToUpdate 갱신 대상 속성 집합
     * @param ActiveEffect       재계산 대상 활성 이펙트
     */
    void UpdateAggregatorModMagnitudes(const TSet<FTacticalAttribute>& AttributesToUpdate, FActiveTacticalEffect& ActiveEffect);

public:
    // @brief 핸들로 활성 이펙트를 조회(없으면 nullptr) — 가변 버전
    FActiveTacticalEffect* GetActiveTacticalEffect(const FActiveTacticalEffectHandle Handle);
    // @brief 핸들로 활성 이펙트를 조회(없으면 nullptr) — const 버전
    const FActiveTacticalEffect* GetActiveTacticalEffect(const FActiveTacticalEffectHandle Handle) const;
    // @brief 배열 인덱스로 활성 이펙트를 조회 — 가변 버전
    FActiveTacticalEffect* GetActiveTacticalEffect(int32 Index);
    // @brief 배열 인덱스로 활성 이펙트를 조회 — const 버전
    const FActiveTacticalEffect* GetActiveTacticalEffect(int32 Index) const;

    // @brief 현재 활성 이펙트 개수 반환
    int32 GetNumTacticalEffects() const;

private:
    // @brief 스펙과 스택 조건이 일치하는 기존 활성 이펙트를 찾는다(스택 합치기용).
    FActiveTacticalEffect* FindStackableActiveTacticalEffect(const FTacticalEffectSpec& Spec);

    /* 반복자 정의 */
public:
    // @brief 읽기 전용 반복자 생성(시작 위치)
    inline ConstIterator CreateConstIterator() const
    {
        return ConstIterator(*this);
    }
    // @brief 수정 가능 반복자 생성(시작 위치)
    inline Iterator CreateIterator()
    {
        return Iterator(*this);
    }

    // 범위 기반 for 지원: 비const 컨테이너 begin/end (end는 인덱스 -1 = 종료 표지)
public:
    inline friend Iterator begin(FActiveTacticalEffectsContainer* Container)
    {
        return Container->CreateIterator();
    }
    inline friend Iterator end(FActiveTacticalEffectsContainer* Container)
    {
        return Iterator(*Container, -1);
    }

    // 범위 기반 for 지원: const 컨테이너 begin/end
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
    // @details 각 Aggregator는 연산 종류별 모디파이어를 mMods[ETacticalModOp::Max] 배열에
    //          op 정수값으로 인덱싱해 보관한다. ETacticalModOp 정수값이 구 EGameplayModOp와
    //          동일하게 유지되어야 하는 이유 중 하나가 바로 이 배열 인덱싱이다.
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
    // @details Head는 보류 목록의 첫 노드, Tail은 "마지막 노드의 mPendingNext"를 가리키는
    //          이중 포인터로, 반복자가 tail 경계에 닿으면 보류 순회를 종료한다(AdvancePending 참고).
    FActiveTacticalEffect* mPendingTacticalEffectHead;
    FActiveTacticalEffect** mPendingTacticalEffectTail;
};

