/*****************************************************************//**
 * @file   TacticalAggregator.h
 * @brief  속성 값에 대한 모든 변화 계산기 객체 정의 헤더
 * @author 모호재
 * @date   2026-06-23
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "TAS/Effect/ActiveTacticalEffect.h"

struct FActiveTacticalEffectsContainer;
class UTacticalFrameworkModel;

struct FScopedTacticalAggregatorOnDirtyBatch
{
public:
    FScopedTacticalAggregatorOnDirtyBatch(UWorld* World);
    ~FScopedTacticalAggregatorOnDirtyBatch();

protected:
    TObjectPtr<UWorld> mWorld = nullptr;
};

/**
 * @brief 하나의 변화값
 */
struct FTacticalAggregatorMod
{
public:
    // @brief 평가된 값
    float mEvaluatedMagnitude = 0.f;
    // @brief 스택 갯수
    float mStackCount = 0.f;

    // @brief 활성화 핸들
    FActiveTacticalEffectHandle mActiveHandle;
};

/**
 * @brief 속성 값에 대한 모든 변화 계산기 객체
 */
struct FTacticalAggregator : public TSharedFromThis<FTacticalAggregator>
{
    friend struct FActiveTacticalEffectsContainer;
    friend class UTacticalFrameworkModel;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnAttributeAggregatorDirty, FTacticalAggregator*);

public:
    FTacticalAggregator(UAttributeSetComponentModel* Owner, float InBaseValue = 0.f) :
        mOwner(Owner),
        mDirtyCount(0),
        mBaseValue(InBaseValue)
    {
    }

    ~FTacticalAggregator();

    /* 계산 함수 */
public:
    float GetAttributeBaseValue() const;
    void SetAttributeBaseValue(float BaseValue, bool BroadcastDirtyEvent = true);

public:
    static float StaticExecModOnBaseValue(float BaseValue, TEnumAsByte<EGameplayModOp::Type> ModifierOp, float EvaluatedMagnitude);
    void ExecModOnBaseValue(TEnumAsByte<EGameplayModOp::Type> ModifierOp, float EvaluatedMagnitude);

    void AddAggregatorMod(float EvaluatedData, TEnumAsByte<EGameplayModOp::Type> ModifierOp, FActiveTacticalEffectHandle ActiveHandle = FActiveTacticalEffectHandle());
    void RemoveAggregatorMod(FActiveTacticalEffectHandle ActiveHandle);
    void UpdateAggregatorMod(FActiveTacticalEffectHandle ActiveHandle, const FGameplayAttribute& Attribute, const FTacticalEffectSpec& Spec, FActiveTacticalEffectHandle InHandle);

public:
    /**
     * 현재 값 계산
     */
    float Evaluate() const;
    float EvaluateWithBase(float BaseValue) const;

    /* 헬퍼 함수 */
public:
    static float SumMods(const TArray<FTacticalAggregatorMod>& Mods, float Bias);
    static float MultiplyMods(const TArray<FTacticalAggregatorMod>& Mods);

    /* 업데이트 함수 */
private:
    void BroadcastOnDirty();

public:
    TWeakObjectPtr<UAttributeSetComponentModel> mOwner;

public:
    // @brief 값이 변경됨을 알리는 대리자
    FOnAttributeAggregatorDirty OnDirty;
    // @brief 재귀적 호출 방지용 카운팅
    int32 mDirtyCount = 0;

private:
    // @brief 베이스 값
    float mBaseValue = 0.f;
    // @brief 각 연산자에 따른 결과값들
    TArray<FTacticalAggregatorMod> mMods[EGameplayModOp::Max];
    // @brief 해당 값에 영향을 받는 외부 이펙트들
    TArray<FActiveTacticalEffectHandle>	mDependentEffects;
};

