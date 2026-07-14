#include "TAS/Effect/ActiveTacticalEffectsContainer.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "TAS/Aggregator/TacticalAggregator.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "TAS/Calculation/TacticalEffectExecutionCalculation.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"

/**
 * @file ActiveTacticalEffectsContainer.cpp
 * @brief 한 유닛(AttributeSetComponentModel)에 적용 중인 Tactical Effect들을 보관/실행/제거하는 컨테이너 구현.
 *
 *        이 컨테이너는 GAS의 FActiveGameplayEffectsContainer를 대체하는 자체 구현이다.
 *        주요 책임:
 *          - 이펙트 적용/스태킹/제거 (Apply/Remove, Scope Lock으로 순회 중 안전한 지연 추가/제거)
 *          - 모디파이어를 Attribute에 반영 (Aggregator를 통한 current value 계산, 또는 base value 직접 실행)
 *          - 모디파이어 "연산 종류"는 본 PR(#191)에서 GAS의 EGameplayModOp 대신 자체 ETacticalModOp를 사용한다.
 *            연산 종류 enum 치환이 이 파일 전반에 걸쳐 나타나므로 해당 지점마다 별도 주석을 둔다.
 */

/**
 * @brief Scope 진입 시 컨테이너 Lock을 증가시키는 RAII 가드 생성자.
 *        Lock이 걸린 동안에는 mTacticalEffects 배열을 직접 추가/삭제하지 않고 지연(pending)시켜,
 *        이펙트 배열을 순회하는 도중 배열이 재할당/이동되어 참조가 깨지는 것을 막는다.
 * @param Container Lock을 적용할 대상 컨테이너.
 */
FScopedActiveTacticalEffectLock::FScopedActiveTacticalEffectLock(FActiveTacticalEffectsContainer& Container) :
    mContainer(Container)
{
    mContainer.IncrementLock();
}

/**
 * @brief Scope 이탈 시 Lock을 감소시키는 소멸자.
 *        Lock 카운트가 0이 되면 그동안 미뤄둔 추가/제거 작업이 DecrementLock 내부에서 일괄 처리된다.
 */
FScopedActiveTacticalEffectLock::~FScopedActiveTacticalEffectLock()
{
    mContainer.DecrementLock();
}

/**
 * @brief 컨테이너 기본 생성자.
 *        지연 이펙트 연결 리스트의 tail 포인터를 head를 가리키도록 초기화한다(빈 리스트 상태).
 */
FActiveTacticalEffectsContainer::FActiveTacticalEffectsContainer() :
    mOwner(nullptr),
    mScopedLockCount(0),
    mPendingRemoveCount(0),
    mPendingTacticalEffectHead(nullptr)
{
    // tail은 "다음 노드를 끼울 자리"를 가리키는 이중 포인터. 비어 있을 땐 head 자체를 가리킨다.
    mPendingTacticalEffectTail = &mPendingTacticalEffectHead;
}

/**
 * @brief 소멸자. 아직 본 배열로 합쳐지지 못한 지연(pending) 이펙트 노드들을 직접 delete로 정리한다.
 *        (지연 노드는 new로 동적 할당되므로 별도 해제가 필요하다.)
 */
FActiveTacticalEffectsContainer::~FActiveTacticalEffectsContainer()
{
    while (mPendingTacticalEffectHead != nullptr)
    {
        FActiveTacticalEffect* Next = mPendingTacticalEffectHead->mPendingNext;
        delete mPendingTacticalEffectHead;
        mPendingTacticalEffectHead = Next;
    }
}

/**
 * @brief 이 컨테이너의 소유자(Attribute를 보유한 컴포넌트 모델)를 등록한다.
 * @param Owner 소유 컴포넌트 모델. nullptr이거나 이미 동일하면 무시한다.
 */
void FActiveTacticalEffectsContainer::RegisterWithOwnerModel(UAttributeSetComponentModel* Owner)
{
    if (mOwner != Owner && Owner != nullptr)
    {
        mOwner = Owner;
    }
    mPendingTacticalEffectTail = &mPendingTacticalEffectHead;
}

/**
 * @brief 복제(예: PIE 복제) 직후 호출. 런타임 전용 상태를 초기화하고 Aggregator를 재구축한다.
 *        Lock/지연 리스트/Aggregator 맵/델리게이트/스태킹 맵 등 비직렬화 상태를 깨끗이 비운 뒤,
 *        살아있는 각 이펙트에 대해 Aggregator 모디파이어 크기를 다시 계산해 채워 넣는다.
 * @param DuplicateForPIE PIE 복제 여부(현재 분기에는 사용하지 않음).
 */
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

/**
 * @brief Scope Lock 카운트를 1 증가시킨다. 0보다 크면 추가/제거가 즉시 반영되지 않고 지연된다.
 */
void FActiveTacticalEffectsContainer::IncrementLock()
{
    mScopedLockCount++;
}

/**
 * @brief Scope Lock 카운트를 1 감소시킨다. 0이 되는 순간 그동안 미뤄둔 추가/제거를 일괄 처리한다.
 *        처리 순서: (1) 지연 추가 노드를 본 배열로 합치고, (2) pendingRemove로 표시된 항목을 실제로 제거한다.
 */
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
        // [방어 가드] 위 가드로 flush가 중단된 손상 상황에서 checkf 어설트가 2차 크래시를 내지 않도록,
        // 불변식 위반을 기록 후 복구한다(정상 경로에선 원본 checkf와 동일하게 침묵).
        if (mPendingRemoveCount != 0)
        {
            UE_LOG(LogTemp, Error, TEXT("[TASDBG] DecrementLock: pendingRemove 잔존 %d — 예약된 이펙트 증감 로직 에러(원본 checkf 지점)"), mPendingRemoveCount);
            mPendingRemoveCount = 0;
        }
    }
}

/**
 * @brief 특정 Attribute에 대응하는 Aggregator를 찾고, 없으면 생성해 맵에 등록한다.
 *        Aggregator는 base 값과 모든 모디파이어를 모아 current 값을 계산하는 객체다.
 *        새로 만들 때 현재 base 값으로 초기화하고, OnDirty(재계산 필요) 델리게이트를 소유자에 연결한다.
 * @param Attribute 대상 속성.
 * @return 해당 속성의 Aggregator 공유 포인터 참조(맵 슬롯에 대한 참조).
 */
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

/**
 * @brief 특정 Attribute의 Aggregator를 맵에서 제거하고 OnDirty 바인딩을 해제한다.
 * @param Attribute 정리할 속성.
 */
void FActiveTacticalEffectsContainer::CleanupAttributeAggregator(const FTacticalAttribute& Attribute)
{
    TSharedPtr<FTacticalAggregator>* AggregatorPtr = mAttributeAggregatorMap.Find(Attribute);
    if (AggregatorPtr != nullptr)
    {
        (*AggregatorPtr)->OnDirty.RemoveAll(mOwner.Get());

        mAttributeAggregatorMap.Remove(Attribute);
    }
}

/**
 * @brief Aggregator가 dirty 상태가 됐을 때 호출되는 콜백.
 *        Aggregator를 다시 평가(Evaluate)해 얻은 새 current 값으로 속성 값을 갱신한다.
 * @param Aggregator dirty가 발생한 Aggregator(맵에 등록된 것과 동일해야 함).
 * @param Attribute 해당 Aggregator가 담당하는 속성.
 */
void FActiveTacticalEffectsContainer::OnAttributeAggregatorDirty(FTacticalAggregator* Aggregator, FTacticalAttribute Attribute)
{
    // 전투 종료(패배 포함) 일괄 정리 중에는 CleanupAttributeAggregator로 맵에서 빠진 Aggregator가
    // 뒤늦게 dirty를 쏠 수 있다. 이때 FindChecked는 assert로 즉사(패배 시 크래시 실측)하므로,
    // 스테일 통지는 무시하고 반환한다. 정리 중이 아닌데 불일치하면 로직 오류이므로 경고만 남긴다.
    /*const TSharedPtr<FTacticalAggregator>* RegisteredAggregator = mAttributeAggregatorMap.Find(Attribute);
    if (RegisteredAggregator == nullptr || RegisteredAggregator->Get() != Aggregator)
    {
        UE_LOG(LogAttributeSetComp, Warning, TEXT("[%s] 맵에 없는(정리된) Aggregator의 dirty 통지 무시"), *Attribute.GetName());
        return;
    }*/
    checkf(mAttributeAggregatorMap.FindChecked(Attribute).Get() == Aggregator, TEXT("속성에 대한 계산 객체가 동일하지 않음"));

    const float NewCurrentValue = Aggregator->Evaluate();
    const float OldCurrentValue = mOwner->GetAttributeCurrentValue(Attribute);
    UE_LOG(LogAttributeSetComp, Log, TEXT("[%s] 현재 값 변경 %.2f -> %.2f"), *Attribute.GetName(), OldCurrentValue, NewCurrentValue);

    UpdateAttributeCurrentValue(Attribute, NewCurrentValue);
}

/**
 * @brief 모디파이어 크기가 의존하는 외부 값이 바뀌었을 때 호출되는 훅(현재는 미구현).
 *        실시간으로 다른 속성에 따라 이펙트 최종 수치를 재계산할 필요가 없어 본문을 비워 둔다.
 * @param Handle 영향을 받을 수 있는 활성 이펙트 핸들.
 * @param ChangedAgg 변경된 의존 Aggregator.
 */
void FActiveTacticalEffectsContainer::OnMagnitudeDependencyChange(FActiveTacticalEffectHandle Handle, const FTacticalAggregator* ChangedAgg)
{
    if (Handle.IsValid())
    {
        // NOTE : 
        // 현재 해당 속성을 통해 실시간으로 Effect 최종 수치를 결정하는 요소는 딱히 필요가 없다.
        // 따라서 구현 생략
    }
}

/**
 * @brief 이펙트의 스택 수가 변했을 때 후처리.
 *        스택 수가 실제로 바뀌었으면 모디파이어 크기를 재계산하고(스택 수에 따라 크기가 달라짐),
 *        부여 태그의 스택 카운트 변경을 알리며, OnStackChanged 이벤트를 브로드캐스트한다.
 * @param ActiveEffect 스택 수가 변경된 활성 이펙트.
 * @param OldStackCount 변경 전 스택 수.
 * @param NewStackCount 변경 후 스택 수.
 */
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

/**
 * @brief 모디파이어를 속성의 base 값에 직접 실행(Instant/Execute 계열)한다.
 *        현재 base 값에 연산 종류(ModifierOp)와 크기(ModifierMagnitude)를 적용해 새 base를 구하고 반영한다.
 *
 * @note [PR #191 enum 치환] ModifierOp 타입이 GAS의 EGameplayModOp -> 자체 ETacticalModOp로 바뀐 지점이다.
 *       ETacticalModOp의 정수값은 구 EGameplayModOp와 동일하게 유지된다(직렬화 호환 / Aggregator의 op-인덱싱 /
 *       DefaultEngine.ini CoreRedirect 매핑을 위해). 실제 연산식은 StaticExecModOnBaseValue가 op에 따라 분기한다:
 *         AddBase(0)=합산, MultiplyAdditive(1)=배율 가산, DivideAdditive(2)=나눗셈 가산,
 *         Override(3)=덮어쓰기, MultiplyCompound(4)=거듭제곱 곱, AddFinal(5)=최종 합산.
 * @param Attribute 대상 속성.
 * @param ModifierOp 연산 종류(ETacticalModOp).
 * @param ModifierMagnitude 적용할 모디파이어 크기.
 */
void FActiveTacticalEffectsContainer::ApplyModToAttribute(const FTacticalAttribute& Attribute, TEnumAsByte<ETacticalModOp::Type> ModifierOp, float ModifierMagnitude)
{
    float CurrentBase = GetAttributeBaseValue(Attribute);
    // op별 연산식 적용은 Aggregator 정적 헬퍼에 위임(base 값 직접 변경 경로).
    float NewBase = FTacticalAggregator::StaticExecModOnBaseValue(CurrentBase, ModifierOp, ModifierMagnitude);

    SetAttributeBaseValue(Attribute, NewBase);
}

/**
 * @brief 지속(Duration/Infinite) 이펙트 Spec을 컨테이너에 적용한다.
 *        스태킹 가능한 동일 이펙트가 이미 있으면 스택 수를 합치고, 없으면 새 활성 이펙트를 만든다.
 *        새 이펙트 생성 시, 본 배열에 여유 슬랙이 없으면(재할당 필요) Scope Lock 중 안전을 위해
 *        지연(pending) 연결 리스트에 넣어 Lock 해제 시점에 합쳐지도록 한다.
 * @param Spec 적용할 이펙트 명세.
 * @param[out] FoundExistingStackableGE 기존 스태킹 대상에 합쳐졌으면 true.
 * @return 적용 결과로 활성화된(또는 갱신된) 이펙트 포인터.
 */
FActiveTacticalEffect* FActiveTacticalEffectsContainer::ApplyTacticalEffectSpec(const FTacticalEffectSpec& Spec, bool& FoundExistingStackableGE)
{
	// 적용 과정 동안 배열 변동을 막기 위해 Scope Lock을 건다(추가는 필요 시 지연 처리됨).
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

    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mOwner.Get());
    check(TacticalFrameworkModel != nullptr);

    TacticalFrameworkModel->SetCurrentAppliedTE(&AppliedEffectSpec);
    TacticalFrameworkModel->GlobalPreTacticalEffectSpecApply(AppliedEffectSpec, mOwner.Get());

	// 적용된 Spec의 각 모디파이어 최종 크기를 미리 계산해 둔다.
	AppliedEffectSpec.CalculateModifierMagnitudes();

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

/**
 * @brief Instant(즉발) 이펙트처럼 base 값에 즉시 반영해야 하는 모디파이어들을 실행한다.
 *        Spec의 모든 모디파이어를 평가해 InternalExecuteMod로 하나씩 base 값에 적용한 뒤,
 *        이펙트 클래스의 OnExecuted 훅을 호출한다.
 * @param Spec 실행할 이펙트 명세(모디파이어 크기는 내부에서 재계산됨).
 */
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

        FTacticalModifierEvaluatedData EvalData(ModDef.mAttribute, ModDef.mModifierOp, SpecToUse.GetModifierMagnitude(ModIdx));
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

/**
 * @brief 핸들로 활성 이펙트를 찾아 제거(또는 일부 스택 제거)한다.
 * @param Handle 제거 대상 이펙트 핸들.
 * @param StacksToRemove 제거할 스택 수(0 이하 또는 보유 스택 이상이면 이펙트 전체 제거).
 * @return 일치하는 이펙트를 찾아 제거 처리했으면 true, 못 찾으면 false.
 */
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

/**
 * @brief 특정 속성의 값 변경 알림 델리게이트를 반환한다(없으면 생성).
 * @param Attribute 대상 속성.
 * @return 해당 속성의 값 변경 델리게이트 참조.
 */
FOnChangeAttributeValue& FActiveTacticalEffectsContainer::GetTacticalAttributeValueChangeDelegate(FTacticalAttribute Attribute)
{
    return mAttributeValueChangeDelegates.FindOrAdd(Attribute);
}

/**
 * @brief 활성 이펙트가 새로 추가됐을 때의 내부 후처리.
 *        이펙트 클래스의 OnAddedToActiveContainer 훅을 호출하고, 소유자에 활성 핸들을 등록한다.
 * @param Effect 새로 추가된 활성 이펙트.
 */
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

/**
 * @brief 활성 이펙트가 제거될 때의 내부 후처리.
 *        이펙트를 pendingRemove로 표시하고, 부여했던 태그/모디파이어(Aggregator mod)를 회수한 뒤,
 *        제거 이벤트들을 브로드캐스트한다.
 * @param Effect 제거되는 활성 이펙트.
 * @param TacticalEffectRemovalInfo 제거 사유/스택/컨텍스트 등 제거 정보.
 */
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

/**
 * @brief 평가된 모디파이어 하나를 실제 속성에 실행한다.
 *        대상 속성의 AttributeSet이 UTacticalAttributeSet 계열로 존재할 때만 base 값에 적용한다.
 *
 * @note [PR #191 enum 치환] ModEvalData.mModifierOp는 ETacticalModOp 타입(구 EGameplayModOp 대체)이며,
 *       ApplyModToAttribute로 전달되어 op별 연산식(합산/배율가산/나눗셈가산/덮어쓰기/거듭제곱곱/최종합산)이 적용된다.
 * @param Spec 실행 중인 이펙트 명세(현재 직접 사용하진 않으나 시그니처 유지).
 * @param ModEvalData 적용할 속성/연산종류/크기를 담은 평가 데이터.
 * @return 모디파이어가 실제로 적용됐으면 true.
 */
bool FActiveTacticalEffectsContainer::InternalExecuteMod(FTacticalEffectSpec& Spec, FTacticalModifierEvaluatedData& ModEvalData)
{
    check(mOwner.IsValid() == true);

    bool Executed = false;

    UTacticalAttributeSet* AttributeSet = nullptr;
    UClass* AttributeSetClass = ModEvalData.mAttribute.GetAttributeSetClass();
    // 대상 속성이 Tactical AttributeSet 계열에 속할 때만 적용한다.
    if (AttributeSetClass != nullptr && AttributeSetClass->IsChildOf(UTacticalAttributeSet::StaticClass()) == true)
    {
        AttributeSet = const_cast<UTacticalAttributeSet*>(mOwner->GetAttributeSet_Internal(AttributeSetClass));
    }

    if (AttributeSet != nullptr)
    {
        float OldValueOfProperty = mOwner->GetAttributeCurrentValue(ModEvalData.mAttribute);
        // op 종류에 따른 연산을 base 값에 직접 실행.
        ApplyModToAttribute(ModEvalData.mAttribute, ModEvalData.mModifierOp, ModEvalData.mMagnitude);

        Executed = true;
    }

    return Executed;
}

/**
 * @brief 인덱스로 지정한 활성 이펙트를 제거(또는 일부 스택 제거)하는 내부 구현.
 *        제거할 스택이 보유 스택보다 적으면 스택만 줄이고 끝낸다. 그 외엔 이펙트 전체를 제거하되,
 *        Scope Lock 중이면 즉시 배열에서 빼지 않고 pendingRemove로 표시(지연 제거)한다.
 * @param Idx 제거 대상 이펙트 인덱스.
 * @param StacksToRemove 제거할 스택 수.
 * @param bPrematureRemoval 만료 전 강제 제거 여부.
 * @return 본 배열이 즉시 변경(RemoveAtSwap)됐으면 true, 스택만 줄였거나 지연 제거면 false.
 */
bool FActiveTacticalEffectsContainer::InternalRemoveActiveTacticalEffect(int32 Idx, int32 StacksToRemove, bool bPrematureRemoval)
{
    // Lock이 걸려 있으면 즉시 배열 제거 대신 지연 제거 경로를 탄다.
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

/**
 * @brief 지속 이펙트가 부여하는 모디파이어와 태그를 등록한다(Duration/Infinite 경로).
 *        각 모디파이어를 해당 속성의 Aggregator에 등록(current 값 계산에 반영)하고, 부여 태그 맵을 갱신하며,
 *        추가 델리게이트를 브로드캐스트한다.
 *
 * @note [PR #191 enum 치환] ModInfo.mModifierOp는 ETacticalModOp(구 EGameplayModOp 대체).
 *       Aggregator는 이 op 값으로 mMods[ETacticalModOp::Max] 배열을 인덱싱하므로, op의 정수값이 구 enum과
 *       동일하게 유지되는 것이 전제다. AddAggregatorMod는 (크기, op, 핸들)을 op별 버킷에 쌓아 둔다.
 * @param Effect 태그/모디파이어를 부여할 활성 이펙트.
 */
void FActiveTacticalEffectsContainer::AddActiveTacticalEffectGrantedTagsAndModifiers(FActiveTacticalEffect& Effect)
{
    check(Effect.mSpec.mEffectClass != nullptr);
    check(mOwner != nullptr);

    TACTICAL_EFFECT_SCOPE_LOCK();

    for (int32 ModIdx = 0; ModIdx < Effect.mSpec.mModifierValues.Num(); ++ModIdx)
    {
        if (Effect.mSpec.mEffectClass->mModifiers.IsValidIndex(ModIdx) == false)
        {
            continue;
        }

        const FTacticalModifierInfo& ModInfo = Effect.mSpec.mEffectClass->mModifiers[ModIdx];
        // 소유자가 해당 속성의 AttributeSet을 갖고 있지 않으면 등록을 건너뛴다.
        if (mOwner->HasAttributeSetForAttribute(ModInfo.mAttribute) == false)
        {
            continue;
        }

        float EvaluatedMagnitude = Effect.mSpec.GetModifierMagnitude(ModIdx);
        FTacticalAggregator* Aggregator = FindOrCreateAttributeAggregator(Effect.mSpec.mEffectClass->mModifiers[ModIdx].mAttribute).Get();
        if (ensure(Aggregator))
        {
            // op(ETacticalModOp)별 버킷에 모디파이어를 추가. 이펙트 핸들로 추후 제거 식별이 가능하다.
            Aggregator->AddAggregatorMod(EvaluatedMagnitude, ModInfo.mModifierOp, Effect.mHandle);
        }
    }

    // 부여 태그를 +1 카운트로 태그 맵에 반영하고, 추가 사실을 소유자에게 알린다.
    mOwner->UpdateTagMap(Effect.mSpec.mEffectClass->GetGrantedTags(), 1);
    mOwner->OnActiveTacticalEffectAddedDelegateToSelf.Broadcast(mOwner.Get(), Effect.mSpec, Effect.mHandle);
}

/**
 * @brief 지속 이펙트가 부여했던 모디파이어와 태그를 회수한다(제거 경로).
 *        이펙트 핸들을 키로 각 속성 Aggregator에서 모디파이어를 떼어내고, 부여 태그를 -1 카운트로 반영한다.
 * @param Effect 회수 대상 활성 이펙트.
 */
void FActiveTacticalEffectsContainer::RemoveActiveTacticalEffectGrantedTagsAndModifiers(const FActiveTacticalEffect& Effect)
{
    for (const FTacticalModifierInfo& Mod : Effect.mSpec.mEffectClass->mModifiers)
    {
        if (Mod.mAttribute.IsValid() == true)
        {
            if (const TSharedPtr<FTacticalAggregator>* RefPtr = mAttributeAggregatorMap.Find(Mod.mAttribute))
            {
                // 이 이펙트 핸들로 등록됐던 모디파이어들을 op 버킷에서 제거(추가 시의 핸들과 매칭).
                RefPtr->Get()->RemoveAggregatorMod(Effect.mHandle);
            }
        }
    }
    // 추가 때 +1 했던 부여 태그를 -1로 상쇄한다.
    mOwner->UpdateTagMap(Effect.mSpec.mEffectClass->GetGrantedTags(), -1);
}

/**
 * @brief 속성의 base 값을 설정하고, 그에 따른 current 값을 다시 계산한다.
 *        FTacticalAttributeData에 base를 직접 쓰고, 해당 속성의 Aggregator가 있으면 base를 갱신해
 *        모든 모디파이어가 반영된 current를 산출한다. Aggregator가 없으면 base == current로 본다.
 *        변경 전후로 PreAttributeBaseChange / PostAttributeBaseChange 훅을 호출한다.
 * @param Attribute 대상 속성.
 * @param BaseValue 설정할 새 base 값.
 */
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
    // 속성 데이터는 FTacticalAttributeData(virtual 보유로 vptr이 있어 FGameplayAttributeData와 메모리 레이아웃이 다름).
    // 반드시 FTacticalAttributeData로 접근해야 base 값이 올바른 오프셋에 읽고 쓰인다.
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

/**
 * @brief 속성의 base 값을 조회한다.
 *        FTacticalAttributeData 기반 속성이면 데이터에서 직접 읽고, 아니면 Aggregator의 base 값을 사용한다.
 * @param Attribute 조회할 속성.
 * @return base 값(소유자/데이터가 없으면 0).
 */
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

float FActiveTacticalEffectsContainer::EvaluateAttributeWithAdditionalModifier(
    const FTacticalAttribute& Attribute,
    TEnumAsByte<ETacticalModOp::Type> ModifierOp,
    float ModifierMagnitude) const
{
    if (const TSharedPtr<FTacticalAggregator>* Aggregator = mAttributeAggregatorMap.Find(Attribute))
    {
        if (Aggregator->IsValid())
        {
            return (*Aggregator)->EvaluateWithAdditionalModifier(ModifierOp, ModifierMagnitude);
        }
    }

    // 지속 모디파이어가 하나도 없는 속성은 BaseValue에 단일 연산을 적용한 값과 같다.
    return FTacticalAggregator::StaticExecModOnBaseValue(
        GetAttributeBaseValue(Attribute), ModifierOp, ModifierMagnitude);
}

/**
 * @brief 속성의 current 값을 설정하고, 등록된 변경 델리게이트가 있으면 변경 데이터를 브로드캐스트한다.
 * @param Attribute 대상 속성.
 * @param CurrentValue 설정할 current 값(클램프 등으로 보정될 수 있어 설정 후 다시 읽어 사용).
 */
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

/**
 * @brief 한 이펙트가 건드리는 모든 속성에 대해 Aggregator 모디파이어 크기를 다시 산정한다.
 *        (예: 스택 수 변화로 모디파이어 크기가 바뀌었을 때 호출.)
 * @param ActiveEffect 크기를 재산정할 활성 이펙트.
 */
void FActiveTacticalEffectsContainer::UpdateAllAggregatorModMagnitudes(FActiveTacticalEffect& ActiveEffect)
{
    const FTacticalEffectSpec& Spec = ActiveEffect.mSpec;
    checkf(Spec.mEffectClass != nullptr, TEXT("추가하려는 Effect Class가 보이지 않음"));

    // 이 이펙트의 모든 모디파이어가 영향을 주는 속성 집합을 중복 없이 수집.
    TSet<FTacticalAttribute> AttributesToUpdate;
    for (int32 ModIdx = 0; ModIdx < Spec.mModifierValues.Num(); ++ModIdx)
    {
        const FTacticalModifierInfo& ModDef = Spec.mEffectClass->mModifiers[ModIdx];
        AttributesToUpdate.Add(ModDef.mAttribute);
    }

    // 변경된 Attribute 갱신
    UpdateAggregatorModMagnitudes(AttributesToUpdate, ActiveEffect);
}

/**
 * @brief 지정한 속성 집합에 대해 해당 이펙트가 등록한 Aggregator 모디파이어의 크기를 갱신한다.
 *        소유자가 해당 속성 AttributeSet을 가진 경우에만, 이펙트 핸들로 식별되는 모디파이어 크기를 재설정한다.
 * @param AttributesToUpdate 갱신 대상 속성 집합.
 * @param ActiveEffect 갱신 기준이 되는 활성 이펙트(핸들/스펙 제공).
 */
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

        // 핸들로 식별되는 기존 모디파이어를 현재 Spec 기준 크기로 갱신(op 종류는 유지).
        Aggregator->UpdateAggregatorMod(ActiveEffect.mHandle, Attribute, Spec, ActiveEffect.mHandle);
    }
}

/**
 * @brief 핸들로 활성 이펙트를 찾는다(본 배열 + 지연 리스트를 함께 순회하는 반복자 사용).
 * @param Handle 찾을 이펙트 핸들.
 * @return 일치하는 이펙트 포인터(없으면 nullptr).
 */
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

/**
 * @brief 핸들로 활성 이펙트를 찾는다(const 버전).
 * @param Handle 찾을 이펙트 핸들.
 * @return 일치하는 이펙트의 const 포인터(없으면 nullptr).
 */
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

/**
 * @brief 인덱스로 활성 이펙트를 얻는다. 본 배열 범위를 넘는 인덱스는 지연 리스트로 이어 접근한다.
 *        즉 논리 인덱스 공간은 [본 배열 0..N-1] 다음에 [지연 리스트]가 이어진 형태다.
 * @param Index 0-기반 논리 인덱스.
 * @return 해당 위치의 이펙트 포인터(범위를 벗어나면 nullptr).
 */
FActiveTacticalEffect* FActiveTacticalEffectsContainer::GetActiveTacticalEffect(int32 Index)
{
    if (Index < mTacticalEffects.Num())
    {
        return &mTacticalEffects[Index];
    }

    // 본 배열 범위를 넘으면 지연 리스트에서 상대 인덱스만큼 전진한다.
    Index -= mTacticalEffects.Num();
    FActiveTacticalEffect* Ptr = mPendingTacticalEffectHead;
    FActiveTacticalEffect* Stop = *mPendingTacticalEffectTail;

    while (Index-- > 0 && Ptr != nullptr && Ptr != Stop && Ptr->mPendingNext != Stop)
    {
        Ptr = Ptr->mPendingNext;
    }

    return Index <= 0 ? Ptr : nullptr;
}

/**
 * @brief 인덱스로 활성 이펙트를 얻는다(const 버전). 비-const 구현으로 위임한다.
 * @param Index 0-기반 논리 인덱스.
 * @return 해당 위치의 이펙트 const 포인터(범위를 벗어나면 nullptr).
 */
const FActiveTacticalEffect* FActiveTacticalEffectsContainer::GetActiveTacticalEffect(int32 Index) const
{
    return const_cast<FActiveTacticalEffectsContainer*>(this)->GetActiveTacticalEffect(Index);
}

/**
 * @brief 본 배열과 지연 추가 리스트를 합친 전체 활성 이펙트 개수를 반환한다.
 * @return 활성 이펙트 총 개수(본 배열 + pending).
 */
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

/**
 * @brief 주어진 Spec과 스태킹 가능한 기존 활성 이펙트를 찾는다.
 *        스태킹 정책이 None이 아니고 지속형(Instant 아님)일 때만 탐색한다.
 *        AggregateByTarget이면 같은 이펙트 클래스면 매칭, 그 외(소스 기준)면 소스 ASC 모델까지 일치해야 매칭.
 * @param Spec 새로 적용하려는 이펙트 명세.
 * @return 스태킹 대상 활성 이펙트(없으면 nullptr).
 */
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

