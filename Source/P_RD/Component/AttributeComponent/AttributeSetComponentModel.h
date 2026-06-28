/*****************************************************************//**
 * @file   AttributeSetComponentModel.h
 * @brief  속성 컴포넌트 모델 정의 헤더
 * @author 모호재
 * @date   2026-06-19
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "Component/ComponentModel.h"
#include "TAS/Effect/ActiveTacticalEffect.h"
#include "TAS/Effect/ActiveTacticalEffectsContainer.h"
#include "TAS/Effect/TacticalTagCountContainer.h"
#include "GameplayTagAssetInterface.h"
#include "AttributeSetComponentModel.generated.h"

// 전방 선언: 속성별 모디파이어 계산기. 헤더 의존성을 줄이기 위해 포인터로만 참조한다.
struct FTacticalAggregator;

/** 전술 게임플레이 이벤트 태그가 브로드캐스트될 때 (태그, 이벤트 데이터)를 전달하는 멀티캐스트 델리게이트. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FTacticalEventTagMulticastDelegate, FGameplayTag, const FGameplayEventData*);

/**
 * @brief 한 유닛(액터)이 보유하는 모든 TacticalAttributeSet, 활성 Effect, 게임플레이 태그 카운트를 관리하는 컴포넌트 모델.
 *
 * GAS의 UAbilitySystemComponent에 대응하는 자체 구현체로, 속성 기본값/현재값 조회·설정, Effect Spec 생성·적용·제거,
 * Loose 게임플레이 태그 관리, 속성 변경 전파(Aggregator dirty)를 담당한다.
 * IGameplayTagAssetInterface를 구현하여 외부에서 이 모델이 소유한 태그를 질의할 수 있다.
 */
UCLASS()
class P_RD_API UAttributeSetComponentModel : public UComponentModel, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

    // 활성 Effect 구조체가 이 모델의 protected 내부(Aggregator/태그맵 갱신 등)에 직접 접근해야 하므로 friend 부여.
    friend struct FActiveTacticalEffect;
    friend struct FActiveTacticalEffectsContainer;

    /** Effect가 적용되었을 때 (적용 대상 모델, 적용된 Spec, 활성 핸들)을 전달하는 멀티캐스트 델리게이트. */
    DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnTacticalEffectAppliedDelegate, UAttributeSetComponentModel*, const FTacticalEffectSpec&, FActiveTacticalEffectHandle);

    /* UComponentModel 상속 */
public:
    /** @brief 컴포넌트 모델 초기화. 베이스 클래스의 생성 시점 셋업을 수행한다. */
    void Initialize() override;
    /** @brief 컴포넌트 모델 해제. 보유 리소스를 정리한다. */
    void Uninitialize() override;

public:
    /**
     * @brief 복제(Duplicate) 직후 호출되는 후처리.
     * @param DuplicateForPIE PIE(에디터 내 플레이)용 복제이면 true.
     */
    void PostDuplicate(bool DuplicateForPIE) override;

public:
    /** @brief 게임플레이 시작 시점 콜백. */
    void BeginPlay() override;
    /** @brief 게임플레이 종료 시점 콜백. */
    void EndPlay() override;

    /* AttributeSet 세팅 */
public:
    /**
     * @brief 지정 타입 T의 AttributeSet을 조회한다(없으면 생성하지 않음).
     * @tparam T 조회할 UTacticalAttributeSet 파생 타입.
     * @return 해당 타입의 AttributeSet 포인터(없으면 nullptr).
     */
    template <typename T>
    const T* GetAttributeSet() const
    {
        return StaticCast<T*>(GetAttributeSet_Internal(T::StaticClass()));
    }
    /**
     * @brief 지정 타입 T의 AttributeSet을 조회하고, 없으면 새로 생성하여 반환한다.
     * @tparam T 추가할 UTacticalAttributeSet 파생 타입.
     * @return 기존 또는 새로 생성된 AttributeSet 포인터.
     */
    template <typename T>
    const T* AddAttributeSet()
    {
        return StaticCast<T*>(GetOrCreateAttributeSet_Internal(T::StaticClass()));
    }

    /** @brief 현재 이 모델에 스폰되어 있는 모든 AttributeSet 배열을 반환한다. */
    const TArray<UTacticalAttributeSet*>& GetSpawnedAttributes() const;

public:
    /**
     * @brief 지정 속성을 담당하는 AttributeSet이 존재하는지 검사한다.
     * @param Attribute 검사할 전술 속성.
     * @return 해당 속성을 보유한 AttributeSet이 있으면 true.
     */
    bool HasAttributeSetForAttribute(FTacticalAttribute Attribute) const;
    /** @brief 스폰된 AttributeSet 목록에 등록한다. @param AttributeSet 추가할 AttributeSet. */
    void AddSpawnedAttributeSet(UTacticalAttributeSet* AttributeSet);
    /** @brief 스폰된 AttributeSet 목록에서 제거한다. @param AttributeSet 제거할 AttributeSet. */
    void RemoveSpawnedAttributeSet(UTacticalAttributeSet* AttributeSet);

protected:
    /**
     * @brief (내부) 클래스 기준으로 AttributeSet을 조회한다.
     * @param Class 조회할 AttributeSet 클래스.
     * @return 일치하는 AttributeSet(없으면 nullptr).
     */
    const UTacticalAttributeSet* GetAttributeSet_Internal(TSubclassOf<UTacticalAttributeSet> Class) const;
    /**
     * @brief (내부) 클래스 기준으로 AttributeSet을 조회하고, 없으면 생성하여 등록한다.
     * @param Class 조회/생성할 AttributeSet 클래스.
     * @return 기존 또는 새로 생성된 AttributeSet.
     */
    const UTacticalAttributeSet* GetOrCreateAttributeSet_Internal(TSubclassOf<UTacticalAttributeSet> Class);

    /* 캡처 */
public:
    /**
     * @brief 현재 모든 속성/상태를 스냅샷에 캡처한다(세이브/전투 상태 기록 용도).
     * @param Snapshot 출력 스냅샷 데이터.
     */
    void CaptureAllStates(FBoardCombatTargetSnapshotData& Snapshot) const;

    /* 기본값 설정 */
public:
    /**
     * @brief 속성의 기본값(BaseValue)을 설정한다. 모디파이어 적용 전의 원본 값에 해당한다.
     * @param Attribute 대상 속성.
     * @param BaseValue 설정할 기본값.
     */
    void SetAttributeBaseValue(const FTacticalAttribute& Attribute, float BaseValue);
    /**
     * @brief 속성의 기본값(BaseValue)을 조회한다.
     * @param Attribute 대상 속성.
     * @return 해당 속성의 기본값.
     */
    float GetAttributeBaseValue(const FTacticalAttribute& Attribute) const;

    /* 현재값 설정 */
public:
    /**
     * @brief 속성의 현재값(모디파이어가 모두 반영된 최종 값)을 조회한다.
     * @param Attribute 대상 속성.
     * @param[out] Found 해당 속성을 찾았으면 true로 설정된다.
     * @return 현재값(미발견 시 정의된 기본값).
     */
    float GetAttributeCurrentValue(FTacticalAttribute Attribute, bool& Found) const;
    /**
     * @brief 속성의 현재값을 조회한다(발견 여부 출력 없는 오버로드).
     * @param Attribute 대상 속성.
     * @return 현재값.
     */
    float GetAttributeCurrentValue(const FTacticalAttribute& Attribute) const;

protected:
    /**
     * @brief (내부) 속성 현재값을 직접 갱신한다. 클램프 등 후처리를 거쳐 NewValue가 수정될 수 있다.
     * @param Attribute 대상 속성.
     * @param[in,out] NewValue 적용할 값(클램프 결과로 변경될 수 있음).
     */
    void SetAttributeCurrentValue_Internal(const FTacticalAttribute& Attribute, float& NewValue);

public:
    /**
     * @brief 지정 속성의 "현재값 변경" 델리게이트를 반환한다. 구독 시 해당 속성 값이 바뀔 때 알림을 받는다.
     * @param Attribute 대상 속성.
     * @return 값 변경 델리게이트 참조.
     */
    FOnChangeAttributeValue& GetTacticalAttributeValueChangeDelegate(FTacticalAttribute Attribute);
    /**
     * @brief 이 모델이 "다른 대상에게" Effect를 적용했을 때 호출되는 콜백.
     * @param Model 적용된 대상 모델.
     * @param SpecApplied 적용된 Effect Spec.
     * @param ActiveHandle 활성 Effect 핸들.
     */
    void OnTacticalEffectAppliedToTarget(UAttributeSetComponentModel* Model, const FTacticalEffectSpec& SpecApplied, FActiveTacticalEffectHandle ActiveHandle);
    /**
     * @brief 이 모델이 "자기 자신에게" Effect를 적용했을 때 호출되는 콜백.
     * @param Model 적용된 대상 모델(자기 자신).
     * @param SpecApplied 적용된 Effect Spec.
     * @param ActiveHandle 활성 Effect 핸들.
     */
    void OnTacticalEffectAppliedToSelf(UAttributeSetComponentModel* Model, const FTacticalEffectSpec& SpecApplied, FActiveTacticalEffectHandle ActiveHandle);

    /* 수정자 적용 */
public:
    /**
     * @brief 단일 모디파이어를 속성에 즉시 적용한다.
     *
     * [PR #191 마이그레이션] ModifierOp 인자의 타입이 GAS의 TEnumAsByte<EGameplayModOp::Type>에서
     * 자체 enum TEnumAsByte<ETacticalModOp::Type>(TacticalEffectType.h 정의)으로 치환되었다.
     * ETacticalModOp의 정수값은 구 EGameplayModOp와 의도적으로 동일하게 유지된다 — (1) 기존 에셋/세이브에
     * 박힌 op 정수값의 직렬화 호환을 위해, (2) Aggregator가 mMods[ETacticalModOp::Max] 형태로 op 값을 그대로
     * 배열 인덱스로 사용하기 때문, (3) DefaultEngine.ini의 CoreRedirect가 구 enum 이름을 새 enum으로 매핑하기 때문.
     *
     * 연산 의미(op 정수값과 수식):
     *  - AddBase(0)         : Result = Base + Magnitude            (합산)
     *  - MultiplyAdditive(1): 배율 가산 — 1 + Σ(Magnitude)를 곱하는 가산형 배율
     *  - DivideAdditive(2)  : 나눗셈 가산 — 누적 분모에 의한 나눗셈형 배율
     *  - Override(3)        : Result = Magnitude                  (덮어쓰기)
     *  - MultiplyCompound(4): 거듭제곱 곱 — 스택마다 Magnitude를 곱셈 복리로 누적
     *  - AddFinal(5)        : 모든 배율 적용 후 최종 단계에서의 합산
     * (하위호환 별칭: Additive=0 / Multiplicitive=1 / Division=2 / Override=3 — 구 GAS 이름)
     *
     * @param Attribute 대상 속성.
     * @param ModifierOp 적용할 연산 종류(ETacticalModOp).
     * @param ModifierMagnitude 모디파이어 크기(연산에 투입되는 피연산자 값).
     */
    void ApplyModToAttribute(const FTacticalAttribute& Attribute, TEnumAsByte<ETacticalModOp::Type> ModifierOp, float ModifierMagnitude);

    /* Effect 적용 */
public:
    /**
     * @brief 새 Effect Context를 생성한다(적용 주체/인과 정보를 담는 컨테이너).
     * @return 생성된 TacticalEffectContext.
     */
    virtual UTacticalEffectContext* MakeEffectContext() const;
    /**
     * @brief 주어진 Effect 클래스로부터 외부에 적용할 Spec을 생성한다.
     * @param EffectClass Spec의 기반이 될 Effect 정의 클래스.
     * @param Context 적용에 사용할 Effect Context.
     * @return 생성된 Effect Spec(공유 포인터).
     */
    virtual TSharedPtr<FTacticalEffectSpec> MakeOutgoingSpec(TSubclassOf<UTacticalEffect> EffectClass, UTacticalEffectContext* Context) const;

    /**
     * @brief Effect Spec을 지정 대상 모델에 적용한다.
     * @param Spec 적용할 Effect Spec.
     * @param Target 적용 대상 모델.
     * @return 적용 결과로 생성된 활성 Effect 핸들.
     */
    virtual FActiveTacticalEffectHandle ApplyTacticalEffectSpecToTarget(const FTacticalEffectSpec& Spec, UAttributeSetComponentModel* Target);
    /**
     * @brief Effect Spec을 자기 자신에게 적용한다.
     * @param Spec 적용할 Effect Spec.
     * @return 적용 결과로 생성된 활성 Effect 핸들.
     */
    virtual FActiveTacticalEffectHandle ApplyTacticalEffectSpecToSelf(const FTacticalEffectSpec& Spec);

    /**
     * @brief 즉발(Instant) Effect의 모디파이어를 실행하여 속성 기본값에 즉시 반영한다.
     * @param Spec 실행할 Effect Spec.
     */
    void ExecuteTacticalEffect(FTacticalEffectSpec& Spec);

    /**
     * @brief 활성 Effect 핸들을 이 모델의 활성 컨테이너에 등록(이동)한다.
     * @param ActiveHandle 등록할 활성 Effect 핸들(rvalue, 소유권 이동).
     * @return 컨테이너에 등록된 후의 핸들.
     */
    virtual FActiveTacticalEffectHandle SetActiveTacticalEffect(FActiveTacticalEffectHandle&& ActiveHandle);
    /**
     * @brief 활성 Effect를 제거한다.
     * @param Handle 제거할 활성 Effect 핸들.
     * @param StacksToRemove 제거할 스택 수(-1이면 전체 제거).
     * @return 제거에 성공하면 true.
     */
    virtual bool RemoveActiveTacticalEffect(FActiveTacticalEffectHandle Handle, int32 StacksToRemove = -1);

    /**
     * @brief 활성 Effect 핸들에 대응하는 Effect 정의(CDO)를 반환한다.
     * @param Handle 조회할 활성 Effect 핸들.
     * @return 해당 Effect 정의(없으면 nullptr).
     */
    const UTacticalEffect* GetTacticalEffectDefForHandle(FActiveTacticalEffectHandle Handle);

    /* Tag 연관 */
public:
    /**
     * @brief (IGameplayTagAssetInterface) 단일 태그(부모 포함)를 보유하는지 검사한다.
     * @param TagToCheck 검사할 태그.
     * @return 보유하면 true.
     */
    inline bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override
    {
        return mTacticalTagCountContainer.HasMatchingGameplayTag(TagToCheck);
    }

    /**
     * @brief (IGameplayTagAssetInterface) 컨테이너의 모든 태그를 보유하는지 검사한다.
     * @param TagContainer 검사할 태그 집합.
     * @return 전부 보유하면 true.
     */
    inline bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override
    {
        return mTacticalTagCountContainer.HasAllMatchingGameplayTags(TagContainer);
    }

    /**
     * @brief (IGameplayTagAssetInterface) 컨테이너의 태그 중 하나라도 보유하는지 검사한다.
     * @param TagContainer 검사할 태그 집합.
     * @return 하나라도 보유하면 true.
     */
    inline bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override
    {
        return mTacticalTagCountContainer.HasAnyMatchingGameplayTags(TagContainer);
    }

    /**
     * @brief (IGameplayTagAssetInterface) 소유 태그 전부를 출력 컨테이너로 복사한다.
     * @param[out] TagContainer 결과를 담을 컨테이너(먼저 리셋된다).
     */
    inline void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override
    {
        TagContainer.Reset();
        TagContainer.AppendTags(GetOwnedGameplayTags());
    }

    /** @brief 명시적으로 보유 중인 게임플레이 태그 컨테이너를 반환한다(복사 없이 참조 반환). */
    inline const FGameplayTagContainer& GetOwnedGameplayTags() const
    {
        return mTacticalTagCountContainer.GetExplicitGameplayTags();
    }

    /**
     * @brief 지정 태그의 누적 카운트를 반환한다(부모 태그 포함 집계값).
     * @param TagToCheck 조회할 태그.
     * @return 누적 카운트.
     */
    inline int32 GetTagCount(FGameplayTag TagToCheck) const
    {
        return mTacticalTagCountContainer.GetTagCount(TagToCheck);
    }

    /**
     * @brief 태그 카운트를 지정 절대값으로 설정한다(델타로 환산하여 갱신).
     * @param Tag 대상 태그.
     * @param NewCount 설정할 절대 카운트.
     */
    inline void SetTagMapCount(const FGameplayTag& Tag, int32 NewCount)
    {
        // 현재 명시적 카운트와의 차이(델타)를 계산하여 단일 갱신 경로로 위임한다.
        const int32 CurrentCount = mTacticalTagCountContainer.GetExplicitTagCount(Tag);
        UpdateTagMapSingle_Internal(Tag, NewCount - CurrentCount);
    }

    /**
     * @brief 단일 태그 카운트를 델타만큼 증감한다.
     * @param BaseTag 대상 태그.
     * @param CountDelta 증감량(음수면 감소).
     */
    inline void UpdateTagMap(const FGameplayTag& BaseTag, int32 CountDelta)
    {
        UpdateTagMapSingle_Internal(BaseTag, CountDelta);
    }

    /**
     * @brief 태그 컨테이너 전체의 카운트를 델타만큼 증감한다.
     * @param Container 대상 태그 집합.
     * @param CountDelta 증감량(음수면 감소).
     */
    inline void UpdateTagMap(const FGameplayTagContainer& Container, int32 CountDelta)
    {
        if (!Container.IsEmpty()) // 빈 컨테이너는 갱신/알림 비용을 피하기 위해 조기 반환.
        {
            UpdateTagMap_Internal(Container, CountDelta);
        }
    }

    /**
     * @brief Loose 게임플레이 태그를 추가한다(Count만큼 카운트 증가).
     * @param GameplayTag 추가할 태그.
     * @param Count 증가량(기본 1).
     */
    inline void AddLooseGameplayTag(const FGameplayTag& GameplayTag, int32 Count = 1)
    {
        UpdateTagMap(GameplayTag, Count);
    }

    /**
     * @brief 여러 Loose 게임플레이 태그를 한 번에 추가한다.
     * @param GameplayTags 추가할 태그 집합.
     * @param Count 각 태그 증가량(기본 1).
     */
    inline void AddLooseGameplayTags(const FGameplayTagContainer& GameplayTags, int32 Count = 1)
    {
        UpdateTagMap(GameplayTags, Count);
    }

    /**
     * @brief Loose 게임플레이 태그를 제거한다(Count만큼 카운트 감소).
     * @param GameplayTag 제거할 태그.
     * @param Count 감소량(기본 1).
     */
    inline void RemoveLooseGameplayTag(const FGameplayTag& GameplayTag, int32 Count = 1)
    {
        UpdateTagMap(GameplayTag, -Count); // 음수 델타로 감소시킨다.
    }

    /**
     * @brief 여러 Loose 게임플레이 태그를 한 번에 제거한다.
     * @param GameplayTags 제거할 태그 집합.
     * @param Count 각 태그 감소량(기본 1).
     */
    inline void RemoveLooseGameplayTags(const FGameplayTagContainer& GameplayTags, int32 Count = 1)
    {
        UpdateTagMap(GameplayTags, -Count); // 음수 델타로 감소시킨다.
    }

    /**
     * @brief Loose 게임플레이 태그의 카운트를 지정 절대값으로 설정한다.
     * @param GameplayTag 대상 태그.
     * @param NewCount 설정할 절대 카운트.
     */
    inline void SetLooseGameplayTagCount(const FGameplayTag& GameplayTag, int32 NewCount)
    {
        SetTagMapCount(GameplayTag, NewCount);
    }

public:
    /**
     * @brief 태그 카운트 변경 이벤트를 등록한다.
     * @param Tag 구독할 태그.
     * @param EventType 알림 트리거 종류(추가/제거, 카운트 변경 등).
     * @return 등록된 이벤트 델리게이트 참조.
     */
    FOnTacticalEffectTagCountChanged& RegisterTacticalTagEvent(FGameplayTag Tag, EGameplayTagEventType::Type EventType=EGameplayTagEventType::NewOrRemoved);
    /**
     * @brief 등록했던 태그 카운트 변경 이벤트를 해제한다.
     * @param DelegateHandle 등록 시 받은 델리게이트 핸들.
     * @param Tag 대상 태그.
     * @param EventType 등록 시 사용한 이벤트 종류.
     * @return 해제에 성공하면 true.
     */
	bool UnregisterTacticalTagEvent(FDelegateHandle DelegateHandle, FGameplayTag Tag, EGameplayTagEventType::Type EventType=EGameplayTagEventType::NewOrRemoved);

protected:
    /**
     * @brief (내부) 스택 카운트 변경에 따른 태그맵 변경 알림을 발행한다.
     * @param Container 변경된 태그 집합.
     */
    void NotifyTagMap_StackCountChange(const FGameplayTagContainer& Container);
    /**
     * @brief (내부) 단일 태그 카운트를 델타만큼 갱신하고 필요한 알림을 발행한다.
     * @param Tag 대상 태그.
     * @param CountDelta 증감량.
     */
    void UpdateTagMapSingle_Internal(const FGameplayTag& Tag, int32 CountDelta);
    /**
     * @brief (내부) 태그 컨테이너 전체 카운트를 델타만큼 갱신하고 필요한 알림을 발행한다.
     * @param Container 대상 태그 집합.
     * @param CountDelta 증감량.
     */
    void UpdateTagMap_Internal(const FGameplayTagContainer& Container, int32 CountDelta);

    /**
     * @brief 태그 추가/제거 상태가 갱신될 때 호출되는 가상 훅(파생 클래스 확장 지점).
     * @param Tag 변경된 태그.
     * @param TagExists 갱신 후 해당 태그가 존재하면 true.
     */
    virtual void OnTagUpdated(const FGameplayTag& Tag, bool TagExists);

    /* 변경 알림 */
public:
    /**
     * 속성 값이 변경될 경우 실행
     * @param Aggregator 해당 속성의 계산 객체
     * @param Attribute 변경 속성
     */
    void OnAttributeAggregatorDirty(FTacticalAggregator* Aggregator, FTacticalAttribute Attribute);
    /**
     * 속성 값에 의존하는 Effect에게 전파를 위해 실행
     * @param Handle 대상 Effect 핸들
     * @param ChangedAggregator 해당 속성의 계산 객체
     */
    void OnMagnitudeDependencyChange(FActiveTacticalEffectHandle Handle, const FTacticalAggregator* ChangedAggregator);

public:
    /** Effect를 자기 자신에게 적용했을 때 브로드캐스트되는 델리게이트. */
    FOnTacticalEffectAppliedDelegate OnTacticalEffectAppliedDelegateToSelf;
    /** Effect를 다른 대상에게 적용했을 때 브로드캐스트되는 델리게이트. */
    FOnTacticalEffectAppliedDelegate OnTacticalEffectAppliedDelegateToTarget;
    /** 활성(지속) Effect가 자기 자신에게 추가되었을 때 브로드캐스트되는 델리게이트. */
    FOnTacticalEffectAppliedDelegate OnActiveTacticalEffectAddedDelegateToSelf;

protected:
    /** 이 모델에 현재 활성화된(지속) Effect들을 보관하는 컨테이너. 핸들 발급/스택/제거를 관리한다. */
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "ActiveAttributeEffects"))
    FActiveTacticalEffectsContainer mActiveAttributeEffects;

protected:
    /** 이 모델이 스폰하여 소유 중인 모든 TacticalAttributeSet 목록. */
    UPROPERTY(Category = "AttributeSet", VisibleAnywhere, meta = (DisplayName = "SpawnedAttributes"))
    TArray<TObjectPtr<UTacticalAttributeSet>> mSpawnedAttributes;

protected:
    /** 게임플레이 태그별 누적 카운트와 변경 이벤트를 관리하는 컨테이너(GAS의 FGameplayTagCountContainer 대체). */
    UPROPERTY(Category = "Tag", VisibleAnywhere, meta = (DisplayName = "TacticalTagCountContainer"))
    FTacticalTagCountContainer mTacticalTagCountContainer;
};
