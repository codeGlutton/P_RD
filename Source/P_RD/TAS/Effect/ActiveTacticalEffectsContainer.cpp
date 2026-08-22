#include "TAS/Effect/ActiveTacticalEffectsContainer.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "TAS/Effect/TacticalEffectQuery.h"
#include "TAS/Aggregator/TacticalAggregator.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "TAS/Calculation/TacticalEffectExecutionCalculation.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

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
    // tail은 "다음 노드를 끼울 자리"를 가리키는 이중 포인터. 비어 있을 땐 head 자체를 가리킨다.
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
    mPendingTacticalEffectTail = &mPendingTacticalEffectHead;
}

void FActiveTacticalEffectsContainer::PostDuplicate(bool DuplicateForPIE)
{
    mScopedLockCount = 0;
    mPendingRemoveCount = 0;
    mPendingTacticalEffectHead = nullptr;
    mPendingTacticalEffectTail = &mPendingTacticalEffectHead;

    mAttributeAggregatorMap.Empty();
    mAttributeValueChangeDelegates.Empty();
    mSourceStackingMap.Empty();

    for (FActiveTacticalEffect& Effect : mTacticalEffects)
    {
        if (Effect.mIsPendingRemove == false && Effect.mSpec.mEffectClass != nullptr)
        {
            UpdateAllAggregatorModMagnitudes(Effect);
        }
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

        // 지연 추가 연결 리스트를 head부터 tail 직전까지 순회한다.
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

                // 추가되기도 전에 제거 표시된 노드는 본 배열로 옮기지 않고, 예약 제거 카운트만 상쇄한다.
                mPendingRemoveCount--;
            }
            CurPendingEffect = CurPendingEffect->mPendingNext;
        }
        // 지연 리스트를 다시 빈 상태로 되돌린다(노드 메모리는 재사용을 위해 유지).
        mPendingTacticalEffectTail = &mPendingTacticalEffectHead;

        /* 제거 작업 */

        // RemoveAtSwap을 쓰므로 뒤에서 앞으로(역순) 순회해야 인덱스가 흔들리지 않는다.
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

        // 위 두 루프가 끝나면 예약된 제거가 정확히 모두 소진되어 있어야 한다.
        checkf(mPendingRemoveCount == 0, TEXT("PendingRemove 잔존 : %d"), mPendingRemoveCount);
    }
}

TSharedPtr<FTacticalAggregator>& FActiveTacticalEffectsContainer::FindOrCreateAttributeAggregator(const FTacticalAttribute& Attribute)
{
    TSharedPtr<FTacticalAggregator>* RefPtr = mAttributeAggregatorMap.Find(Attribute);
    if (RefPtr != nullptr)
    {
        return *RefPtr;
    }

    float CurrentBaseValueOfProperty = mOwner->GetAttributeBaseValue(Attribute);
    TSharedPtr<FTacticalAggregator> NewAttributeAggregator = MakeShared<FTacticalAggregator>(mOwner.Get(), CurrentBaseValueOfProperty);
    // Aggregator가 dirty(모디파이어 추가/변경)될 때마다 소유자에게 알려 current 값을 다시 평가하게 한다.
    NewAttributeAggregator->OnDirty.AddUObject(mOwner.Get(), &UAttributeSetComponentModel::OnAttributeAggregatorDirty, Attribute);

    return mAttributeAggregatorMap.Add(Attribute, MoveTemp(NewAttributeAggregator));
}

void FActiveTacticalEffectsContainer::CleanupAttributeAggregator(const FTacticalAttribute& Attribute)
{
    TSharedPtr<FTacticalAggregator>* AggregatorPtr = mAttributeAggregatorMap.Find(Attribute);
    if (AggregatorPtr != nullptr)
    {
        (*AggregatorPtr)->OnDirty.RemoveAll(mOwner.Get());

        mAttributeAggregatorMap.Remove(Attribute);
    }
}

void FActiveTacticalEffectsContainer::OnAttributeAggregatorDirty(FTacticalAggregator* Aggregator, FTacticalAttribute Attribute)
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
        // 스택 수에 비례/거듭제곱하는 모디파이어가 있으므로 Aggregator 모디파이어 크기를 다시 산정한다.
        UpdateAllAggregatorModMagnitudes(ActiveEffect);
    }

    if (ActiveEffect.mSpec.mEffectClass != nullptr)
    {
        mOwner->NotifyTagMap_StackCountChange(ActiveEffect.mSpec.mEffectClass->GetGrantedTags());
    }

    ActiveEffect.mEventSet.OnStackChanged.Broadcast(ActiveEffect.mHandle, ActiveEffect.mSpec.GetStackCount(), OldStackCount);
}

void FActiveTacticalEffectsContainer::OnDurationChange(FActiveTacticalEffect& ActiveEffect)
{
    ActiveEffect.mEventSet.OnTimeChanged.Broadcast(ActiveEffect.mHandle, ActiveEffect.mStartTime, ActiveEffect.GetDuration());
    mOwner->OnTacticalEffectDurationChange(ActiveEffect);
}

void FActiveTacticalEffectsContainer::ApplyModToAttribute(const FTacticalAttribute& Attribute, TEnumAsByte<ETacticalModOp::Type> ModifierOp, float ModifierMagnitude)
{
    float CurrentBase = GetAttributeBaseValue(Attribute);
    float NewBase = FTacticalAggregator::StaticExecModOnBaseValue(CurrentBase, ModifierOp, ModifierMagnitude);

    SetAttributeBaseValue(Attribute, NewBase);
}

FActiveTacticalEffect* FActiveTacticalEffectsContainer::ApplyTacticalEffectSpec(const FTacticalEffectSpec& Spec, OUT bool& FoundExistingStackableGE)
{
	TACTICAL_EFFECT_SCOPE_LOCK();

	checkf(Spec.mEffectClass != nullptr, TEXT("적용하려는 Effect 클래스가 없음"));
	FoundExistingStackableGE = false;

	FActiveTacticalEffect* AppliedActiveEffect = nullptr;
	FActiveTacticalEffect* ExistingStackableEffect = FindStackableActiveTacticalEffect(Spec);

    bool IsSetDurationTimer = true;
    int32 CarryOverDuration = 0;

    int32 PreStackCount = 0;
	int32 NewStackCount = 0;

    if (Spec.IsValidDuration() == false)
    {
        return AppliedActiveEffect;
    }

	if (ExistingStackableEffect != nullptr)
	{
        /* 스태킹 Effect의 경우 */

        FoundExistingStackableGE = true;

		FTacticalEffectSpec& ExistingSpec = ExistingStackableEffect->mSpec;
		PreStackCount = ExistingSpec.GetStackCount();
        NewStackCount = ExistingSpec.GetStackCount() + Spec.GetStackCount();

        checkf(ExistingSpec.mDynamicMagnitude == Spec.mDynamicMagnitude, TEXT("스태킹되는 이펙트는 수치가 다를 수 없음"));

        if (ExistingSpec.mEffectClass->mStackDurationRefreshPolicy == ETacticalEffectStackingDurationPolicy::ExtendDuration)
        {
            int32 WorldTime = GetWorldTime(ExistingStackableEffect->GetDurationUnit());
            CarryOverDuration = ExistingStackableEffect->GetTimeRemaining(WorldTime);
        }

		ExistingStackableEffect->mSpec = Spec;
		ExistingStackableEffect->mSpec.SetStackCount(NewStackCount);

		AppliedActiveEffect = ExistingStackableEffect;

        const UTacticalEffect* EffectDef = ExistingSpec.mEffectClass;
        if (EffectDef->mStackDurationRefreshPolicy == ETacticalEffectStackingDurationPolicy::NeverRefresh)
        {
            IsSetDurationTimer = false;
        }
        else
        {
            RestartActiveTacticalEffectDuration(*ExistingStackableEffect);
        }
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

				AppliedActiveEffect = new FActiveTacticalEffect(NewHandle, Spec, GetWorldTime(Spec.mEffectClass->mDurationUnitPolicy));
				*mPendingTacticalEffectTail = AppliedActiveEffect;
			}
			else
			{
				/* 할당 공간이 남아있는 경우 */

				**mPendingTacticalEffectTail = FActiveTacticalEffect(NewHandle, Spec, GetWorldTime(Spec.mEffectClass->mDurationUnitPolicy));
				AppliedActiveEffect = *mPendingTacticalEffectTail;
			}
			mPendingTacticalEffectTail = &AppliedActiveEffect->mPendingNext;
		}
		else
		{
			/* 배열 크기 재할당이 필요없는 경우 */

			AppliedActiveEffect = new(mTacticalEffects) FActiveTacticalEffect(NewHandle, Spec, GetWorldTime(Spec.mEffectClass->mDurationUnitPolicy));
		}
	}

    FTacticalEffectSpec& AppliedEffectSpec = AppliedActiveEffect->mSpec;

    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mOwner.Get());
    check(TacticalFrameworkModel != nullptr);

    TacticalFrameworkModel->SetCurrentAppliedTE(&AppliedEffectSpec);
    TacticalFrameworkModel->GlobalPreTacticalEffectSpecApply(AppliedEffectSpec, mOwner.Get());

	// 적용된 Spec의 각 모디파이어 최종 크기를 미리 계산해 둔다.
	AppliedEffectSpec.CalculateModifierMagnitudes();

    // Duration의 경우, 속성 변경 로그 채우기
    {
        const bool HasModifiedAttributes = AppliedEffectSpec.mModifiedAttributes.Num() > 0;
        const bool HasDuration = AppliedEffectSpec.mEffectClass->mDurationPolicy == ETacticalEffectDurationType::Duration;
        const bool ShouldBuildModifiedAttributeList = HasModifiedAttributes == false && HasDuration == true;
        if (ShouldBuildModifiedAttributeList == true)
        {
            int32 ModifierIndex = -1;
            for (const FTacticalModifierInfo& Mod : AppliedEffectSpec.mEffectClass->mModifiers)
            {
                ++ModifierIndex;

                float Magnitude = 0.f;
                if (AppliedEffectSpec.mModifiedAttributes.IsValidIndex(ModifierIndex) == true)
                {
                    Magnitude = AppliedEffectSpec.mModifiers[ModifierIndex];
                }

                FTacticalEffectModifiedAttribute* ModifiedAttribute = AppliedEffectSpec.GetModifiedAttribute(Mod.mAttribute);
                if (ModifiedAttribute == nullptr)
                {
                    ModifiedAttribute = AppliedEffectSpec.AddModifiedAttribute(Mod.mAttribute);
                }
                ModifiedAttribute->mTotalMagnitude += Magnitude;
            }
        }
    }

    // Duration 계산
    int32 CalcDuration = 0;
    if (AppliedEffectSpec.AttemptCalculateDurationFromDef(OUT CalcDuration) == true)
    {
        AppliedEffectSpec.SetDuration(CalcDuration);
    }

    const int32 DurationBaseValue = AppliedEffectSpec.GetDuration();
    if (DurationBaseValue > 0)
    {
        float FinalDuration = DurationBaseValue;
        if (CarryOverDuration > 0)
        {
            FinalDuration += CarryOverDuration;
            AppliedEffectSpec.SetDuration(FinalDuration);
        }

        // 대리자 호출
        OnDurationChange(*AppliedActiveEffect);
    }

	if (ExistingStackableEffect != nullptr)
	{
		// 기존 이펙트에 스택이 합쳐진 경우: 스택 변경 후처리만 수행.
		OnStackCountChange(*ExistingStackableEffect, PreStackCount, NewStackCount);
	}
    else
    {
        // 새 이펙트가 추가된 경우: 태그/모디파이어 부여 등 추가 후처리 수행.
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

    check(SpecToUse.mModifiers.Num() == SpecToUse.mEffectClass->mModifiers.Num());
    for (int32 ModIdx = 0; ModIdx < SpecToUse.mModifiers.Num(); ++ModIdx)
    {
        const FTacticalModifierInfo& ModDef = SpecToUse.mEffectClass->mModifiers[ModIdx];

        FTacticalModifierEvaluatedData EvalData(ModDef.mAttribute, ModDef.mModifierOp, SpecToUse.GetStackedModifierMagnitude(ModIdx));
        ModifierSuccessfullyExecuted |= InternalExecuteMod(SpecToUse, EvalData);
    }

    /* 익스큐션 실행 */

    for (const FTacticalEffectExecutionDefinition& CurExecDef : SpecToUse.mEffectClass->mExecutions)
    {
        if (CurExecDef.mCalculationClass != nullptr)
        {
            const UTacticalEffectExecutionCalculation* ExecCDO = CurExecDef.mCalculationClass->GetDefaultObject<UTacticalEffectExecutionCalculation>();
            check(ExecCDO != nullptr);

            // 계산기 실행
            FTacticalEffectCustomExecutionParameters ExecutionParams(SpecToUse, mOwner.Get());
            FTacticalEffectCustomExecutionOutput ExecutionOutput;
            ExecCDO->Execute(ExecutionParams, ExecutionOutput);

            // 수정자 결과 내뱉기
            TArray<FTacticalModifierEvaluatedData>& OutModifiers = ExecutionOutput.GetOutputModifiersRef();

            const bool ApplyStackCountToEmittedMods = ExecutionOutput.IsStackCountHandledManually() == false;
            const int32 SpecStackCount = SpecToUse.GetStackCount();

            const bool ApplyDynamicMagnitudeToEmittedMods = ExecutionOutput.IsDynamicMagnitudeHandledManually() == false;
            const float DynamicMagnitude = ApplyDynamicMagnitudeToEmittedMods == true ? SpecToUse.mDynamicMagnitude : 1.f;

            // 수정자 결과 처리
            for (FTacticalModifierEvaluatedData& CurExecMod : OutModifiers)
            {
                // 스택 처리 필요시
                if (ApplyStackCountToEmittedMods == true && SpecStackCount > 1)
                {
                    CurExecMod.mMagnitude = TacticalEffectUtilities::ComputeStackedModifierMagnitude(CurExecMod.mMagnitude * DynamicMagnitude, SpecStackCount, CurExecMod.mModifierOp);
                }
                ModifierSuccessfullyExecuted |= InternalExecuteMod(SpecToUse, CurExecMod);
            }
        }
    }

    Spec.mEffectClass->OnExecuted(*this, Spec);
}

bool FActiveTacticalEffectsContainer::RemoveActiveTacticalEffect(FActiveTacticalEffectHandle Handle, int32 StacksToRemove)
{
    int32 NumTacticalEffects = GetNumTacticalEffects();
    for (int32 ActiveEffectIdx = 0; ActiveEffectIdx < NumTacticalEffects; ++ActiveEffectIdx)
    {
        FActiveTacticalEffect& Effect = *GetActiveTacticalEffect(ActiveEffectIdx);
        if (Effect.mHandle == Handle && Effect.mIsPendingRemove == false)
        {
            InternalRemoveActiveTacticalEffect(ActiveEffectIdx, StacksToRemove, true);
            return true;
        }
    }

    return false;
}

int32 FActiveTacticalEffectsContainer::RemoveActiveEffects(const FTacticalEffectQuery& Query, int32 StacksToRemove)
{
    TACTICAL_EFFECT_SCOPE_LOCK();
    int32 NumRemoved = 0;

    for (int32 ActiveEffectIdx = GetNumTacticalEffects() - 1; ActiveEffectIdx >= 0; --ActiveEffectIdx)
    {
        const FActiveTacticalEffect& Effect = *GetActiveTacticalEffect(ActiveEffectIdx);
        if (Effect.mIsPendingRemove == false && Query.Matches(Effect) == true)
        {
            InternalRemoveActiveTacticalEffect(ActiveEffectIdx, StacksToRemove, true);
            ++NumRemoved;
        }
    }
    return NumRemoved;
}

FOnChangeAttributeValue& FActiveTacticalEffectsContainer::GetTacticalAttributeValueChangeDelegate(FTacticalAttribute Attribute)
{
    return mAttributeValueChangeDelegates.FindOrAdd(Attribute);
}

void FActiveTacticalEffectsContainer::CheckDurationExpired(const int32 Time, const ETacticalEffectDurationUnitType UnitType)
{
    TACTICAL_EFFECT_SCOPE_LOCK();

    for (int32 ActiveGEIdx = 0; ActiveGEIdx < mTacticalEffects.Num(); ++ActiveGEIdx)
    {
        FActiveTacticalEffect& Effect = mTacticalEffects[ActiveGEIdx];
        
        if (Effect.mIsPendingRemove == true)
        {
            continue;
        }
        const int32 Duration = Effect.GetDuration();
        if (Duration <= 0)
        {
            continue;
        }
        if (Effect.GetDurationUnit() != UnitType)
        {
            continue;
        }

        Effect.mSpec.mEffectClass->OnReduceTimeRemaining(*this, Effect.mSpec);

        int32 StacksToRemove = -2;
        bool NeedToRefreshStartTime = false;

        /* Effect 만기 */

        if ((Effect.mStartTime + Duration) <= Time)
        {
            switch (Effect.mSpec.mEffectClass->mStackExpirationPolicy)
            {
            case ETacticalEffectStackingExpirationPolicy::ClearEntireStack:
                StacksToRemove = -1; // 모두 제거
                break;

            case ETacticalEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration:
                StacksToRemove = 1;
                NeedToRefreshStartTime = true;
                break;
            case ETacticalEffectStackingExpirationPolicy::RefreshDuration:
                NeedToRefreshStartTime = true;
                break;
            };
        }

        if (StacksToRemove >= -1)
        {
            InternalRemoveActiveTacticalEffect(ActiveGEIdx, StacksToRemove, false);
        }

        if (NeedToRefreshStartTime == true)
        {
            RestartActiveTacticalEffectDuration(Effect);
            OnDurationChange(Effect);
        }
    }
}

void FActiveTacticalEffectsContainer::InternalOnActiveTacticalEffectAdded(FActiveTacticalEffect& Effect)
{
    const UTacticalEffect* EffectDef = Effect.mSpec.mEffectClass;
    checkf(EffectDef != nullptr, TEXT("추가한 Effect Class가 nullptr"));

    TACTICAL_EFFECT_SCOPE_LOCK();

    EffectDef->OnAddedToActiveContainer(*this, Effect);

    FActiveTacticalEffectHandle EffectHandle = Effect.mHandle;
    if (mOwner.IsValid() == true)
    {
        mOwner->SetActiveTacticalEffect(MoveTemp(EffectHandle));
    }
}

void FActiveTacticalEffectsContainer::InternalOnActiveTacticalEffectRemoved(FActiveTacticalEffect& Effect, const FTacticalEffectRemovalInfo& TacticalEffectRemovalInfo)
{
    Effect.mIsPendingRemove = true;

    if (Effect.mSpec.mEffectClass)
    {
        RemoveActiveTacticalEffectGrantedTagsAndModifiers(Effect);
    }

    Effect.mEventSet.OnEffectRemoved.Broadcast(TacticalEffectRemovalInfo);
    OnGivenActiveTacticalEffectRemovedDelegate.Broadcast(Effect);
}

bool FActiveTacticalEffectsContainer::InternalExecuteMod(FTacticalEffectSpec& Spec, FTacticalModifierEvaluatedData& ModEvalData)
{
    check(mOwner.IsValid() == true);

    bool Executed = false;

    UTacticalAttributeSet* AttributeSet = nullptr;
    UClass* AttributeSetClass = ModEvalData.mAttribute.GetAttributeSetClass();
    
    if (AttributeSetClass != nullptr && AttributeSetClass->IsChildOf(UTacticalAttributeSet::StaticClass()) == true)
    {
        AttributeSet = const_cast<UTacticalAttributeSet*>(mOwner->GetAttributeSet_Internal(AttributeSetClass));
    }

    if (AttributeSet != nullptr)
    {
        float OldValueOfProperty = mOwner->GetAttributeCurrentValue(ModEvalData.mAttribute);
        ApplyModToAttribute(ModEvalData.mAttribute, ModEvalData.mModifierOp, ModEvalData.mMagnitude);

        FTacticalEffectModifiedAttribute* ModifiedAttribute = Spec.GetModifiedAttribute(ModEvalData.mAttribute);
        if (ModifiedAttribute == nullptr)
        {
            ModifiedAttribute = Spec.AddModifiedAttribute(ModEvalData.mAttribute);
        }
        ModifiedAttribute->mTotalMagnitude += ModEvalData.mMagnitude;

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
        if (!ensure(Effect.mIsPendingRemove == false))
        {
            return true;
        }

        FTacticalEffectRemovalInfo GameplayEffectRemovalInfo;
        GameplayEffectRemovalInfo.mActiveEffect = &Effect;
        GameplayEffectRemovalInfo.mStackCount = Effect.mSpec.GetStackCount();
        GameplayEffectRemovalInfo.mEffectContext = Effect.mSpec.GetContext();

        if (StacksToRemove > 0 && Effect.mSpec.GetStackCount() > StacksToRemove)
        {
            // 보유 스택보다 적게 제거하는 경우: 이펙트는 유지하고 스택 수만 감소시킨다.
            int32 StartingStackCount = Effect.mSpec.GetStackCount();
            Effect.mSpec.SetStackCount(StartingStackCount - StacksToRemove);
            OnStackCountChange(Effect, StartingStackCount, Effect.mSpec.GetStackCount());
            return false;
        }

        InternalOnActiveTacticalEffectRemoved(Effect, GameplayEffectRemovalInfo);

        bool ModifiedArray = false;
        if (IsLocked == true)
        {
            // Lock 중: 즉시 제거하지 않고 카운트만 올려 Lock 해제 시 일괄 제거(DecrementLock)되게 한다.
            mPendingRemoveCount++;
        }
        else
        {
            // Lock이 없으면 즉시 글로벌 맵에서 핸들을 지우고 배열에서 swap 제거.
            Effect.mHandle.RemoveFromGlobalMap();
            check(Idx < mTacticalEffects.Num());
            mTacticalEffects.RemoveAtSwap(Idx);
            ModifiedArray = true;
        }
        return ModifiedArray;
    }

    return false;
}

void FActiveTacticalEffectsContainer::AddActiveTacticalEffectGrantedTagsAndModifiers(FActiveTacticalEffect& Effect)
{
    check(Effect.mSpec.mEffectClass != nullptr);
    check(mOwner != nullptr);

    TACTICAL_EFFECT_SCOPE_LOCK();

    for (int32 ModIdx = 0; ModIdx < Effect.mSpec.mModifiers.Num(); ++ModIdx)
    {
        if (Effect.mSpec.mEffectClass->mModifiers.IsValidIndex(ModIdx) == false)
        {
            continue;
        }

        const FTacticalModifierInfo& ModInfo = Effect.mSpec.mEffectClass->mModifiers[ModIdx];
        if (mOwner->HasAttributeSetForAttribute(ModInfo.mAttribute) == false)
        {
            // 소유자가 해당 속성의 AttributeSet을 갖고 있지 않는 경우
            continue;
        }

        float EvaluatedMagnitude = Effect.mSpec.GetStackedModifierMagnitude(ModIdx);
        FTacticalAggregator* Aggregator = FindOrCreateAttributeAggregator(Effect.mSpec.mEffectClass->mModifiers[ModIdx].mAttribute).Get();
        if (ensure(Aggregator))
        {
            // op(ETacticalModOp)별 버킷에 모디파이어를 추가
            Aggregator->AddAggregatorMod(EvaluatedMagnitude, ModInfo.mModifierOp, Effect.mHandle);
        }
    }

    // 부여 태그를 +1 카운트로 태그 맵에 반영하고, 추가 사실을 소유자에게 알림
    mOwner->UpdateTagMap(Effect.mSpec.mEffectClass->GetGrantedTags(), 1);
    mOwner->OnActiveTacticalEffectAddedDelegateToSelf.Broadcast(mOwner.Get(), Effect.mSpec, Effect.mHandle);
}

void FActiveTacticalEffectsContainer::RemoveActiveTacticalEffectGrantedTagsAndModifiers(const FActiveTacticalEffect& Effect)
{
    for (const FTacticalModifierInfo& Mod : Effect.mSpec.mEffectClass->mModifiers)
    {
        if (Mod.mAttribute.IsValid() == true)
        {
            if (const TSharedPtr<FTacticalAggregator>* RefPtr = mAttributeAggregatorMap.Find(Mod.mAttribute))
            {
                // 이 이펙트 핸들로 등록됐던 모디파이어들을 op 버킷에서 제거
                RefPtr->Get()->RemoveAggregatorMod(Effect.mHandle);
            }
        }
    }
    // 등록되었던 Tag도 제거함에 따라 같이 내리기
    mOwner->UpdateTagMap(Effect.mSpec.mEffectClass->GetGrantedTags(), -1);
}

void FActiveTacticalEffectsContainer::SetAttributeBaseValue(FTacticalAttribute Attribute, float BaseValue)
{
    checkf(mOwner != nullptr, TEXT("변경 ASC 대상이 존재하지 않음"));
    const UTacticalAttributeSet* Set = mOwner->GetAttributeSet_Internal(Attribute.GetAttributeSetClass());
    checkf(Set != nullptr, TEXT("변경 AttributeSet 대상이 존재하지 않음"));

    float OldBaseValue = 0.0f;
    Set->PreAttributeBaseChange(Attribute, BaseValue);

    /* 속성에서 베이스 값 변경 */

    const FStructProperty* StructProperty = CastField<FStructProperty>(Attribute.GetUProperty());
    checkf(StructProperty != nullptr, TEXT("변경 속성 대상이 존재하지 않음"));
    FTacticalAttributeData* DataPtr = StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(const_cast<UTacticalAttributeSet*>(Set));
    checkf(DataPtr != nullptr, TEXT("변경 FTacticalAttributeData 대상이 존재하지 않음"));
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

float FActiveTacticalEffectsContainer::GetAttributeBaseValue(FTacticalAttribute Attribute) const
{
    float BaseValue = 0.f;
    if (mOwner != nullptr)
    {
        const UTacticalAttributeSet* AttributeSet = mOwner->GetAttributeSet_Internal(Attribute.GetAttributeSetClass());
        checkf(AttributeSet != nullptr, TEXT("탐색 AttributeSet 대상이 존재하지 않음"));

        const TSharedPtr<FTacticalAggregator>* RefPtr = mAttributeAggregatorMap.Find(Attribute);

        if (FTacticalAttribute::IsTacticalAttributeDataProperty(Attribute.GetUProperty()))
        {
            const FStructProperty* StructProperty = CastField<FStructProperty>(Attribute.GetUProperty());
            checkf(StructProperty != nullptr, TEXT("탐색 속성 대상이 존재하지 않음"));
            const FTacticalAttributeData* DataPtr = StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(AttributeSet);
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

void FActiveTacticalEffectsContainer::UpdateAttributeCurrentValue(FTacticalAttribute Attribute, float CurrentValue)
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
    checkf(Spec.mEffectClass != nullptr, TEXT("추가하려는 Effect Class가 보이지 않음"));

    // 이 이펙트의 모든 모디파이어가 영향을 주는 속성 집합을 중복 없이 수집
    TSet<FTacticalAttribute> AttributesToUpdate;
    for (int32 ModIdx = 0; ModIdx < Spec.mModifiers.Num(); ++ModIdx)
    {
        const FTacticalModifierInfo& ModDef = Spec.mEffectClass->mModifiers[ModIdx];
        AttributesToUpdate.Add(ModDef.mAttribute);
    }

    // 변경된 Attribute 갱신
    UpdateAggregatorModMagnitudes(AttributesToUpdate, ActiveEffect);
}

void FActiveTacticalEffectsContainer::UpdateAggregatorModMagnitudes(const TSet<FTacticalAttribute>& AttributesToUpdate, FActiveTacticalEffect& ActiveEffect)
{
    const FTacticalEffectSpec& Spec = ActiveEffect.mSpec;
    for (const FTacticalAttribute& Attribute : AttributesToUpdate)
    {
        if (mOwner == nullptr || mOwner->HasAttributeSetForAttribute(Attribute) == false)
        {
            continue;
        }

        FTacticalAggregator* Aggregator = FindOrCreateAttributeAggregator(Attribute).Get();
        checkf(Aggregator != nullptr, TEXT("Aggregator 미 생성 오류"));

        // 핸들로 식별되는 기존 모디파이어를 현재 Spec 기준 크기로 갱신(op 종류는 유지)
        Aggregator->UpdateAggregatorMod(ActiveEffect.mHandle, Attribute, Spec, ActiveEffect.mHandle);
    }
}

void FActiveTacticalEffectsContainer::RestartActiveTacticalEffectDuration(FActiveTacticalEffect& ActiveTacticalEffect)
{
    ActiveTacticalEffect.mStartTime = GetWorldTime(ActiveTacticalEffect.GetDurationUnit());
}

void FActiveTacticalEffectsContainer::CaptureAllEffectStacks(UBoardCombatTargetSnapshotData* Snapshot) const
{
    for (const FActiveTacticalEffect& Effect : this)
    {
        for (const FGameplayTag& AssetTag : Effect.mSpec.mEffectClass->GetAssetTags())
        {
            Snapshot->mEffectCounts.FindOrAdd(AssetTag) += Effect.mSpec.GetStackCount();
        }
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

    // 본 배열 범위를 넘으면 지연 리스트에서 상대 인덱스만큼 전진
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

TArray<FActiveTacticalEffectHandle> FActiveTacticalEffectsContainer::GetActiveEffects(const FTacticalEffectQuery& Query) const
{
    TArray<FActiveTacticalEffectHandle> ReturnList;

    for (const FActiveTacticalEffect& Effect : this)
    {
        if (Query.Matches(Effect) == false)
        {
            continue;
        }
        ReturnList.Add(Effect.mHandle);
    }

    return ReturnList;
}

TArray<float> FActiveTacticalEffectsContainer::GetActiveEffectsTimeRemaining(const FTacticalEffectQuery& Query, ETacticalEffectDurationUnitType UnitType) const
{
    float CurrentTime = GetWorldTime(UnitType);

    TArray<float> ReturnList;
    for (const FActiveTacticalEffect& Effect : this)
    {
        if (Query.Matches(Effect) == false)
        {
            continue;
        }

        float Elapsed = CurrentTime - Effect.mStartTime;
        float Duration = Effect.GetDuration();

        ReturnList.Add(Duration - Elapsed);
    }

    return ReturnList;
}

TArray<float> FActiveTacticalEffectsContainer::GetActiveEffectsDuration(const FTacticalEffectQuery& Query) const
{
    TArray<float> ReturnList;
    for (const FActiveTacticalEffect& Effect : this)
    {
        if (Query.Matches(Effect) == false)
        {
            continue;
        }

        ReturnList.Add(Effect.GetDuration());
    }

    return ReturnList;
}

TArray<TPair<float, float>> FActiveTacticalEffectsContainer::GetActiveEffectsTimeRemainingAndDuration(const FTacticalEffectQuery& Query, ETacticalEffectDurationUnitType UnitType) const
{
    float CurrentTime = GetWorldTime(UnitType);

    TArray<TPair<float, float>> ReturnList;
    for (const FActiveTacticalEffect& Effect : this)
    {
        if (Query.Matches(Effect) == false)
        {
            continue;
        }

        float Elapsed = CurrentTime - Effect.mStartTime;
        float Duration = Effect.GetDuration();

        ReturnList.Emplace(Duration - Elapsed, Duration);
    }

    return ReturnList;
}

float FActiveTacticalEffectsContainer::GetTacticalEffectMagnitude(FActiveTacticalEffectHandle Handle, const FTacticalAttribute& Attribute) const
{
    for (const FActiveTacticalEffect& Effect : this)
    {
        if (Effect.mHandle == Handle)
        {
            for (int32 ModIdx = 0; ModIdx < Effect.mSpec.mModifiers.Num(); ++ModIdx)
            {
                const FTacticalModifierInfo& ModDef = Effect.mSpec.mEffectClass->mModifiers[ModIdx];
                const float ModSpec = Effect.mSpec.mModifiers[ModIdx];

                if (ModDef.mAttribute == Attribute)
                {
                    return ModSpec;
                }
            }
            return -1.f;
        }
    }
    return -1.f;
}

int32 FActiveTacticalEffectsContainer::GetActiveEffectCount(const FTacticalEffectQuery& Query) const
{
    int32 Count = 0;
    for (const FActiveTacticalEffect& Effect : this)
    {
        if (Query.Matches(Effect))
        {
            Count += Effect.mSpec.GetStackCount();
        }
    }
    return Count;
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

int32 FActiveTacticalEffectsContainer::GetWorldTime(ETacticalEffectDurationUnitType UnitType) const
{
    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mOwner.Get());
    if (TacticalFrameworkModel == nullptr)
    {
        return INDEX_NONE;
    }
    return TacticalFrameworkModel->GetWorldTime(UnitType);
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

