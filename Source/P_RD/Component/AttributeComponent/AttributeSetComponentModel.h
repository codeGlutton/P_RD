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
struct FAttributeChangeData;

// USaveGameSubsystem 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogAttributeSetComp, Log, All)

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChangeAttributeValue, const FAttributeChangeData& /*ChangeData*/);

/**
 * @brief 수치 변경 결과 데이터
 */
USTRUCT()
struct FAttributeChangeData
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
struct FActiveAttributeEffectHandle
{
    GENERATED_BODY()

public:
    FActiveAttributeEffectHandle() = default;
    FActiveAttributeEffectHandle(int32 Index, UAttributeSetComponentModel* OwningModel);

public:
    static FActiveAttributeEffectHandle GenerateNewHandle(UAttributeSetComponentModel* OwningModel);

public:
    bool operator==(const FActiveAttributeEffectHandle& Other) const
    {
        return mIndex == Other.mIndex;
    }

    bool operator!=(const FActiveAttributeEffectHandle& Other) const
    {
        return mIndex != Other.mIndex;
    }

    friend uint32 GetTypeHash(const FActiveAttributeEffectHandle& Other)
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
    UPROPERTY()
    int32 mIndex = INDEX_NONE;

    UPROPERTY()
    TWeakObjectPtr<UAttributeSetComponentModel> mOwningModel;
};

/**
 * @brief 현재 활성화 이펙트
 */
USTRUCT()
struct FActiveAttributeEffect
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FActiveAttributeEffectHandle Handle;

    // UPROPERTY()
    // FGameplayEffectSpec mSpec;
};

UGameplayEffect

/**
 * @brief 하나의 변화값
 */
struct FAttributeAggregatorMod
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
struct FAttributeAggregator : public TSharedFromThis<FAttributeAggregator>
{
    friend struct FActiveAttributeEffectsContainer;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnAttributeAggregatorDirty, FAttributeAggregator*);

public:
    float GetAttributeBaseValue() const;
    void SetAttributeBaseValue(float BaseValue, bool BroadcastDirtyEvent = true);

private:
    void BroadcastOnDirty();

public:
    // @brief 값이 변경됨을 알리는 대리자
    FOnAttributeAggregatorDirty OnDirty;
    int32 mDirtyCount;

private:
    // @brief 베이스 값
    float mBaseValue;
    // @brief 각 연산자에 따른 결과값들
    TArray<FAttributeAggregatorMod> Mods[EGameplayModOp::Max];
};

/**
 * @brief 현재 활성화 이펙트들과 그에 따른 결과를 보유한 객체
 */
USTRUCT()
struct FActiveAttributeEffectsContainer
{
    GENERATED_BODY()

    friend class UAttributeSetComponentModel;

private:
    void CleanupAttributeAggregator(const FGameplayAttribute& Attribute);

public:
    // @brief 해당 객체를 소유한 컴포넌트 모델
    UPROPERTY()
    TWeakObjectPtr<UAttributeSetComponentModel> mOwner;

private:
    // @brief 활성화 중인 이펙트들
    UPROPERTY()
    TArray<FActiveAttributeEffect> mAttributeEffects;

    /* 이펙트를 확인하여 복구 가능한 객체들 */
private:
    // @brief 속성에 따른 값 계산기
    TMap<FGameplayAttribute, TSharedPtr<FAttributeAggregator>> mAttributeAggregatorMap;

    // @brief 값 변경에 따른 대리자
    TMap<FGameplayAttribute, FOnChangeAttributeValue> mAttributeValueChangeDelegates;
//
//    TMap<TWeakObjectPtr<UGameplayEffect>, TArray<FActiveAttributeEffectHandle>> SourceStackingMap;
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

    /* 현재값 세팅 */
public:
    float GetAttributeCurrentValue(const FGameplayAttribute& Attribute) const;

public:
    void OnAttributeAggregatorDirty(FAggregator* Aggregator, FGameplayAttribute Attribute, bool FromRecursiveCall = false);

protected:
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "ActiveAttributeEffects"))
    FActiveAttributeEffectsContainer mActiveAttributeEffects;

protected:
    UPROPERTY(Category = "AttributeSet", VisibleAnywhere, meta = (DisplayName = "SpawnedAttributes"))
    TArray<TObjectPtr<UAttributeSet>> mSpawnedAttributes;
};
