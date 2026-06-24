#include "TAS/Aggregator/TacticalAggregator.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"

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

float FTacticalAggregator::Evaluate() const
{
    return EvaluateWithBase(mBaseValue);
}

float FTacticalAggregator::EvaluateWithBase(float BaseValue) const
{
    for (const FTacticalAggregatorMod& Mod : mMods[EGameplayModOp::Override])
    {
        return Mod.mEvaluatedMagnitude;
    }

    float Additive = SumMods(mMods[EGameplayModOp::Additive], GameplayEffectUtilities::GetModifierBiasByModifierOp(EGameplayModOp::Additive));
    float Multiplicitive = SumMods(mMods[EGameplayModOp::Multiplicitive], GameplayEffectUtilities::GetModifierBiasByModifierOp(EGameplayModOp::Multiplicitive));
    float Division = SumMods(mMods[EGameplayModOp::Division], GameplayEffectUtilities::GetModifierBiasByModifierOp(EGameplayModOp::Division));
    float FinalAdd = SumMods(mMods[EGameplayModOp::AddFinal], GameplayEffectUtilities::GetModifierBiasByModifierOp(EGameplayModOp::AddFinal));
    float CompoundMultiply = MultiplyMods(mMods[EGameplayModOp::MultiplyCompound]);

    if (FMath::IsNearlyZero(Division) == true)
    {
        UE_LOG(LogAttributeSetComp, Log, TEXT("속성 변경 시, 변경 값을 0으로 나눌 수 없음. 1로 치환"));
        Division = 1.f;
    }
    return ((BaseValue + Additive) * Multiplicitive / Division * CompoundMultiply) + FinalAdd;
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

