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

DECLARE_MULTICAST_DELEGATE_TwoParams(FTacticalEventTagMulticastDelegate, FGameplayTag, const FGameplayEventData*);

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

    const TArray<UTacticalAttributeSet*>& GetSpawnedAttributes() const;

public:
    bool HasAttributeSetForAttribute(FTacticalAttribute Attribute) const;
    void AddSpawnedAttributeSet(UTacticalAttributeSet* AttributeSet);
    void RemoveSpawnedAttributeSet(UTacticalAttributeSet* AttributeSet);

protected:
    const UTacticalAttributeSet* GetAttributeSet_Internal(TSubclassOf<UTacticalAttributeSet> Class) const;
    const UTacticalAttributeSet* GetOrCreateAttributeSet_Internal(TSubclassOf<UTacticalAttributeSet> Class);

    /* 캡처 */
public:
    void CaptureAllStates(FBoardCombatTargetSnapshotData& Snapshot) const;

    /* 기본값 설정 */
public:
    void SetAttributeBaseValue(const FTacticalAttribute& Attribute, float BaseValue);
    float GetAttributeBaseValue(const FTacticalAttribute& Attribute) const;

    /* 현재값 설정 */
public:
    float GetAttributeCurrentValue(FTacticalAttribute Attribute, bool& Found) const;
    float GetAttributeCurrentValue(const FTacticalAttribute& Attribute) const;

protected:
    void SetAttributeCurrentValue_Internal(const FTacticalAttribute& Attribute, float& NewValue);

public:
    FOnChangeAttributeValue& GetTacticalAttributeValueChangeDelegate(FTacticalAttribute Attribute);
    void OnTacticalEffectAppliedToTarget(UAttributeSetComponentModel* Model, const FTacticalEffectSpec& SpecApplied, FActiveTacticalEffectHandle ActiveHandle);
    void OnTacticalEffectAppliedToSelf(UAttributeSetComponentModel* Model, const FTacticalEffectSpec& SpecApplied, FActiveTacticalEffectHandle ActiveHandle);

    /* 수정자 적용 */
public:
    void ApplyModToAttribute(const FTacticalAttribute& Attribute, TEnumAsByte<ETacticalModOp::Type> ModifierOp, float ModifierMagnitude);

    /* Effect 적용 */
public:
    virtual UTacticalEffectContext* MakeEffectContext() const;
    virtual TSharedPtr<FTacticalEffectSpec> MakeOutgoingSpec(TSubclassOf<UTacticalEffect> EffectClass, UTacticalEffectContext* Context) const;

    virtual FActiveTacticalEffectHandle ApplyTacticalEffectSpecToTarget(const FTacticalEffectSpec& Spec, UAttributeSetComponentModel* Target);
    virtual FActiveTacticalEffectHandle ApplyTacticalEffectSpecToSelf(const FTacticalEffectSpec& Spec);

    void ExecuteTacticalEffect(FTacticalEffectSpec& Spec);

    virtual FActiveTacticalEffectHandle SetActiveTacticalEffect(FActiveTacticalEffectHandle&& ActiveHandle);
    virtual bool RemoveActiveTacticalEffect(FActiveTacticalEffectHandle Handle, int32 StacksToRemove = -1);

    const UTacticalEffect* GetTacticalEffectDefForHandle(FActiveTacticalEffectHandle Handle);

    /* Tag 연관 */
public:
    inline bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override
    {
        return mTacticalTagCountContainer.HasMatchingGameplayTag(TagToCheck);
    }

    inline bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override
    {
        return mTacticalTagCountContainer.HasAllMatchingGameplayTags(TagContainer);
    }

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

    inline int32 GetTagCount(FGameplayTag TagToCheck) const
    {
        return mTacticalTagCountContainer.GetTagCount(TagToCheck);
    }

    inline void SetTagMapCount(const FGameplayTag& Tag, int32 NewCount)
    {
        const int32 CurrentCount = mTacticalTagCountContainer.GetExplicitTagCount(Tag);
        UpdateTagMapSingle_Internal(Tag, NewCount - CurrentCount);
    }

    inline void UpdateTagMap(const FGameplayTag& BaseTag, int32 CountDelta)
    {
        UpdateTagMapSingle_Internal(BaseTag, CountDelta);
    }

    inline void UpdateTagMap(const FGameplayTagContainer& Container, int32 CountDelta)
    {
        if (!Container.IsEmpty())
        {
            UpdateTagMap_Internal(Container, CountDelta);
        }
    }

    inline void AddLooseGameplayTag(const FGameplayTag& GameplayTag, int32 Count = 1)
    {
        UpdateTagMap(GameplayTag, Count);
    }

    inline void AddLooseGameplayTags(const FGameplayTagContainer& GameplayTags, int32 Count = 1)
    {
        UpdateTagMap(GameplayTags, Count);
    }

    inline void RemoveLooseGameplayTag(const FGameplayTag& GameplayTag, int32 Count = 1)
    {
        UpdateTagMap(GameplayTag, -Count);
    }

    inline void RemoveLooseGameplayTags(const FGameplayTagContainer& GameplayTags, int32 Count = 1)
    {
        UpdateTagMap(GameplayTags, -Count);
    }

    inline void SetLooseGameplayTagCount(const FGameplayTag& GameplayTag, int32 NewCount)
    {
        SetTagMapCount(GameplayTag, NewCount);
    }

public:
    FOnTacticalEffectTagCountChanged& RegisterTacticalTagEvent(FGameplayTag Tag, EGameplayTagEventType::Type EventType=EGameplayTagEventType::NewOrRemoved);
	bool UnregisterTacticalTagEvent(FDelegateHandle DelegateHandle, FGameplayTag Tag, EGameplayTagEventType::Type EventType=EGameplayTagEventType::NewOrRemoved);

protected:
    void NotifyTagMap_StackCountChange(const FGameplayTagContainer& Container);
    void UpdateTagMapSingle_Internal(const FGameplayTag& Tag, int32 CountDelta);
    void UpdateTagMap_Internal(const FGameplayTagContainer& Container, int32 CountDelta);

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
    FOnTacticalEffectAppliedDelegate OnTacticalEffectAppliedDelegateToSelf;
    FOnTacticalEffectAppliedDelegate OnTacticalEffectAppliedDelegateToTarget;
    FOnTacticalEffectAppliedDelegate OnActiveTacticalEffectAddedDelegateToSelf;

protected:
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "ActiveAttributeEffects"))
    FActiveTacticalEffectsContainer mActiveAttributeEffects;

protected:
    UPROPERTY(Category = "AttributeSet", VisibleAnywhere, meta = (DisplayName = "SpawnedAttributes"))
    TArray<TObjectPtr<UTacticalAttributeSet>> mSpawnedAttributes;

protected:
    UPROPERTY(Category = "Tag", VisibleAnywhere, meta = (DisplayName = "TacticalTagCountContainer"))
    FTacticalTagCountContainer mTacticalTagCountContainer;
};
