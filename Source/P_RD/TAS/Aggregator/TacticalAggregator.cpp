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

float FTacticalAggregator::StaticExecModOnBaseValue(float BaseValue, TEnumAsByte<EGameplayModOp::Type> ModifierOp, float EvaluatedMagnitude)
{
    switch (ModifierOp)
    {
    case EGameplayModOp::Override:
    {
        BaseValue = EvaluatedMagnitude;
        break;
    }
    case EGameplayModOp::AddBase:
    case EGameplayModOp::AddFinal:
    {
        BaseValue += EvaluatedMagnitude;
        break;
    }
    case EGameplayModOp::MultiplyAdditive:
    case EGameplayModOp::MultiplyCompound:
    {
        BaseValue *= EvaluatedMagnitude;
        break;
    }
    case EGameplayModOp::DivideAdditive:
    {
        if (FMath::IsNearlyZero(EvaluatedMagnitude) == false)
        {
            BaseValue /= EvaluatedMagnitude;
        }
        break;
    }
    }
    return BaseValue;
}

void FTacticalAggregator::ExecModOnBaseValue(TEnumAsByte<EGameplayModOp::Type> ModifierOp, float EvaluatedMagnitude)
{
    mBaseValue = StaticExecModOnBaseValue(mBaseValue, ModifierOp, EvaluatedMagnitude);
    BroadcastOnDirty();
}

void FTacticalAggregator::AddAggregatorMod(float EvaluatedData, TEnumAsByte<EGameplayModOp::Type> ModifierOp, FActiveTacticalEffectHandle ActiveHandle)
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

    for (int32 ModIdx = 0; ModIdx < Spec.mModifierValues.Num(); ++ModIdx)
    {
        const FTacticalModifierInfo& ModDef = Spec.mEffectClass->mModifiers[ModIdx];
        if (ModDef.mAttribute == Attribute)
        {
            AddAggregatorMod(Spec.GetModifierMagnitude(ModIdx), ModDef.mModifierOp, InHandle);
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
    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mOwner->GetWorld());
    checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

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

