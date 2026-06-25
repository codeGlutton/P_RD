#include "TAS/Effect/ActiveTacticalEffectsContainer.h"
#include "TAS/Aggregator/TacticalAggregator.h"

#include "TAS/Effect/TacticalEffectContext.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "GameplayEffectExtension.h"

FScopedActiveTacticalEffectLock::FScopedActiveTacticalEffectLock(FActiveTacticalEffectsContainer& Container) :
    mContainer(Container)
{
    mContainer.IncrementLock();
}

FScopedActiveTacticalEffectLock::~FScopedActiveTacticalEffectLock()
{
    mContainer.DecrementLock();
}

FActiveTacticalEffectsContainer::FActiveTacticalEffectsContainer() :
    mOwner(nullptr),
    mScopedLockCount(0),
    mPendingRemoveCount(0),
    mPendingTacticalEffectHead(nullptr)
{
    mPendingTacticalEffectTail = &mPendingTacticalEffectHead;
}

FActiveTacticalEffectsContainer::~FActiveTacticalEffectsContainer()
{
    while (mPendingTacticalEffectHead != nullptr)
    {
        FActiveTacticalEffect* Next = mPendingTacticalEffectHead->mPendingNext;
        delete mPendingTacticalEffectHead;
        mPendingTacticalEffectHead = Next;
    }
}

void FActiveTacticalEffectsContainer::RegisterWithOwnerModel(UAttributeSetComponentModel* Owner)
{
    if (mOwner != Owner && Owner != nullptr)
    {
        mOwner = Owner;
    }
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

        FActiveTacticalEffect* CurPendingEffect = mPendingTacticalEffectHead;
        FActiveTacticalEffect* EndPendingEffect = *mPendingTacticalEffectTail;

        while (CurPendingEffect != EndPendingEffect)
        {
            if (CurPendingEffect->mIsPendingRemove == false)
            {
                /* 추가가 미루어진 이펙트 추가 */

                mTacticalEffects.Add(MoveTemp(*CurPendingEffect));
            }
            else
            {
                /* 추가가 미루어진 이펙트는 이미 제거됨 */

                mPendingRemoveCount--;
            }
            CurPendingEffect = CurPendingEffect->mPendingNext;
        }
        mPendingTacticalEffectTail = &mPendingTacticalEffectHead;

        /* 제거 작업 */

        for (int32 index = mTacticalEffects.Num() - 1; index >= 0 && mPendingRemoveCount > 0; --index)
        {
            FActiveTacticalEffect& Effect = mTacticalEffects[index];

            if (Effect.mIsPendingRemove)
            {
                // 해당 이펙트 핸들이 사라짐을 알리기
                Effect.mHandle.RemoveFromGlobalMap();

                mTacticalEffects.RemoveAtSwap(index, EAllowShrinking::No);

                mPendingRemoveCount--;
            }
        }

        checkf(mPendingRemoveCount == 0, TEXT("예약된 이펙트 증감 로직 에러"));
    }
}

TSharedPtr<FTacticalAggregator>& FActiveTacticalEffectsContainer::FindOrCreateAttributeAggregator(const FGameplayAttribute& Attribute)
{
    TSharedPtr<FTacticalAggregator>* RefPtr = mAttributeAggregatorMap.Find(Attribute);
    if (RefPtr != nullptr)
    {
        return *RefPtr;
    }

    float CurrentBaseValueOfProperty = mOwner->GetAttributeBaseValue(Attribute);
    TSharedPtr<FTacticalAggregator> NewAttributeAggregator = MakeShared<FTacticalAggregator>(CurrentBaseValueOfProperty);
    NewAttributeAggregator->OnDirty.AddUObject(mOwner.Get(), &UAttributeSetComponentModel::OnAttributeAggregatorDirty, Attribute);

    return mAttributeAggregatorMap.Add(Attribute, MoveTemp(NewAttributeAggregator));
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
        // NOTE : 
        // 현재 해당 속성을 통해 실시간으로 Effect 최종 수치를 결정하는 요소는 딱히 필요가 없다.
        // 따라서 구현 생략
    }
}

void FActiveTacticalEffectsContainer::OnStackCountChange(FActiveTacticalEffect& ActiveEffect, int32 OldStackCount, int32 NewStackCount)
{
    if (OldStackCount != NewStackCount)
    {
        UpdateAllAggregatorModMagnitudes(ActiveEffect);
    }

    if (ActiveEffect.mSpec.mEffectClass != nullptr)
    {
        mOwner->NotifyTagMap_StackCountChange(ActiveEffect.mSpec.mEffectClass->GetGrantedTags());
    }

    ActiveEffect.mEventSet.OnStackChanged.Broadcast(ActiveEffect.mHandle, ActiveEffect.mSpec.GetStackCount(), OldStackCount);
}

void FActiveTacticalEffectsContainer::ApplyModToAttribute(const FGameplayAttribute& Attribute, TEnumAsByte<EGameplayModOp::Type> ModifierOp, float ModifierMagnitude)
{
    float CurrentBase = GetAttributeBaseValue(Attribute);
    float NewBase = FTacticalAggregator::StaticExecModOnBaseValue(CurrentBase, ModifierOp, ModifierMagnitude);

    SetAttributeBaseValue(Attribute, NewBase);
}

FActiveTacticalEffect* FActiveTacticalEffectsContainer::ApplyTacticalEffectSpec(const FTacticalEffectSpec& Spec, bool& FoundExistingStackableGE)
{
	TACTICAL_EFFECT_SCOPE_LOCK();

	checkf(Spec.mEffectClass != nullptr, TEXT("적용하려는 Effect 클래스가 없음"));
	FoundExistingStackableGE = false;

	FActiveTacticalEffect* AppliedActiveEffect = nullptr;
	FActiveTacticalEffect* ExistingStackableEffect = FindStackableActiveTacticalEffect(Spec);

	int32 PreStackCount = 0;
	int32 NewStackCount = 0;

	if (ExistingStackableEffect != nullptr)
	{
        /* 스태킹 Effect의 경우 */

        FoundExistingStackableGE = true;

		FTacticalEffectSpec& ExistingSpec = ExistingStackableEffect->mSpec;
		PreStackCount = ExistingSpec.GetStackCount();
        NewStackCount = ExistingSpec.GetStackCount() + Spec.GetStackCount();

        checkf(ExistingSpec.mDynamicMagnitude == Spec.mDynamicMagnitude, TEXT("스태킹되는 이펙트는 수치가 다를 수 없음"));

		ExistingStackableEffect->mSpec = Spec;
		ExistingStackableEffect->mSpec.SetStackCount(NewStackCount);

		AppliedActiveEffect = ExistingStackableEffect;
	}
	else
	{
        /* 일반 Effect의 경우 */

        FActiveTacticalEffectHandle NewHandle = FActiveTacticalEffectHandle::GenerateNewHandle(mOwner->GetWorld(), mOwner.Get());
		if (mTacticalEffects.GetSlack() <= 0)
		{
			/* 배열 크기 변동으로 재할당이 필요한 경우 */

			check(mPendingTacticalEffectTail != nullptr);

			if (*mPendingTacticalEffectTail == nullptr)
			{
				/* 비어있는 경우 */

				AppliedActiveEffect = new FActiveTacticalEffect(NewHandle, Spec);
				*mPendingTacticalEffectTail = AppliedActiveEffect;
			}
			else
			{
				/* 할당 공간이 남아있는 경우 */

				**mPendingTacticalEffectTail = FActiveTacticalEffect(NewHandle, Spec);
				AppliedActiveEffect = *mPendingTacticalEffectTail;
			}
			mPendingTacticalEffectTail = &AppliedActiveEffect->mPendingNext;
		}
		else
		{
			/* 배열 크기 재할당이 필요없는 경우 */

			AppliedActiveEffect = new(mTacticalEffects) FActiveTacticalEffect(NewHandle, Spec);
		}
	}

	FTacticalEffectSpec& AppliedEffectSpec = AppliedActiveEffect->mSpec;
	AppliedEffectSpec.CalculateModifierMagnitudes();

	if (ExistingStackableEffect != nullptr)
	{
		OnStackCountChange(*ExistingStackableEffect, PreStackCount, NewStackCount);
	}
    else
    {
        InternalOnActiveTacticalEffectAdded(*AppliedActiveEffect);
    }

	return AppliedActiveEffect;
}

void FActiveTacticalEffectsContainer::ExecuteActiveEffectsFrom(FTacticalEffectSpec& Spec)
{
    if (mOwner.IsValid() == false)
    {
        return;
    }

    FTacticalEffectSpec& SpecToUse = Spec;
    SpecToUse.CalculateModifierMagnitudes();

    /* 모디파이어 실행 */

    bool ModifierSuccessfullyExecuted = false;

    check(SpecToUse.mModifierValues.Num() == SpecToUse.mEffectClass->mModifiers.Num());
    for (int32 ModIdx = 0; ModIdx < SpecToUse.mModifierValues.Num(); ++ModIdx)
    {
        const FTacticalModifierInfo& ModDef = SpecToUse.mEffectClass->mModifiers[ModIdx];

        FGameplayModifierEvaluatedData EvalData(ModDef.mAttribute, ModDef.mModifierOp, SpecToUse.GetModifierMagnitude(ModIdx));
        ModifierSuccessfullyExecuted |= InternalExecuteMod(SpecToUse, EvalData);
    }

    Spec.mEffectClass->OnExecuted(*this, Spec);
}

bool FActiveTacticalEffectsContainer::RemoveActiveTacticalEffect(FActiveTacticalEffectHandle Handle, int32 StacksToRemove)
{
    int32 NumTacticalEffects = GetNumTacticalEffects();
    for (int32 ActiveGEIdx = 0; ActiveGEIdx < NumTacticalEffects; ++ActiveGEIdx)
    {
        FActiveTacticalEffect& Effect = *GetActiveTacticalEffect(ActiveGEIdx);
        if (Effect.mHandle == Handle && Effect.mIsPendingRemove == false)
        {
            InternalRemoveActiveTacticalEffect(ActiveGEIdx, StacksToRemove, true);
            return true;
        }
    }

    return false;
}

FOnChangeAttributeValue& FActiveTacticalEffectsContainer::GetTacticalAttributeValueChangeDelegate(FGameplayAttribute Attribute)
{
    return mAttributeValueChangeDelegates.FindOrAdd(Attribute);
}

void FActiveTacticalEffectsContainer::InternalOnActiveTacticalEffectAdded(FActiveTacticalEffect& Effect)
{
    const UTacticalEffect* EffectDef = Effect.mSpec.mEffectClass;
    checkf(EffectDef != nullptr, TEXT("추가한 Effect Class가 nullptr"));

    EffectDef->OnAddedToActiveContainer(*this, Effect);
}

void FActiveTacticalEffectsContainer::InternalOnActiveTacticalEffectRemoved(FActiveTacticalEffect& Effect, const FTacticalEffectRemovalInfo& TacticalEffectRemovalInfo)
{
    Effect.mIsPendingRemove = true;
    Effect.mEventSet.OnEffectRemoved.Broadcast(TacticalEffectRemovalInfo);

    OnGivenActiveTacticalEffectRemovedDelegate.Broadcast(Effect);
}

bool FActiveTacticalEffectsContainer::InternalExecuteMod(FTacticalEffectSpec& Spec, FGameplayModifierEvaluatedData& ModEvalData)
{
    check(mOwner.IsValid() == true);

    bool Executed = false;

    UAttributeSet* AttributeSet = nullptr;
    UClass* AttributeSetClass = ModEvalData.Attribute.GetAttributeSetClass();
    if (AttributeSetClass != nullptr && AttributeSetClass->IsChildOf(UAttributeSet::StaticClass()) == true)
    {
        AttributeSet = const_cast<UAttributeSet*>(mOwner->GetAttributeSet_Internal(AttributeSetClass));
    }

    if (AttributeSet != nullptr)
    {
        float OldValueOfProperty = mOwner->GetAttributeCurrentValue(ModEvalData.Attribute);
        ApplyModToAttribute(ModEvalData.Attribute, ModEvalData.ModifierOp, ModEvalData.Magnitude);

        Executed = true;
    }

    return Executed;
}

bool FActiveTacticalEffectsContainer::InternalRemoveActiveTacticalEffect(int32 Idx, int32 StacksToRemove, bool bPrematureRemoval)
{
    bool IsLocked = (mScopedLockCount > 0);
    TACTICAL_EFFECT_SCOPE_LOCK();

    if (Idx < GetNumTacticalEffects())
    {
        FActiveTacticalEffect& Effect = *GetActiveTacticalEffect(Idx);
        if (!ensure(!Effect.mIsPendingRemove))
        {
            return true;
        }

        FTacticalEffectRemovalInfo GameplayEffectRemovalInfo;
        GameplayEffectRemovalInfo.mActiveEffect = &Effect;
        GameplayEffectRemovalInfo.mStackCount = Effect.mSpec.GetStackCount();
        GameplayEffectRemovalInfo.mEffectContext = Effect.mSpec.GetContext();

        if (StacksToRemove > 0 && Effect.mSpec.GetStackCount() > StacksToRemove)
        {
            int32 StartingStackCount = Effect.mSpec.GetStackCount();
            Effect.mSpec.SetStackCount(StartingStackCount - StacksToRemove);
            OnStackCountChange(Effect, StartingStackCount, Effect.mSpec.GetStackCount());
            return false;
        }

        InternalOnActiveTacticalEffectRemoved(Effect, GameplayEffectRemovalInfo);

        bool ModifiedArray = false;
        if (IsLocked == true)
        {
            mPendingRemoveCount++;
        }
        else
        {
            Effect.mHandle.RemoveFromGlobalMap();
            check(Idx < mTacticalEffects.Num());
            mTacticalEffects.RemoveAtSwap(Idx);
            ModifiedArray = true;
        }
        return ModifiedArray;
    }

    return false;
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
    CurrentValue = mOwner->GetAttributeCurrentValue(Attribute);

    if (FOnChangeAttributeValue* NewDelegate = mAttributeValueChangeDelegates.Find(Attribute))
    {
        FTacticalAttributeChangeData CallbackData;
        CallbackData.mAttribute = Attribute;
        CallbackData.mNewValue = CurrentValue;
        CallbackData.mOldValue = OldValue;
        NewDelegate->Broadcast(CallbackData);
    }
}

void FActiveTacticalEffectsContainer::UpdateAllAggregatorModMagnitudes(FActiveTacticalEffect& ActiveEffect)
{
    const FTacticalEffectSpec& Spec = ActiveEffect.mSpec;
    checkf(Spec.mEffectClass == nullptr, TEXT("추가하려는 Effect Class가 보이지 않음"));

    TSet<FGameplayAttribute> AttributesToUpdate;
    for (int32 ModIdx = 0; ModIdx < Spec.mModifierValues.Num(); ++ModIdx)
    {
        const FTacticalModifierInfo& ModDef = Spec.mEffectClass->mModifiers[ModIdx];
        AttributesToUpdate.Add(ModDef.mAttribute);
    }

    // 변경된 Attribute 갱신
    UpdateAggregatorModMagnitudes(AttributesToUpdate, ActiveEffect);
}

void FActiveTacticalEffectsContainer::UpdateAggregatorModMagnitudes(const TSet<FGameplayAttribute>& AttributesToUpdate, FActiveTacticalEffect& ActiveEffect)
{
    const FTacticalEffectSpec& Spec = ActiveEffect.mSpec;
    for (const FGameplayAttribute& Attribute : AttributesToUpdate)
    {
        if (mOwner == nullptr || mOwner->HasAttributeSetForAttribute(Attribute) == false)
        {
            continue;
        }

        FTacticalAggregator* Aggregator = FindOrCreateAttributeAggregator(Attribute).Get();
        checkf(Aggregator != nullptr, TEXT("Aggregator 미 생성 오류"));

        Aggregator->UpdateAggregatorMod(ActiveEffect.mHandle, Attribute, Spec, ActiveEffect.mHandle);
    }
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

FActiveTacticalEffect* FActiveTacticalEffectsContainer::GetActiveTacticalEffect(int32 Index)
{
    if (Index < mTacticalEffects.Num())
    {
        return &mTacticalEffects[Index];
    }

    Index -= mTacticalEffects.Num();
    FActiveTacticalEffect* Ptr = mPendingTacticalEffectHead;
    FActiveTacticalEffect* Stop = *mPendingTacticalEffectTail;

    while (Index-- > 0 && Ptr != nullptr && Ptr != Stop && Ptr->mPendingNext != Stop)
    {
        Ptr = Ptr->mPendingNext;
    }

    return Index <= 0 ? Ptr : nullptr;
}

const FActiveTacticalEffect* FActiveTacticalEffectsContainer::GetActiveTacticalEffect(int32 Index) const
{
    return const_cast<FActiveTacticalEffectsContainer*>(this)->GetActiveTacticalEffect(Index);
}

int32 FActiveTacticalEffectsContainer::GetNumTacticalEffects() const
{
    int32 NumPending = 0;
    FActiveTacticalEffect* PendingGameplayEffect = mPendingTacticalEffectHead;
    FActiveTacticalEffect* Stop = *mPendingTacticalEffectTail;
    while (PendingGameplayEffect && PendingGameplayEffect != Stop)
    {
        ++NumPending;
        PendingGameplayEffect = PendingGameplayEffect->mPendingNext;
    }

    return mTacticalEffects.Num() + NumPending;
}

FActiveTacticalEffect* FActiveTacticalEffectsContainer::FindStackableActiveTacticalEffect(const FTacticalEffectSpec& Spec)
{
	FActiveTacticalEffect* FoundStackableEffect = nullptr;
	const UTacticalEffect* EffectClass = Spec.mEffectClass;
	ETacticalEffectStackingType StackingType = EffectClass->mStackingType;

	if ((StackingType != ETacticalEffectStackingType::None) && (EffectClass->mDurationPolicy != ETacticalEffectDurationType::Instant))
	{
		UAttributeSetComponentModel* SourceASCModel = Spec.GetContext()->GetAttributeSetComponentModel();
		for (FActiveTacticalEffect& ActiveEffect : this)
		{
			if (ActiveEffect.mSpec.mEffectClass == EffectClass && ((StackingType == ETacticalEffectStackingType::AggregateByTarget) || (SourceASCModel != nullptr && SourceASCModel == ActiveEffect.mSpec.GetContext()->GetAttributeSetComponentModel())))
			{
				FoundStackableEffect = &ActiveEffect;
				break;
			}
		}
	}
	return FoundStackableEffect;
}

