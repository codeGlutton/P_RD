/*****************************************************************//**
 * @file   TacticalAggregator.h
 * @brief  속성 값에 대한 모든 변화 계산기 객체 정의 헤더
 * @author 모호재
 * @date   2026-06-23
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "TAS/Effect/ActiveTacticalEffect.h"

// @brief Aggregator를 보유/갱신하는 이펙트 컨테이너 (friend 접근용 전방 선언)
struct FActiveTacticalEffectsContainer;
// @brief TAS 프레임워크 모델 (friend 접근용 전방 선언)
class UTacticalFrameworkModel;

/**
 * @brief Dirty 브로드캐스트를 일괄 처리(batch)하기 위한 RAII 스코프 가드.
 *
 * 생성 시점에 World 단위로 "Dirty 누적 모드"를 켜고, 소멸 시점에 한꺼번에
 * OnDirty를 발화시켜 다중 모디파이어 변경 중 중복 재계산/재귀 호출을 줄인다.
 */
struct FScopedTacticalAggregatorOnDirtyBatch
{
public:
    /**
     * @brief 배치 스코프 진입. 대상 World의 Dirty 누적을 시작한다.
     * @param World 배치를 적용할 월드
     */
    FScopedTacticalAggregatorOnDirtyBatch(UWorld* World);
    /** @brief 배치 스코프 종료. 누적된 Dirty를 일괄 발화한다. */
    ~FScopedTacticalAggregatorOnDirtyBatch();

protected:
    // @brief 배치 대상 월드
    TObjectPtr<UWorld> mWorld = nullptr;
};

/**
 * @brief 하나의 변화값(단일 모디파이어). Aggregator의 op별 버킷(mMods[op])에 담기는 원소.
 *
 * 어떤 연산(op)을 적용할지는 이 구조체가 아니라, 이 구조체가 담긴 배열의 인덱스
 * (= ETacticalModOp 값)로 결정된다. 즉 op는 mMods의 인덱스에 인코딩되어 있다.
 */
struct FTacticalAggregatorMod
{
public:
    // @brief 평가된 값. 스펙/속성으로부터 계산된 이 모디파이어의 적용 크기(magnitude).
    float mEvaluatedMagnitude = 0.f;
    // @brief 스택 갯수. 중첩(stack) 수. ComputeStackedModifierMagnitude에서 크기 스케일에 사용.
    float mStackCount = 0.f;

    // @brief 활성화 핸들. 이 모디파이어를 만들어낸 활성 이펙트 인스턴스를 역참조하는 키.
    FActiveTacticalEffectHandle mActiveHandle;
};

/**
 * @brief 하나의 속성(Attribute)에 적용되는 모든 변화(모디파이어)를 누적하고 최종값을 계산하는 객체.
 *
 * 동작 개요:
 *  - 모디파이어는 자신의 연산자(ETacticalModOp 값)에 따라 mMods[op] 버킷에 분류되어 보관된다.
 *  - Evaluate()는 BaseValue에서 출발해 op별 버킷을 정해진 순서(합산 → 배율 → 나눗셈 → ...)로
 *    적용하여 최종 속성값을 산출한다.
 *  - 값이 바뀌면 OnDirty로 의존 이펙트/뷰에 전파한다(배치 가드로 묶을 수 있음).
 *
 * [PR #191] 본 구조체의 모든 연산자 파라미터 타입이 EGameplayModOp::Type → ETacticalModOp::Type 로
 *           치환되었다(GAS 폐기). 정수값 호환 유지에 대한 근거는 파일 상단 헤더 주석 참고.
 */
struct FTacticalAggregator : public TSharedFromThis<FTacticalAggregator>
{
    // @brief 모디파이어 추가/제거 등 내부 상태를 직접 다루는 컨테이너에 friend 접근 허용
    friend struct FActiveTacticalEffectsContainer;
    // @brief 프레임워크 모델에 friend 접근 허용
    friend class UTacticalFrameworkModel;

    // @brief 이 Aggregator의 평가값이 변경되었을 때 발화하는 멀티캐스트 델리게이트 타입
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnAttributeAggregatorDirty, FTacticalAggregator*);

public:
    /**
     * @brief 생성자.
     * @param Owner       이 Aggregator가 속한 속성 셋 컴포넌트 모델
     * @param InBaseValue 모디파이어 적용 이전의 기준값(Base)
     */
    FTacticalAggregator(UAttributeSetComponentModel* Owner, float InBaseValue = 0.f);
    ~FTacticalAggregator();

    /* 계산 함수 */
public:
    /**
     * @brief 모디파이어 적용 전 기준값(Base)을 반환한다.
     * @return 현재 베이스 값
     */
    float GetAttributeBaseValue() const;
    /**
     * @brief 기준값(Base)을 설정한다.
     * @param BaseValue          새 베이스 값
     * @param BroadcastDirtyEvent true면 변경 후 OnDirty를 발화한다
     */
    void SetAttributeBaseValue(float BaseValue, bool BroadcastDirtyEvent = true);

public:
    /**
     * @brief 단일 모디파이어를 BaseValue에 즉시 적용한 결과를 계산하는 정적 헬퍼.
     *        op별 연산 수식:
     *          - AddBase          : BaseValue + EvaluatedMagnitude   (합산)
     *          - MultiplyAdditive : BaseValue * (1 + EvaluatedMagnitude)  (배율 가산)
     *          - DivideAdditive   : EvaluatedMagnitude==0 가드 후 BaseValue / EvaluatedMagnitude (나눗셈)
     *          - Override         : EvaluatedMagnitude              (덮어쓰기, Base 무시)
     * @param BaseValue          적용 대상 기준값
     * @param ModifierOp         적용할 연산 종류 (구 EGameplayModOp::Type 에서 치환됨)
     * @param EvaluatedMagnitude 모디파이어의 평가된 크기
     * @return 연산 적용 후 값
     */
    static float StaticExecModOnBaseValue(float BaseValue, TEnumAsByte<ETacticalModOp::Type> ModifierOp, float EvaluatedMagnitude);
    /**
     * @brief 자신의 BaseValue에 단일 모디파이어를 직접 적용(in-place)한다.
     * @param ModifierOp         적용할 연산 종류 (ETacticalModOp)
     * @param EvaluatedMagnitude 모디파이어의 평가된 크기
     */
    void ExecModOnBaseValue(TEnumAsByte<ETacticalModOp::Type> ModifierOp, float EvaluatedMagnitude);

    /**
     * @brief 모디파이어를 해당 op 버킷(mMods[ModifierOp])에 추가한다.
     * @param EvaluatedData 평가된 크기(magnitude)
     * @param ModifierOp    연산 종류. 이 값이 곧 mMods 배열 인덱스로 쓰인다 (ETacticalModOp).
     * @param ActiveHandle  이 모디파이어를 만든 활성 이펙트 핸들(제거 시 식별자)
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
     * @param Attribute    재평가 기준이 되는 속성
     * @param Spec         재평가 기준이 되는 이펙트 스펙
     * @param InHandle     갱신 후 적용할 활성 이펙트 핸들
     */
    void UpdateAggregatorMod(FActiveTacticalEffectHandle ActiveHandle, const FTacticalAttribute& Attribute, const FTacticalEffectSpec& Spec, FActiveTacticalEffectHandle InHandle);

public:
    /**
     * @brief 현재 BaseValue와 누적된 모든 모디파이어를 적용해 최종 속성값을 계산한다.
     * @return 평가된 최종값
     */
    float Evaluate() const;

    /**
     * @brief 현재 Aggregator 상태를 변경하지 않고 모디파이어 한 건을 더 적용한 값을 계산한다.
     * @details 프리뷰/조준 범위처럼 실제 이펙트를 적용하면 안 되는 읽기 전용 예측에 사용한다.
     */
    float EvaluateWithAdditionalModifier(TEnumAsByte<ETacticalModOp::Type> ModifierOp, float EvaluatedMagnitude) const;
    /**
     * @brief 외부에서 지정한 Base를 출발점으로 모디파이어를 적용해 평가한다.
     * @param BaseValue 평가 기준이 될 베이스 값
     * @return 평가된 최종값
     */
    float EvaluateWithBase(float BaseValue) const;

    /* 헬퍼 함수 */
public:
    /**
     * @brief 합산계열(AddBase/AddFinal) 버킷의 magnitude 총합을 계산한다.
     * @param Mods 합산할 모디파이어 배열(특정 op 버킷)
     * @param Bias 시작 항등값(합산계열=0). GetModifierBiasByModifierOp가 공급.
     * @return Bias + 모든 mEvaluatedMagnitude 합
     */
    static float SumMods(const TArray<FTacticalAggregatorMod>& Mods, float Bias);
    /**
     * @brief 곱셈계열(MultiplyAdditive/MultiplyCompound 등) 버킷의 배율 곱을 계산한다.
     *        배율은 (1 + magnitude) 형태로 누적 곱해진다(항등값 1에서 시작).
     * @param Mods 곱할 모디파이어 배열(특정 op 버킷)
     * @return 누적 곱 결과 배율
     */
    static float MultiplyMods(const TArray<FTacticalAggregatorMod>& Mods);

    /* 업데이트 함수 */
private:
    // @brief 값 변경을 OnDirty로 전파한다(배치 가드 활성 시 즉시 발화하지 않고 누적될 수 있음).
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
    // @brief 각 연산자에 따른 결과값들.
    //        배열 길이 ETacticalModOp::Max(=6)이며, op 정수값을 그대로 인덱스로 사용한다.
    //        예) mMods[ETacticalModOp::AddBase], mMods[ETacticalModOp::MultiplyAdditive] ...
    //        op 정수값을 구 EGameplayModOp와 동일하게 유지하는 이유가 바로 이 인덱싱 때문이다(헤더 참고).
    TArray<FTacticalAggregatorMod> mMods[ETacticalModOp::Max];
    // @brief 해당 값에 영향을 받는 외부 이펙트들
    TArray<FActiveTacticalEffectHandle>	mDependentEffects;
};

