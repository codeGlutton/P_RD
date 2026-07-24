#include "TAS/Aggregator/TacticalAggregator.h"

#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

FScopedTacticalAggregatorOnDirtyBatch::FScopedTacticalAggregatorOnDirtyBatch(UWorld* World) :
    mWorld(World)
{
    check(mWorld);

    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mWorld);
    checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

    TacticalFrameworkModel->BeginAggregatorDirtyBatch();
}

FScopedTacticalAggregatorOnDirtyBatch::~FScopedTacticalAggregatorOnDirtyBatch()
{
    check(mWorld);

    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mWorld);
    checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

    TacticalFrameworkModel->EndAggregatorDirtyBatch();
}

FTacticalAggregator::FTacticalAggregator(UAttributeSetComponentModel* Owner, float InBaseValue) :
    mOwner(Owner),
    mDirtyCount(0),
    mBaseValue(InBaseValue)
{
}

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

float FTacticalAggregator::GetAttributeBaseValue() const
{
    return mBaseValue;
}

void FTacticalAggregator::SetAttributeBaseValue(float BaseValue, bool BroadcastDirtyEvent)
{
    mBaseValue = BaseValue;
    if (BroadcastDirtyEvent == true)
    {
        BroadcastOnDirty();
    }
}

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

void FTacticalAggregator::ExecModOnBaseValue(TEnumAsByte<ETacticalModOp::Type> ModifierOp, float EvaluatedMagnitude)
{
    mBaseValue = StaticExecModOnBaseValue(mBaseValue, ModifierOp, EvaluatedMagnitude);
    BroadcastOnDirty();
}

void FTacticalAggregator::AddAggregatorMod(float EvaluatedData, TEnumAsByte<ETacticalModOp::Type> ModifierOp, FActiveTacticalEffectHandle ActiveHandle)
{
    TArray<FTacticalAggregatorMod>& ModList = mMods[ModifierOp];

    int32 NewIdx = ModList.AddUninitialized();
    FTacticalAggregatorMod& NewMod = ModList[NewIdx];

    NewMod.mEvaluatedMagnitude = EvaluatedData;
    NewMod.mStackCount = 0;
    NewMod.mActiveHandle = ActiveHandle;

    BroadcastOnDirty();
}

void FTacticalAggregator::RemoveAggregatorMod(FActiveTacticalEffectHandle ActiveHandle)
{
    checkf(ActiveHandle.IsValid() == true, TEXT("비활성 핸들을 제거하려 시도"));

    for (int32 ModOpIdx = 0; ModOpIdx < UE_ARRAY_COUNT(mMods); ++ModOpIdx)
    {
        mMods[ModOpIdx].RemoveAllSwap([&ActiveHandle](const FTacticalAggregatorMod& Element) {
                return (Element.mActiveHandle == ActiveHandle);
            }, EAllowShrinking::No);
    }

    BroadcastOnDirty();
}

void FTacticalAggregator::UpdateAggregatorMod(FActiveTacticalEffectHandle ActiveHandle, const FTacticalAttribute& Attribute, const FTacticalEffectSpec& Spec, FActiveTacticalEffectHandle InHandle)
{
    RemoveAggregatorMod(ActiveHandle);

    for (int32 ModIdx = 0; ModIdx < Spec.mModifiers.Num(); ++ModIdx)
    {
        const FTacticalModifierInfo& ModDef = Spec.mEffectClass->mModifiers[ModIdx];
        if (ModDef.mAttribute == Attribute)
        {
            AddAggregatorMod(Spec.GetStackedModifierMagnitude(ModIdx), ModDef.mModifierOp, InHandle);
        }
    }

    BroadcastOnDirty();
}

float FTacticalAggregator::Evaluate() const
{
    return EvaluateWithBase(mBaseValue);
}

float FTacticalAggregator::EvaluateWithBase(float BaseValue) const
{
    // 덮어쓰기 모디파이어가 존재하면 첫 값으로 즉시 반환
    for (const FTacticalAggregatorMod& Mod : mMods[ETacticalModOp::Override])
    {
        return Mod.mEvaluatedMagnitude;
    }

    /* 연산 종류별 누적값 계산 */

    float Additive = SumMods(mMods[ETacticalModOp::AddBase], TacticalEffectUtilities::GetModifierBiasByModifierOp(ETacticalModOp::AddBase));
    float Multiplicative = SumMods(mMods[ETacticalModOp::MultiplyAdditive], TacticalEffectUtilities::GetModifierBiasByModifierOp(ETacticalModOp::MultiplyAdditive));
    float Division = SumMods(mMods[ETacticalModOp::DivideAdditive], TacticalEffectUtilities::GetModifierBiasByModifierOp(ETacticalModOp::DivideAdditive));
    float FinalAdd = SumMods(mMods[ETacticalModOp::AddFinal], TacticalEffectUtilities::GetModifierBiasByModifierOp(ETacticalModOp::AddFinal));
    float CompoundMultiply = MultiplyMods(mMods[ETacticalModOp::MultiplyCompound]);

    // 0 나눗셈 방지. 분모가 0에 가까우면 1로 치환
    if (FMath::IsNearlyZero(Division) == true)
    {
        UE_LOG(LogAttributeSetComp, Log, TEXT("속성 변경 시, 변경 값을 0으로 나눌 수 없음. 1로 치환"));
        Division = 1.f;
    }

    // 최종 연산자에 따른 수식 계산
    return ((BaseValue + Additive) * Multiplicative / Division * CompoundMultiply) + FinalAdd;
}

float FTacticalAggregator::SumMods(const TArray<FTacticalAggregatorMod>& Mods, float Bias)
{
    float Sum = Bias;
    for (const FTacticalAggregatorMod& Mod : Mods)
    {
        Sum += (Mod.mEvaluatedMagnitude - Bias);
    }
    return Sum;
}

float FTacticalAggregator::MultiplyMods(const TArray<FTacticalAggregatorMod>& Mods)
{
    float Result = 1.0f;
    for (const FTacticalAggregatorMod& Mod : Mods)
    {
        Result *= Mod.mEvaluatedMagnitude;
    }
    return Result;
}

void FTacticalAggregator::BroadcastOnDirty()
{
    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mOwner->GetWorld());
    checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

    // 배치가 열려 있고 알릴 대상이 있으면 전파를 보류하고 대기열에만 등록
    if (TacticalFrameworkModel->GetGlobalBatchCount() > 0 && (mDependentEffects.Num() > 0 || OnDirty.IsBound()))
    {
        TacticalFrameworkModel->AddAggregatorDirty(this);
        return;
    }

    /* 재귀 검사 */

    const int32 MAX_DIRTY = 10;
    if (mDirtyCount > MAX_DIRTY)
    {
        checkf(false, TEXT("재귀적으로 해당 속성을 계속 변경해주고 있음"));
        return;
    }

    mDirtyCount++;
    OnDirty.Broadcast(this);

    /* 값 갱신 요청 */

    TArray<FActiveTacticalEffectHandle> DependentEffectsCopy = mDependentEffects;
    mDependentEffects.Empty();

    for (FActiveTacticalEffectHandle& Handle : DependentEffectsCopy)
    {
        UAttributeSetComponentModel* ASC = Handle.GetOwningAttributeSetComponentModel();
        if (ASC != nullptr)
        {
            /* ASC가 아직 살아있다면 재 등록 */

            ASC->OnMagnitudeDependencyChange(Handle, this);
            mDependentEffects.Add(Handle);
        }
    }

    mDirtyCount--;
}

