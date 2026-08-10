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

struct FTacticalAggregator;

DECLARE_MULTICAST_DELEGATE_TwoParams(FTacticalEventTagMulticastDelegate, FGameplayTag, const FTacticalEventData*);

/**
 * @brief 액터 모델에 대한 TacticalAttributeSet, 활성 Effect, 게임플레이 태그 카운트를 관리하는 컴포넌트 모델.
 */
UCLASS()
class P_RD_API UAttributeSetComponentModel : public UComponentModel, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

    friend struct FActiveTacticalEffect;
    friend struct FActiveTacticalEffectsContainer;

    DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnTacticalEffectAppliedDelegate, UAttributeSetComponentModel*, const FTacticalEffectSpec&, FActiveTacticalEffectHandle);

    /* UComponentModel 상속 */
public:
    void Initialize() override;
    void Uninitialize() override;

public:
    void PostDuplicate(bool DuplicateForPIE) override;

public:
    void BeginPlay() override;
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
        return StaticCast<const T*>(GetAttributeSet_Internal(T::StaticClass()));
    }
    /**
     * @brief 지정 타입 T의 AttributeSet을 조회하고, 없으면 새로 생성하여 반환한다.
     * @tparam T 추가할 UTacticalAttributeSet 파생 타입.
     * @return 기존 또는 새로 생성된 AttributeSet 포인터.
     */
    template <typename T>
    const T* AddAttributeSet()
    {
        return StaticCast<const T*>(GetOrCreateAttributeSet_Internal(T::StaticClass()));
    }

    const TArray<UTacticalAttributeSet*>& GetSpawnedAttributes() const;

public:
    /**
     * @brief 지정 속성을 담당하는 AttributeSet이 존재하는지 검사한다.
     * @param Attribute 검사할 전술 속성.
     * @return 해당 속성을 보유한 AttributeSet이 있으면 true.
     */
    bool HasAttributeSetForAttribute(FTacticalAttribute Attribute) const;
    /**
     * @brief 스폰된 AttributeSet 목록에 등록한다. 
     * @param AttributeSet 추가할 AttributeSet.
     */
    void AddSpawnedAttributeSet(UTacticalAttributeSet* AttributeSet);
    /**
     * @brief 스폰된 AttributeSet 목록에서 제거한다. 
     * @param AttributeSet 제거할 AttributeSet.
     */
    void RemoveSpawnedAttributeSet(UTacticalAttributeSet* AttributeSet);

protected:
    const UTacticalAttributeSet* GetAttributeSet_Internal(TSubclassOf<UTacticalAttributeSet> Class) const;
    const UTacticalAttributeSet* GetOrCreateAttributeSet_Internal(TSubclassOf<UTacticalAttributeSet> Class);

    /* 캡처 */
public:
    /**
     * @brief 현재 모든 속성/상태를 스냅샷에 캡처한다.
     * @param Snapshot 출력 스냅샷 데이터.
     */
    void CaptureAllStates(UBoardCombatTargetSnapshotData* Snapshot) const;

    /* 기본값 설정 */
public:
    /**
     * @brief 속성의 기본값(BaseValue)을 설정한다.
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
     * @brief 속성의 현재값을 조회한다.
     * @param Attribute 대상 속성.
     * @param Found 해당 속성을 찾았으면 true로 설정된다.
     * @return 현재값
     */
    float GetAttributeCurrentValue(FTacticalAttribute Attribute, bool& Found) const;
    /**
     * @brief 속성의 현재값을 조회한다
     * @param Attribute 대상 속성.
     * @return 현재값
     */
    float GetAttributeCurrentValue(const FTacticalAttribute& Attribute) const;

protected:
    void SetAttributeCurrentValue_Internal(const FTacticalAttribute& Attribute, float& NewValue);

public:
    /**
     * @brief 지정 속성의 "현재값 변경" 델리게이트를 반환한다
     * @param Attribute 대상 속성
     * @return 값 변경 델리게이트
     */
    FOnChangeAttributeValue& GetTacticalAttributeValueChangeDelegate(FTacticalAttribute Attribute);
    /**
     * @brief 이 모델이 "다른 대상에게" Effect를 적용했을 때 호출되는 콜백.
     */
    void OnTacticalEffectAppliedToTarget(UAttributeSetComponentModel* Model, const FTacticalEffectSpec& SpecApplied, FActiveTacticalEffectHandle ActiveHandle);
    /**
     * @brief 이 모델이 "자기 자신에게" Effect를 적용했을 때 호출되는 콜백.
     */
    void OnTacticalEffectAppliedToSelf(UAttributeSetComponentModel* Model, const FTacticalEffectSpec& SpecApplied, FActiveTacticalEffectHandle ActiveHandle);

    /* 수정자 적용 */
public:
    /**
     * @brief 단일 모디파이어를 속성에 즉시 적용한다.
     * @param Attribute 대상 속성.
     * @param ModifierOp 적용할 연산 종류
     * @param ModifierMagnitude 모디파이어 크기
     */
    void ApplyModToAttribute(const FTacticalAttribute& Attribute, TEnumAsByte<ETacticalModOp::Type> ModifierOp, float ModifierMagnitude);

    /* Effect 적용 */
public:
    /**
     * @brief 새 Effect Context를 생성한다.
     * @return 생성된 TacticalEffectContext
     */
    virtual UTacticalEffectContext* MakeEffectContext() const;
    /**
     * @brief 주어진 Effect 클래스로부터 외부에 적용할 Spec을 생성한다.
     * @param EffectClass Spec의 기반이 될 Effect 정의 클래스.
     * @param Context 적용에 사용할 Effect Context.
     * @return 생성된 Effect Spec
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
     * @param ActiveHandle 등록할 활성 Effect 핸들
     * @return 컨테이너에 등록된 후의 핸들
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

    /**
     * @brief 활성 Effect 핸들에 대응하는 Active Effect 객체를 반환한다.
     * @param Handle 조회할 활성 Effect 핸들.
     * @return Active Effect 객체(없으면 nullptr).
     */
    const FActiveTacticalEffect* GetActiveTacticalEffect(const FActiveTacticalEffectHandle Handle) const;

public:
    /**
     * @brief Duration 만기 체크
     */
    void CheckDurationExpired(const int32 Time, ETacticalEffectDurationUnitType UnitType);

    /**
     * @brief 활성 Effect이 Duration이 변경되었을 때 호출된다.
     * @param ActiveEffect Duration이 변경된 Effect
     */
    virtual void OnTacticalEffectDurationChange(FActiveTacticalEffect& ActiveEffect);

public:
    /**
     * @brief 특정 Effect의 남은 시간 가져오기
     */
    int32 GetActiveEffectsTimeRemaining(const FActiveTacticalEffectHandle Handle) const;
    /**
     * @brief 특정 Effect의 Duration 가져오기
     */
    int32 GetActiveEffectsDuration(const FActiveTacticalEffectHandle Handle) const;

    /* Tag 연관 */
public:
    /**
     * @brief 단일 태그(부모 포함)를 보유하는지 검사한다.
     * @param TagToCheck 검사할 태그.
     * @return 보유여부
     */
    inline bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override
    {
        return mTacticalTagCountContainer.HasMatchingGameplayTag(TagToCheck);
    }

    /**
     * @brief 컨테이너의 모든 태그를 보유하는지 검사한다.
     * @param TagContainer 검사할 태그 집합.
     * @return 전부 보유 여부
     */
    inline bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override
    {
        return mTacticalTagCountContainer.HasAllMatchingGameplayTags(TagContainer);
    }

    /**
     * @brief 컨테이너의 태그 중 하나라도 보유하는지 검사한다.
     * @param TagContainer 검사할 태그 집합.
     * @return 하나 이상 보유 여부.
     */
    inline bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override
    {
        return mTacticalTagCountContainer.HasAnyMatchingGameplayTags(TagContainer);
    }

    inline void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override
    {
        TagContainer.Reset();
        TagContainer.AppendTags(GetOwnedGameplayTags());
    }
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

    void RemoveLooseGameplayTagsMatchingTag(const FGameplayTag& GameplayTag, int32 Count = 1);

public:
    /**
     * @brief 태그 카운트 변경 이벤트를 등록한다.
     * @param Tag 구독할 태그.
     * @param EventType 알림 트리거 종류(추가/제거, 카운트 변경 등).
     * @return 등록된 이벤트 대리자
     */
    FOnTacticalEffectTagCountChanged& RegisterTacticalTagEvent(FGameplayTag Tag, ETacticalTagEventType::Type EventType= ETacticalTagEventType::NewOrRemoved);
    /**
     * @brief 등록했던 태그 카운트 변경 이벤트를 해제한다.
     * @param DelegateHandle 등록 시 받은 델리게이트 핸들.
     * @param Tag 대상 태그.
     * @param EventType 등록 시 사용한 이벤트 종류.
     * @return 해제에 성공하면 true.
     */
	bool UnregisterTacticalTagEvent(FDelegateHandle DelegateHandle, FGameplayTag Tag, ETacticalTagEventType::Type EventType= ETacticalTagEventType::NewOrRemoved);

protected:
    void NotifyTagMap_StackCountChange(const FGameplayTagContainer& Container);

    void UpdateTagMapSingle_Internal(const FGameplayTag& Tag, int32 CountDelta);
    void UpdateTagMap_Internal(const FGameplayTagContainer& Container, int32 CountDelta);

    virtual void OnTagUpdated(const FGameplayTag& Tag, bool TagExists);

    /* 변경 알림 */
public:
    /**
     * @brief 속성 값이 변경될 경우 실행
     * @param Aggregator 해당 속성의 계산 객체
     * @param Attribute 변경 속성
     */
    void OnAttributeAggregatorDirty(FTacticalAggregator* Aggregator, FTacticalAttribute Attribute);
    /**
     * @brief 속성 값에 의존하는 Effect에게 전파를 위해 실행
     * @param Handle 대상 Effect 핸들
     * @param ChangedAggregator 해당 속성의 계산 객체
     */
    void OnMagnitudeDependencyChange(FActiveTacticalEffectHandle Handle, const FTacticalAggregator* ChangedAggregator);

public:
    // @brief Effect를 자기 자신에게 적용했을 때 호출되는 대리자
    FOnTacticalEffectAppliedDelegate OnTacticalEffectAppliedDelegateToSelf;
    // @brief Effect를 다른 대상에게 적용했을 때 호출되는 대리자
    FOnTacticalEffectAppliedDelegate OnTacticalEffectAppliedDelegateToTarget;
    // @brief 활성(지속) Effect가 자기 자신에게 추가되었을 때 호출되는 대리자
    FOnTacticalEffectAppliedDelegate OnActiveTacticalEffectAddedDelegateToSelf;

protected:
    // @brief 활성화 중인 Effect 모음
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "ActiveAttributeEffects"))
    FActiveTacticalEffectsContainer mActiveAttributeEffects;

protected:
    // @brief 소유한 속성 집합 목록
    UPROPERTY(Category = "AttributeSet", VisibleAnywhere, meta = (DisplayName = "SpawnedAttributes"))
    TArray<TObjectPtr<UTacticalAttributeSet>> mSpawnedAttributes;

protected:
    // @brief 활성화 중인 Tag 모음
    UPROPERTY(Category = "Tag", VisibleAnywhere, meta = (DisplayName = "TacticalTagCountContainer"))
    FTacticalTagCountContainer mTacticalTagCountContainer;
};
