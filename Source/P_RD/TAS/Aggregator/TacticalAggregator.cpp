#include "TAS/Aggregator/TacticalAggregator.h"

#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

/**
 * @file TacticalAggregator.cpp
 * @brief 한 속성(Attribute)에 걸린 모든 이펙트 모디파이어를 연산 종류별로 모아 최종 값을 산출하는 Aggregator 구현.
 *
 *        [PR #191 마이그레이션 근거 — GAS 폐기]
 *        이 PR은 효과 모디파이어의 "연산 종류" enum을 GAS의 EGameplayModOp에서
 *        자체 정의 ETacticalModOp(TacticalEffectType.h)로 치환한다. 이 파일은 그 enum을
 *        실제 수식에 적용하는 핵심 지점이므로 변경 폭이 크다.
 *
 *        ETacticalModOp의 정수값은 구 EGameplayModOp와 "동일하게" 유지된다. 그 이유 3가지:
 *          (1) 직렬화 호환 — 기존 에셋/세이브 데이터에 정수로 박혀버린 op 값을 그대로 보존해야
 *              로드 시 의미가 어긋나지 않는다.
 *          (2) 배열 인덱싱 — Aggregator는 mMods[ETacticalModOp::Max] 크기의 배열을 op 값으로
 *              직접 인덱싱(mMods[ModifierOp])한다. 따라서 enum 값은 0..Max-1의 연속 정수여야 한다.
 *          (3) CoreRedirect — DefaultEngine.ini의 CoreRedirect가 구 enum 이름(EGameplayModOp::*)을
 *              새 enum 이름(ETacticalModOp::*)으로 매핑한다. 값이 같아야 리다이렉트가 무결하다.
 *
 *        ETacticalModOp 값 ↔ 의미 ↔ 구 GAS 대응:
 *          AddBase(0)          = 합산              (구 EGameplayModOp::Additive)        / 별칭 Additive
 *          MultiplyAdditive(1) = 배율 가산          (구 EGameplayModOp::Multiplicitive)  / 별칭 Multiplicitive
 *          DivideAdditive(2)   = 나눗셈 가산         (구 EGameplayModOp::Division)        / 별칭 Division
 *          Override(3)         = 덮어쓰기            (구 EGameplayModOp::Override)
 *          MultiplyCompound(4) = 거듭제곱 곱(복리)   (GAS 외 확장)
 *          AddFinal(5)         = 최종 합산           (GAS 외 확장)
 *          Max(6)              = 무효/개수(배열 크기)
 *
 *        최종 산출 수식(EvaluateWithBase 참조):
 *          result = ((Base + Additive) * Multiplicitive / Division * CompoundMultiply) + FinalAdd
 *          단, Override 모디파이어가 하나라도 있으면 그 값으로 즉시 단락(short-circuit).
 */

/**
 * @brief RAII 스코프 가드: 생성~소멸 구간 동안 Aggregator의 Dirty 전파를 배치(batch)로 묶는다.
 * @param World 배치 카운터를 보유한 TacticalFrameworkModel을 찾을 월드.
 *
 * 생성 시 BeginAggregatorDirtyBatch()로 전역 배치 카운트를 올린다. 배치가 열려 있는 동안
 * BroadcastOnDirty()는 즉시 재계산하지 않고 Dirty 목록에만 등록되어, 다수의 모디파이어를
 * 한꺼번에 변경할 때 중복 재계산을 방지한다.
 */
FScopedTacticalAggregatorOnDirtyBatch::FScopedTacticalAggregatorOnDirtyBatch(UWorld* World) :
    mWorld(World)
{
    check(mWorld);

    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mWorld);
    checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

    TacticalFrameworkModel->BeginAggregatorDirtyBatch();
}

/**
 * @brief 스코프 종료 시 배치를 닫는다. 배치가 0으로 떨어지면 누적된 Dirty들을 한 번에 처리한다.
 */
FScopedTacticalAggregatorOnDirtyBatch::~FScopedTacticalAggregatorOnDirtyBatch()
{
    check(mWorld);

    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mWorld);
    checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

    TacticalFrameworkModel->EndAggregatorDirtyBatch();
}

/**
 * @brief 소멸 시 자신이 프레임워크 모델의 Dirty 대기열에 남아 있지 않도록 정리한다.
 *
 * 댕글링 포인터 방지를 위해 RemoveAggregatorDirty(this)로 자신을 제거하고,
 * 이미 제거되어 0개가 반환되는 것을 정상으로 보고 ensure로 검증한다.
 */
FTacticalAggregator::~FTacticalAggregator()
{
    if (mOwner != nullptr)
    {
        UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mOwner->GetWorld());
        checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

        int32 NumRemoved = TacticalFrameworkModel->RemoveAggregatorDirty(this);
        ensure(NumRemoved == 0);
    }
}

/**
 * @brief 모디파이어가 적용되기 전의 기본 값(Base)을 반환한다.
 * @return 현재 베이스 값.
 */
float FTacticalAggregator::GetAttributeBaseValue() const
{
    return mBaseValue;
}

/**
 * @brief 베이스 값을 설정하고, 선택적으로 Dirty 이벤트를 전파한다.
 * @param BaseValue 새 베이스 값.
 * @param BroadcastDirtyEvent true면 의존 이펙트/리스너에 변경을 전파한다.
 */
void FTacticalAggregator::SetAttributeBaseValue(float BaseValue, bool BroadcastDirtyEvent)
{
    mBaseValue = BaseValue;
    if (BroadcastDirtyEvent == true)
    {
        BroadcastOnDirty();
    }
}

/**
 * @brief 단일 모디파이어 연산을 베이스 값에 직접 적용하는 정적 헬퍼(즉시 실행형 이펙트용).
 * @param BaseValue 연산을 적용할 입력 값.
 * @param ModifierOp 적용할 연산 종류(ETacticalModOp — 구 EGameplayModOp 치환).
 * @param EvaluatedMagnitude 모디파이어가 평가한 크기(피연산자).
 * @return 연산이 적용된 값.
 *
 * [PR #191] switch의 case 라벨이 EGameplayModOp::* → ETacticalModOp::*로 전면 치환된 지점이다.
 * EvaluateWithBase()가 "지속형" 모디파이어를 연산 종류별로 누적해 한 수식으로 합성하는 것과 달리,
 * 이 함수는 하나의 모디파이어를 베이스에 곧장 반영한다. 그래서 연산 종류별로 가산(+)/곱(*)/나눗셈(/)/
 * 덮어쓰기(=)를 단순 매핑한다.
 */
float FTacticalAggregator::StaticExecModOnBaseValue(float BaseValue, TEnumAsByte<ETacticalModOp::Type> ModifierOp, float EvaluatedMagnitude)
{
    switch (ModifierOp)
    {
    case ETacticalModOp::Override:
    {
        // 덮어쓰기: 기존 값을 무시하고 크기 값으로 치환한다.
        BaseValue = EvaluatedMagnitude;
        break;
    }
    case ETacticalModOp::AddBase:
    case ETacticalModOp::AddFinal:
    {
        // 합산 계열(AddBase=베이스 합산, AddFinal=최종 합산): 즉시 적용 경로에서는 둘 다 단순 가산.
        BaseValue += EvaluatedMagnitude;
        break;
    }
    case ETacticalModOp::MultiplyAdditive:
    case ETacticalModOp::MultiplyCompound:
    {
        // 곱 계열(MultiplyAdditive=배율 가산, MultiplyCompound=거듭제곱 곱): 즉시 적용에선 둘 다 단순 곱.
        BaseValue *= EvaluatedMagnitude;
        break;
    }
    case ETacticalModOp::DivideAdditive:
    {
        // 나눗셈 가산: 0으로 나누기는 무시(값 보존)한다.
        if (FMath::IsNearlyZero(EvaluatedMagnitude) == false)
        {
            BaseValue /= EvaluatedMagnitude;
        }
        break;
    }
    }
    return BaseValue;
}

/**
 * @brief 즉시 실행형 모디파이어를 자신의 베이스 값에 적용하고 Dirty를 전파한다.
 * @param ModifierOp 적용할 연산 종류(ETacticalModOp).
 * @param EvaluatedMagnitude 적용할 크기 값.
 */
void FTacticalAggregator::ExecModOnBaseValue(TEnumAsByte<ETacticalModOp::Type> ModifierOp, float EvaluatedMagnitude)
{
    mBaseValue = StaticExecModOnBaseValue(mBaseValue, ModifierOp, EvaluatedMagnitude);
    BroadcastOnDirty();
}

/**
 * @brief 지속형 모디파이어 하나를 해당 연산 종류 버킷에 추가한다.
 * @param EvaluatedData 모디파이어가 평가한 크기 값.
 * @param ModifierOp 연산 종류(ETacticalModOp). mMods 배열의 인덱스로 직접 사용된다.
 * @param ActiveHandle 이 모디파이어를 만든 활성 이펙트 핸들(제거 시 식별자).
 *
 * mMods는 ETacticalModOp::Max 크기의 배열이며 op 값으로 인덱싱한다. enum 정수값이 구
 * EGameplayModOp와 동일하게 유지되는 이유 중 (2) "op 값으로 배열 인덱싱"이 바로 이 줄이다.
 */
void FTacticalAggregator::AddAggregatorMod(float EvaluatedData, TEnumAsByte<ETacticalModOp::Type> ModifierOp, FActiveTacticalEffectHandle ActiveHandle)
{
    // mMods[op]: op enum 정수값을 그대로 배열 인덱스로 사용 → enum 값 연속성/호환성이 필수.
    TArray<FTacticalAggregatorMod>& ModList = mMods[ModifierOp];

    int32 NewIdx = ModList.AddUninitialized();
    FTacticalAggregatorMod& NewMod = ModList[NewIdx];

    NewMod.mEvaluatedMagnitude = EvaluatedData;
    NewMod.mStackCount = 0;
    NewMod.mActiveHandle = ActiveHandle;

    BroadcastOnDirty();
}

/**
 * @brief 주어진 활성 이펙트 핸들에 속한 모든 모디파이어를 전 연산 버킷에서 제거한다.
 * @param ActiveHandle 제거 대상 이펙트 핸들(유효해야 함).
 *
 * 어떤 op 버킷에 들어갔는지 알 필요 없이 모든 mMods 버킷을 순회하며 핸들이 일치하는 항목을
 * RemoveAllSwap으로 제거한다(순서 보존 불필요 → Swap 제거가 더 빠름).
 */
void FTacticalAggregator::RemoveAggregatorMod(FActiveTacticalEffectHandle ActiveHandle)
{
    checkf(ActiveHandle.IsValid() == true, TEXT("비활성 핸들을 제거하려 시도"));

    // UE_ARRAY_COUNT(mMods) == ETacticalModOp::Max. 모든 연산 버킷을 빠짐없이 훑는다.
    for (int32 ModOpIdx = 0; ModOpIdx < UE_ARRAY_COUNT(mMods); ++ModOpIdx)
    {
        mMods[ModOpIdx].RemoveAllSwap([&ActiveHandle](const FTacticalAggregatorMod& Element) {
                return (Element.mActiveHandle == ActiveHandle);
            }, EAllowShrinking::No);
    }

    BroadcastOnDirty();
}

/**
 * @brief 이펙트 스펙을 다시 평가해 해당 속성에 걸린 모디파이어를 갱신한다(제거 후 재추가).
 * @param ActiveHandle 기존에 등록되어 있던, 제거할 이펙트 핸들.
 * @param Attribute 이 Aggregator가 담당하는 속성. 스펙의 모디파이어 중 이 속성을 대상으로 한 것만 반영.
 * @param Spec 모디파이어 정의와 크기를 담은 이펙트 스펙.
 * @param InHandle 새로 추가되는 모디파이어에 부여할 활성 이펙트 핸들.
 *
 * 먼저 기존 모디파이어를 모두 제거한 뒤, 스펙의 모디파이어를 순회하며 담당 속성과 일치하는 것만
 * 그 연산 종류(ModDef.mModifierOp = ETacticalModOp)와 평가된 크기로 다시 추가한다.
 */
void FTacticalAggregator::UpdateAggregatorMod(FActiveTacticalEffectHandle ActiveHandle, const FTacticalAttribute& Attribute, const FTacticalEffectSpec& Spec, FActiveTacticalEffectHandle InHandle)
{
    RemoveAggregatorMod(ActiveHandle);

    for (int32 ModIdx = 0; ModIdx < Spec.mModifierValues.Num(); ++ModIdx)
    {
        const FTacticalModifierInfo& ModDef = Spec.mEffectClass->mModifiers[ModIdx];
        // 이 Aggregator가 담당하는 속성을 대상으로 한 모디파이어만 반영한다.
        if (ModDef.mAttribute == Attribute)
        {
            // mModifierOp는 ETacticalModOp(구 EGameplayModOp 치환) — 연산 종류를 그대로 버킷에 전달.
            AddAggregatorMod(Spec.GetModifierMagnitude(ModIdx), ModDef.mModifierOp, InHandle);
        }
    }

    BroadcastOnDirty();
}

/**
 * @brief 현재 베이스 값에 모든 모디파이어를 적용한 최종 속성 값을 산출한다.
 * @return 최종 평가 값.
 */
float FTacticalAggregator::Evaluate() const
{
    return EvaluateWithBase(mBaseValue);
}

/**
 * @brief 주어진 베이스 값에 연산 종류별 모디파이어를 합성해 최종 값을 계산한다(핵심 수식).
 * @param BaseValue 모디파이어 적용 전 입력 베이스 값.
 * @return 모든 모디파이어가 합성된 최종 값.
 *
 * [PR #191] 모든 mMods 버킷 인덱스가 ETacticalModOp::*(구 EGameplayModOp 치환)로 바뀐 지점이다.
 *
 * 합성 절차:
 *   1) Override가 하나라도 있으면 즉시 그 값으로 단락한다(덮어쓰기는 다른 모든 연산을 무시).
 *   2) 그 외에는 연산 종류별로 모디파이어를 한 값으로 접는다.
 *      - Additive/Multiplicitive/Division/AddFinal: SumMods로 "가산" 누적.
 *        이때 각 연산의 항등값(Bias)을 GetModifierBiasByModifierOp로 받아 사용한다
 *        (곱셈/나눗셈 계열의 항등값=1, 합산 계열=0). SumMods가 (magnitude - Bias)를 더하므로
 *        모디파이어가 없으면 정확히 항등값(0 또는 1)이 남는다.
 *      - MultiplyCompound: MultiplyMods로 모든 크기를 "곱"으로 누적(복리). 가산 누적과 다르다.
 *   3) 나눗셈 분모가 0에 가까우면 0 나눗셈 방지를 위해 1로 치환한다.
 *
 * 최종 수식: ((Base + Additive) * Multiplicitive / Division * CompoundMultiply) + FinalAdd
 *   - Additive(=AddBase)는 곱/나눗셈 적용 "전"에 더해지고, FinalAdd(=AddFinal)는 모든 배율 적용 "후"에 더해진다.
 *   - Additive/Multiplicitive/Division은 구 GAS 이름 별칭(각각 AddBase/MultiplyAdditive/DivideAdditive와 동일 값).
 */
float FTacticalAggregator::EvaluateWithBase(float BaseValue) const
{
    // 1) Override 단락: 덮어쓰기 모디파이어가 존재하면 첫 값으로 즉시 반환(다른 연산 전부 무시).
    for (const FTacticalAggregatorMod& Mod : mMods[ETacticalModOp::Override])
    {
        return Mod.mEvaluatedMagnitude;
    }

    // 2) 연산 종류별 누적. Bias는 각 연산의 항등값(합산=0, 곱/나눗셈=1)으로, 모디파이어 부재 시 무효원소가 됨.
    float Additive = SumMods(mMods[ETacticalModOp::Additive], TacticalEffectUtilities::GetModifierBiasByModifierOp(ETacticalModOp::Additive));
    float Multiplicitive = SumMods(mMods[ETacticalModOp::Multiplicitive], TacticalEffectUtilities::GetModifierBiasByModifierOp(ETacticalModOp::Multiplicitive));
    float Division = SumMods(mMods[ETacticalModOp::Division], TacticalEffectUtilities::GetModifierBiasByModifierOp(ETacticalModOp::Division));
    float FinalAdd = SumMods(mMods[ETacticalModOp::AddFinal], TacticalEffectUtilities::GetModifierBiasByModifierOp(ETacticalModOp::AddFinal));
    // MultiplyCompound는 가산이 아니라 곱(복리)으로 누적 → 별도 MultiplyMods 사용.
    float CompoundMultiply = MultiplyMods(mMods[ETacticalModOp::MultiplyCompound]);

    // 3) 0 나눗셈 방지: 분모가 0에 가까우면 1로 치환(값 보존).
    if (FMath::IsNearlyZero(Division) == true)
    {
        UE_LOG(LogAttributeSetComp, Log, TEXT("속성 변경 시, 변경 값을 0으로 나눌 수 없음. 1로 치환"));
        Division = 1.f;
    }
    // 최종 합성 수식: 베이스 합산 → 배율/나눗셈/복리곱 → 최종 합산 순.
    return ((BaseValue + Additive) * Multiplicitive / Division * CompoundMultiply) + FinalAdd;
}

/**
 * @brief 한 연산 버킷의 모디파이어들을 항등값(Bias) 기준으로 가산 누적한다.
 * @param Mods 누적할 모디파이어 배열(하나의 연산 종류 버킷).
 * @param Bias 해당 연산의 항등값(합산=0, 곱/나눗셈=1). GetModifierBiasByModifierOp가 제공.
 * @return 누적 결과. 모디파이어가 없으면 Bias 그대로 반환된다.
 *
 * Sum을 Bias로 시작하고 각 모디파이어마다 (magnitude - Bias)를 더한다. 곱셈/나눗셈 계열(Bias=1)에서
 * 예컨대 1.2배, 1.5배 모디파이어는 각각 +0.2, +0.5로 합산되어 "배율 가산"(총 1.7배) 의미가 된다.
 * 합산 계열(Bias=0)에서는 그냥 크기를 그대로 더한다.
 */
float FTacticalAggregator::SumMods(const TArray<FTacticalAggregatorMod>& Mods, float Bias)
{
    float Sum = Bias;
    for (const FTacticalAggregatorMod& Mod : Mods)
    {
        // (magnitude - Bias)를 더해 항등값 중복을 제거 → 모디파이어 부재 시 정확히 Bias가 남는다.
        Sum += (Mod.mEvaluatedMagnitude - Bias);
    }
    return Sum;
}

/**
 * @brief MultiplyCompound 버킷의 모디파이어들을 곱(복리)으로 누적한다.
 * @param Mods 누적할 모디파이어 배열(MultiplyCompound 버킷).
 * @return 모든 크기를 곱한 결과. 모디파이어가 없으면 항등값 1.0을 반환한다.
 *
 * SumMods의 "배율 가산"과 달리, 여기서는 각 배율을 서로 곱한다. 예: 1.2배와 1.5배가 함께 걸리면
 * 1.2 * 1.5 = 1.8배(복리). MultiplyCompound가 별도 연산 종류로 존재하는 이유다.
 */
float FTacticalAggregator::MultiplyMods(const TArray<FTacticalAggregatorMod>& Mods)
{
    float Result = 1.0f;
    for (const FTacticalAggregatorMod& Mod : Mods)
    {
        Result *= Mod.mEvaluatedMagnitude;
    }
    return Result;
}

/**
 * @brief 이 Aggregator 값이 변경되었음을 리스너와 의존 이펙트에 전파한다.
 *
 * 동작 분기:
 *   - 배치가 열려 있고(GetGlobalBatchCount > 0) 알릴 대상이 있으면, 즉시 처리하지 않고 Dirty 대기열에
 *     등록만 하고 반환한다(배치 종료 시 일괄 처리). FScopedTacticalAggregatorOnDirtyBatch 참조.
 *   - 그렇지 않으면 즉시 OnDirty를 브로드캐스트하고, 자신에 의존하는 이펙트들에 크기 재계산을 요청한다.
 *
 * 재귀 방어: 리스너가 다시 이 속성을 변경하는 순환을 막기 위해 mDirtyCount로 깊이를 제한한다(MAX_DIRTY).
 */
void FTacticalAggregator::BroadcastOnDirty()
{
    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mOwner->GetWorld());
    checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

    // 배치가 열려 있고 알릴 대상이 있으면 즉시 전파를 보류하고 대기열에만 등록한다.
    if (TacticalFrameworkModel->GetGlobalBatchCount() > 0 && (mDependentEffects.Num() > 0 || OnDirty.IsBound()))
    {
        TacticalFrameworkModel->AddAggregatorDirty(this);
        return;
    }

    /* 재귀 검사 */

    // 리스너 콜백이 다시 이 속성을 변경하는 무한 순환을 깊이 제한으로 차단한다.
    const int32 MAX_DIRTY = 10;
    if (mDirtyCount > MAX_DIRTY)
    {
        checkf(false, TEXT("재귀적으로 해당 속성을 계속 변경해주고 있음"));
        return;
    }

    mDirtyCount++;
    OnDirty.Broadcast(this);

    /* 값 갱신 요청 */

    // 순회 중 mDependentEffects가 변형될 수 있으므로 복사본을 떠서 돌고, 살아있는 항목만 재등록한다.
    TArray<FActiveTacticalEffectHandle> DependentEffectsCopy = mDependentEffects;
    mDependentEffects.Empty();

    for (FActiveTacticalEffectHandle& Handle : DependentEffectsCopy)
    {
        UAttributeSetComponentModel* ASC = Handle.GetOwningAttributeSetComponentModel();
        if (ASC != nullptr)
        {
            /* ASC가 아직 살아있다면 재 등록 */

            // 의존 이펙트에 크기 재계산을 알리고, 유효한 핸들만 다시 의존 목록에 보존한다(소멸된 ASC는 누락 → 정리).
            ASC->OnMagnitudeDependencyChange(Handle, this);
            mDependentEffects.Add(Handle);
        }
    }

    mDirtyCount--;
}

