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

class UAttributeSetComponentModel;
struct FAttributeChangeData;

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
    FActiveAttributeEffectHandle(int32 Index);

public:
    static FActiveAttributeEffectHandle GenerateNewHandle(UAttributeSetComponentModel* OwningComponent);

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

/**
 * @brief 현재 활성화 이펙트들과 그에 따른 결과를 보유한 객체
 */
USTRUCT()
struct FActiveAttributeEffectsContainer
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TWeakObjectPtr<UAttributeSetComponentModel> mOwner;

private:
    // @brief 활성화 중인 이펙트들
    UPROPERTY()
    TArray<FActiveAttributeEffect> mAttributeEffects;

//    UPROPERTY()
//    TMap<FGameplayAttribute, FAggregatorRef> mAttributeAggregatorMap;

    // @brief 값 변경에 따른 대리자
    TMap<FGameplayAttribute, FOnChangeAttributeValue> mAttributeValueChangeDelegates;
//
//    UPROPERTY()
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

public:
	/*
    * @brief 해당하는 속성의 값을 가져온다.
    */
    float GetAttributeValue(FGameplayAttribute Attribute) const;
    void SetAttributeValue(FGameplayAttribute Attribute, float NewValue);

protected:
    UPROPERTY()
    FActiveAttributeEffectsContainer mActiveAttributeEffects;
};
