#include "TAS/Effect/ActiveTacticalEffectsContainer.h"
#include "TAS/Aggregator/TacticalAggregator.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"

FScopedActiveTacticalEffectLock::FScopedActiveTacticalEffectLock(FActiveTacticalEffectsContainer& Container) :
    mContainer(Container)
{
    mContainer.IncrementLock();
}

FScopedActiveTacticalEffectLock::~FScopedActiveTacticalEffectLock()
{
    mContainer.DecrementLock();
}

void FActiveTacticalEffectsContainer::IncrementLock()
{
    mScopedLockCount++;
}

void FActiveTacticalEffectsContainer::DecrementLock()
{
    if (--mScopedLockCount == 0)
    {
        /* 추가 작업 */

        FActiveTacticalEffect* CurPendingEffect = mPendingGameplayEffectHead;
        FActiveTacticalEffect* EndPendingEffect = *mPendingGameplayEffectTail;

        while (CurPendingEffect != EndPendingEffect)
        {
            if (CurPendingEffect->mIsPendingRemove == false)
            {
                /* 추가가 미루어진 이펙트 추가 */

                mAttributeEffects.Add(MoveTemp(*CurPendingEffect));
            }
            else
            {
                /* 추가가 미루어진 이펙트는 이미 제거됨 */

                mPendingRemoveCount--;
            }
            CurPendingEffect = CurPendingEffect->mPendingNext;
        }
        mPendingGameplayEffectTail = &mPendingGameplayEffectHead;

        /* 제거 작업 */

        for (int32 index = mAttributeEffects.Num() - 1; index >= 0 && mPendingRemoveCount > 0; --index)
        {
            FActiveTacticalEffect& Effect = mAttributeEffects[index];

            if (Effect.mIsPendingRemove)
            {
                // 해당 이펙트 핸들이 사라짐을 알리기
                Effect.mHandle.RemoveFromGlobalMap();

                mAttributeEffects.RemoveAtSwap(index, EAllowShrinking::No);

                mPendingRemoveCount--;
            }
        }

        checkf(mPendingRemoveCount == 0, TEXT("예약된 이펙트 증감 로직 에러"));
    }
}

void FActiveTacticalEffectsContainer::CleanupAttributeAggregator(const FGameplayAttribute& Attribute)
{
    TSharedPtr<FTacticalAggregator>* AggregatorPtr = mAttributeAggregatorMap.Find(Attribute);
    if (AggregatorPtr != nullptr)
    {
        (*AggregatorPtr)->OnDirty.RemoveAll(mOwner.Get());

        mAttributeAggregatorMap.Remove(Attribute);
    }
}

void FActiveTacticalEffectsContainer::OnAttributeAggregatorDirty(FTacticalAggregator* Aggregator, FGameplayAttribute Attribute)
{
    checkf(mAttributeAggregatorMap.FindChecked(Attribute).Get() == Aggregator, TEXT("속성에 대한 계산 객체가 동일하지 않음"));

    const float NewCurrentValue = Aggregator->Evaluate();
    const float OldCurrentValue = mOwner->GetAttributeCurrentValue(Attribute);
    UE_LOG(LogAttributeSetComp, Log, TEXT("[%s] 현재 값 변경 %.2f -> %.2f"), *Attribute.GetName(), OldCurrentValue, NewCurrentValue);

    UpdateAttributeCurrentValue(Attribute, NewCurrentValue);
}

void FActiveTacticalEffectsContainer::OnMagnitudeDependencyChange(FActiveTacticalEffectHandle Handle, const FTacticalAggregator* ChangedAgg)
{
    if (Handle.IsValid())
    {
        TACTICAL_EFFECT_SCOPE_LOCK();
        FActiveTacticalEffect* ActiveEffect = GetActiveTacticalEffect(Handle);
        if (ActiveEffect != nullptr)
        {
            //// This handle registered with the ChangedAgg to be notified when the aggregator changed.
            //// At this point we don't know what actually needs to be updated inside this active gameplay effect.
            //FGameplayEffectSpec& Spec = ActiveEffect->mSpec;

            //// We must update attribute aggregators only if we are actually 'on' right now, and if we are non periodic (periodic effects do their thing on execute callbacks)
            //const bool MustUpdateAttributeAggregators = (ActiveEffect->bIsInhibited == false && (Spec.GetPeriod() <= UGameplayEffect::NO_PERIOD));

            //// As we update our modifier magnitudes, we will update our owner's attribute aggregators. When we do this, we have to clear them first of all of our (Handle's) previous mods.
            //// Since we could potentially have two mods to the same attribute, one that gets updated, and one that doesnt - we need to do this in two passes.
            //TSet<FGameplayAttribute> AttributesToUpdate;

            //bool bMarkedDirty = false;

            //// First pass: update magnitudes of our modifiers that changed
            //for (int32 ModIdx = 0; ModIdx < Spec.Modifiers.Num(); ++ModIdx)
            //{
            //    const FGameplayModifierInfo& ModDef = Spec.Def->Modifiers[ModIdx];
            //    FModifierSpec& ModSpec = Spec.Modifiers[ModIdx];

            //    float RecalculatedMagnitude = 0.f;
            //    if (ModDef.ModifierMagnitude.AttemptRecalculateMagnitudeFromDependentAggregatorChange(Spec, RecalculatedMagnitude, ChangedAgg))
            //    {
            //        // If this is the first pending magnitude change, need to mark the container item dirty as well as
            //        // wake the owner actor from dormancy so replication works properly
            //        if (!bMarkedDirty)
            //        {
            //            bMarkedDirty = true;
            //            AActor* OwnerActor = Owner ? Owner->GetOwnerActor() : nullptr;
            //            if (IsNetAuthority() && OwnerActor)
            //            {
            //                OwnerActor->FlushNetDormancy();
            //            }
            //            MarkItemDirty(*ActiveEffect);
            //        }

            //        ModSpec.EvaluatedMagnitude = RecalculatedMagnitude;

            //        // We changed, so we need to reapply/update our spot in the attribute aggregator map
            //        if (MustUpdateAttributeAggregators)
            //        {
            //            AttributesToUpdate.Add(ModDef.Attribute);
            //        }
            //    }
            //}

            // Second pass, update the aggregators that we need to
            // UpdateAggregatorModMagnitudes(AttributesToUpdate, *ActiveEffect);
        }
    }
}

void FActiveTacticalEffectsContainer::SetAttributeBaseValue(FGameplayAttribute Attribute, float BaseValue)
{
    checkf(mOwner != nullptr, TEXT("변경 ASC 대상이 존재하지 않음"));
    const UAttributeSet* Set = mOwner->GetAttributeSet_Internal(Attribute.GetAttributeSetClass());
    checkf(Set != nullptr, TEXT("변경 AttributeSet 대상이 존재하지 않음"));

    float OldBaseValue = 0.0f;
    Set->PreAttributeBaseChange(Attribute, BaseValue);

    /* 속성에서 베이스 값 변경 */

    const FStructProperty* StructProperty = CastField<FStructProperty>(Attribute.GetUProperty());
    checkf(StructProperty != nullptr, TEXT("변경 속성 대상이 존재하지 않음"));
    FGameplayAttributeData* DataPtr = StructProperty->ContainerPtrToValuePtr<FGameplayAttributeData>(const_cast<UAttributeSet*>(Set));
    checkf(DataPtr != nullptr, TEXT("변경 FGameplayAttributeData 대상이 존재하지 않음"));
    OldBaseValue = DataPtr->GetBaseValue();
    DataPtr->SetBaseValue(BaseValue);

    /* 새로운 베이스 값으로 현재 값 계산 */

    TSharedPtr<FTacticalAggregator>* RefPtr = mAttributeAggregatorMap.Find(Attribute);
    if (RefPtr != nullptr)
    {
        FTacticalAggregator* Aggregator = RefPtr->Get();
        checkf(Aggregator != nullptr, TEXT("변경 FTacticalAggregator 대상이 존재하지 않음"));

        OldBaseValue = Aggregator->GetAttributeBaseValue();
        Aggregator->SetAttributeBaseValue(BaseValue);
    }
    else
    {
        // 추가 값이 없다면, 베이스 값이 곧 현재 값
        UpdateAttributeCurrentValue(Attribute, BaseValue);
    }

    Set->PostAttributeBaseChange(Attribute, OldBaseValue, GetAttributeBaseValue(Attribute));
}

float FActiveTacticalEffectsContainer::GetAttributeBaseValue(FGameplayAttribute Attribute) const
{
    float BaseValue = 0.f;
    if (mOwner != nullptr)
    {
        const UAttributeSet* AttributeSet = mOwner->GetAttributeSet_Internal(Attribute.GetAttributeSetClass());
        checkf(AttributeSet != nullptr, TEXT("탐색 AttributeSet 대상이 존재하지 않음"));

        const TSharedPtr<FTacticalAggregator>* RefPtr = mAttributeAggregatorMap.Find(Attribute);

        if (FGameplayAttribute::IsGameplayAttributeDataProperty(Attribute.GetUProperty()))
        {
            const FStructProperty* StructProperty = CastField<FStructProperty>(Attribute.GetUProperty());
            checkf(StructProperty != nullptr, TEXT("탐색 속성 대상이 존재하지 않음"));
            const FGameplayAttributeData* DataPtr = StructProperty->ContainerPtrToValuePtr<FGameplayAttributeData>(AttributeSet);
            if (DataPtr != nullptr)
            {
                BaseValue = DataPtr->GetBaseValue();
            }
        }
        else if (RefPtr != nullptr)
        {
            BaseValue = RefPtr->Get()->GetAttributeBaseValue();
        }
    }
    else
    {
        UE_LOG(LogAttributeSetComp, Warning, TEXT("해당 값을 찾기 위한 Owner가 존재하지 않음"));
    }
    return BaseValue;
}

void FActiveTacticalEffectsContainer::UpdateAttributeCurrentValue(FGameplayAttribute Attribute, float CurrentValue)
{
    const float OldValue = mOwner->GetAttributeCurrentValue(Attribute);
    mOwner->SetAttributeCurrentValue_Internal(Attribute, CurrentValue);
}

FActiveTacticalEffect* FActiveTacticalEffectsContainer::GetActiveTacticalEffect(const FActiveTacticalEffectHandle Handle)
{
    for (FActiveTacticalEffect& Effect : this)
    {
        if (Effect.mHandle == Handle)
        {
            return &Effect;
        }
    }
    return nullptr;
}

const FActiveTacticalEffect* FActiveTacticalEffectsContainer::GetActiveTacticalEffect(const FActiveTacticalEffectHandle Handle) const
{
    for (const FActiveTacticalEffect& Effect : this)
    {
        if (Effect.mHandle == Handle)
        {
            return &Effect;
        }
    }
    return nullptr;
}

