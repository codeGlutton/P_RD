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

/**
 * @brief Dirty 브로드캐스트를 일괄 처리(batch)하기 위한 RAII 스코프 가드.
 * @details
 * 생성 시점에 World 단위로 "Dirty 누적 모드"를 켜고, 소멸 시점에 한꺼번에
 * OnDirty를 발화시켜 다중 모디파이어 변경 중 중복 재계산/재귀 호출을 줄인다.
 */
struct FScopedTacticalAggregatorOnDirtyBatch
{
public:
    FScopedTacticalAggregatorOnDirtyBatch(UWorld* World);
    ~FScopedTacticalAggregatorOnDirtyBatch();

protected:
    TObjectPtr<UWorld> mWorld = nullptr;
};

/**
 * @brief 하나의 변화 결과값(단일 모디파이어).
 * @details
 * 하나의 속성 내, 하나의 연산자에 대한 해당 Mod 객체들이 배열로 묶여서 보관된다. 
 * 해당 Mod들은 해당 속성이 변경되어 재연산할 때마다 재참조된다.
 */
struct FTacticalAggregatorMod
{
public:
    // @brief 평가된 값. 스펙/속성으로부터 계산된 이 모디파이어의 적용 크기(magnitude)
    float mEvaluatedMagnitude = 0.f;
    // @brief 스택 갯수. 중첩(stack) 수
    float mStackCount = 0.f;

    // @brief 활성화 핸들
    FActiveTacticalEffectHandle mActiveHandle;
};

/**
 * @brief 하나의 속성(Attribute)에 적용되는 모든 변화(모디파이어)를 누적하고 최종값을 계산하는 객체.
 */
struct FTacticalAggregator : public TSharedFromThis<FTacticalAggregator>
{
    friend struct FActiveTacticalEffectsContainer;
    friend class UTacticalFrameworkModel;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnAttributeAggregatorDirty, FTacticalAggregator*);

public:
    FTacticalAggregator(UAttributeSetComponentModel* Owner, float InBaseValue = 0.f);
    ~FTacticalAggregator();

    /* 계산 함수 */
public:
    /**
     * @brief 모디파이어 적용 전 기본값(Base)을 반환한다.
     * @return 현 속성에 해당하는 기본 값
     */
    float GetAttributeBaseValue() const;
    /**
     * @brief 기본값(Base)을 설정한다.
     * @param BaseValue 새 기본값
     * @param BroadcastDirtyEvent true면 변경 후 OnDirty를 호출할지 여부
     */
    void SetAttributeBaseValue(float BaseValue, bool BroadcastDirtyEvent = true);

public:
    /**
     * @brief 단일 모디파이어를 외부에서 대입한 기본값(Base)에 적용한 결과를 반환하는 함수
     * @param BaseValue 적용 대상 기본값
     * @param ModifierOp 적용할 연산 종류
     * @param EvaluatedMagnitude 모디파이어의 크기
     * @return 연산 적용 후 값
     */
    static float StaticExecModOnBaseValue(float BaseValue, TEnumAsByte<ETacticalModOp::Type> ModifierOp, float EvaluatedMagnitude);
    /**
     * @brief 단일 모디파이어를 현재 기본값(Base)에 적용하는 함수
     * @param ModifierOp 적용할 연산 종류
     * @param EvaluatedMagnitude 모디파이어의 크기
     */
    void ExecModOnBaseValue(TEnumAsByte<ETacticalModOp::Type> ModifierOp, float EvaluatedMagnitude);

    /**
     * @brief 모디파이어를 해당 op 버킷(mMods[ModifierOp])에 추가한다.
     * @param EvaluatedData 평가된 크기(magnitude)
     * @param ModifierOp 연산 종류
     * @param ActiveHandle 이 모디파이어를 만든 활성 이펙트 핸들
     */
    void AddAggregatorMod(float EvaluatedData, TEnumAsByte<ETacticalModOp::Type> ModifierOp, FActiveTacticalEffectHandle ActiveHandle = FActiveTacticalEffectHandle());
    /**
     * @brief 주어진 핸들에 해당하는 모디파이어를 모든 op 버킷에서 제거한다.
     * @param ActiveHandle 제거 대상 활성 이펙트 핸들
     */
    void RemoveAggregatorMod(FActiveTacticalEffectHandle ActiveHandle);
    /**
     * @brief 기존 모디파이어를 새 속성/스펙 기준으로 재평가하여 갱신한다.
     * @param ActiveHandle 갱신 대상(기존) 활성 이펙트 핸들
     * @param Attribute 재평가 기준이 되는 속성
     * @param Spec 재평가 기준이 되는 이펙트 스펙
     * @param InHandle 갱신 후 적용할 활성 이펙트 핸들
     */
    void UpdateAggregatorMod(FActiveTacticalEffectHandle ActiveHandle, const FTacticalAttribute& Attribute, const FTacticalEffectSpec& Spec, FActiveTacticalEffectHandle InHandle);

public:
    /**
     * @brief 현재 BaseValue와 누적된 모든 모디파이어를 적용해 최종 현재 속성값을 계산한다.
     * @return 평가된 최종값
     */
    float Evaluate() const;
    /**
     * @brief 외부에서 지정한 Base를 출발점으로 모디파이어를 적용해 현재 속성값을 평가한다.
     * @param BaseValue 평가 기준이 될 베이스 값
     * @return 평가된 최종값
     */
    float EvaluateWithBase(float BaseValue) const;

    /* 헬퍼 함수 */
public:
    static float SumMods(const TArray<FTacticalAggregatorMod>& Mods, float Bias);
    static float MultiplyMods(const TArray<FTacticalAggregatorMod>& Mods);

    /* 업데이트 함수 */
private:
    /**
     * @brief 값 변경을 OnDirty로 전파
     */
    void BroadcastOnDirty();

public:
    // @brief 이 Aggregator를 소유한 속성 셋 컴포넌트 모델(약참조)
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
    TArray<FTacticalAggregatorMod> mMods[ETacticalModOp::Max];
    // @brief 해당 값에 영향을 받는 외부 이펙트들
    TArray<FActiveTacticalEffectHandle>	mDependentEffects;
};

