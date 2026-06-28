/*****************************************************************//**
 * @file   TacticalAggregator.h
 * @brief  속성 값에 대한 모든 변화 계산기 객체 정의 헤더
 * @author 모호재
 * @date   2026-06-23
 *********************************************************************/

/**
 * @file   TacticalAggregator.h
 * @brief  하나의 속성(Attribute)에 적용되는 모든 모디파이어를 누적/평가하는 Aggregator 정의.
 *
 * [PR #191 마이그레이션 맥락]
 *  본 PR은 효과 모디파이어의 "연산 종류(operation)" enum을 GAS의 EGameplayModOp에서
 *  자체 정의한 ETacticalModOp(TacticalEffectType.h)로 치환하는 GAS 폐기 작업의 일부다.
 *  이 파일에서 모디파이어 연산자를 받는 모든 지점(예: ExecModOnBaseValue, AddAggregatorMod 등)이
 *  TEnumAsByte<EGameplayModOp::Type> → TEnumAsByte<ETacticalModOp::Type> 로 바뀌었다.
 *
 * [ETacticalModOp의 정수값을 구 EGameplayModOp와 동일하게 유지하는 이유 3가지]
 *  (1) 직렬화 호환: 기존 에셋/세이브 데이터에 정수값(byte)으로 박혀 있는 연산자 값을 그대로 보존하기 위함.
 *      enum 이름만 바뀌고 정수값이 달라지면 기존 데이터가 엉뚱한 연산으로 해석된다.
 *  (2) 배열 인덱싱: 아래 mMods[ETacticalModOp::Max] 처럼 연산자 값(op)을 그대로 배열 인덱스로 사용한다.
 *      각 모디파이어는 자신의 op 값에 해당하는 버킷(mMods[op])에 분류되어 누적된다.
 *  (3) CoreRedirect: DefaultEngine.ini의 CoreRedirect가 구 enum 이름(EGameplayModOp)을
 *      신 enum 이름(ETacticalModOp)으로 매핑한다. 정수값이 같아야 이 이름 매핑만으로 안전하게 치환된다.
 *
 * [ETacticalModOp 연산 값 대응표 (정수값 = 구 EGameplayModOp 값)]
 *  - AddBase(0)           : 합산.        Base에 EvaluatedMagnitude를 더함.           (구 GAS Additive)
 *  - MultiplyAdditive(1)  : 배율 가산.    여러 배율을 (1 + sum)으로 합산해 곱함.       (구 GAS Multiplicitive)
 *  - DivideAdditive(2)    : 나눗셈 가산.  여러 제수를 (1 + sum)으로 합산해 나눔.        (구 GAS Division)
 *  - Override(3)          : 덮어쓰기.     최종값을 EvaluatedMagnitude로 강제 대체.     (구 GAS Override)
 *  - MultiplyCompound(4)  : 거듭제곱 곱.  각 배율을 (1 + mag)으로 보고 모두 곱셈 복리.
 *  - AddFinal(5)          : 최종 합산.    위 모든 연산 이후 마지막에 더해지는 보정.
 *  - Max(6)               : 무효/개수.    실제 연산이 아니라 enum 원소 개수(=배열 길이)로 사용.
 *  하위호환 별칭: Additive=0 / Multiplicitive=1 / Division=2 / Override=3 (구 GAS 이름 그대로 사용 가능).
 *
 * [연관 유틸: TacticalEffectUtilities (TacticalEffectType.cpp) — 구 GameplayEffectUtilities 대체]
 *  - GetModifierBiasByModifierOp : 연산별 항등값. 곱셈계열(Multiply/Divide/Compound)=1, 합산계열(Add)=0.
 *      SumMods/MultiplyMods가 빈 버킷에서도 올바른 항등원으로 시작하도록 Bias를 공급한다.
 *  - ComputeStackedModifierMagnitude : 스택(중첩) 수에 따른 모디파이어 크기 계산.
 *      MultiplyCompound는 거듭제곱(복리), 그 외는 선형 스케일.
 *  - TacticalModOpToString : 연산자를 디버그용 문자열로 변환.
 *
 * @author 박용수
 * @date   2026-06-26
 */

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
    FTacticalAggregator(UAttributeSetComponentModel* Owner, float InBaseValue = 0.f) :
        mOwner(Owner),
        mDirtyCount(0),
        mBaseValue(InBaseValue)
    {
    }

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

