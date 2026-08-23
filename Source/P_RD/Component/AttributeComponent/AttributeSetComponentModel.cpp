#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "TAS/Effect/TacticalEffectQuery.h"
#include "TAS/Aggregator/TacticalAggregator.h"

#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

void UAttributeSetComponentModel::Initialize()
{
	Super::Initialize();

    // 뒤에서부터 순회하며 null(소멸/직렬화 누락) 엔트리를 제거 — 역순이라 RemoveAt 인덱스가 안정적
    for (int32 Idx = mSpawnedAttributes.Num() - 1; Idx >= 0; --Idx)
    {
        if (mSpawnedAttributes[Idx] == nullptr)
        {
            mSpawnedAttributes.RemoveAt(Idx);
        }
    }

    // 소유 ActorModel을 Outer로 갖는 자식 UObject 중 AttributeSet을 모두 수집한다(Garbage 제외)
    TArray<UObject*> ChildObjects;
    GetObjectsWithOuter(GetOwnerModel(), ChildObjects, false, RF_NoFlags, EInternalObjectFlags::Garbage);

    for (UObject* Obj : ChildObjects)
    {
        UTacticalAttributeSet* Set = Cast<UTacticalAttributeSet>(Obj);
        if (Set)
        {
            mSpawnedAttributes.AddUnique(Set); // 중복 방지하며 등록
        }
    }

    mActiveAttributeEffects.RegisterWithOwnerModel(this);
    mTacticalTagCountContainer.SetOwner(this);
}

void UAttributeSetComponentModel::Uninitialize()
{
    Super::Uninitialize();
}

void UAttributeSetComponentModel::PostDuplicate(bool DuplicateForPIE)
{
	Super::PostDuplicate(DuplicateForPIE);

	mActiveAttributeEffects.PostDuplicate(DuplicateForPIE);
}

void UAttributeSetComponentModel::BeginPlay()
{
	Super::BeginPlay();
}

void UAttributeSetComponentModel::EndPlay()
{
    Super::EndPlay();
}

const TArray<UTacticalAttributeSet*>& UAttributeSetComponentModel::GetSpawnedAttributes() const
{
    return mSpawnedAttributes;
}

const UTacticalAttributeSet* UAttributeSetComponentModel::GetAttributeSet_Internal(TSubclassOf<UTacticalAttributeSet> Class) const
{
    for (const UTacticalAttributeSet* Set : mSpawnedAttributes)
    {
        if (Set != nullptr && Set->IsA(Class) == true)
        {
            return Set;
        }
    }
    return nullptr;
}

bool UAttributeSetComponentModel::HasAttributeSetForAttribute(FTacticalAttribute Attribute) const
{
    return (Attribute.IsValid() == true && (GetAttributeSet_Internal(Attribute.GetAttributeSetClass()) != nullptr));
}

void UAttributeSetComponentModel::AddSpawnedAttributeSet(UTacticalAttributeSet* AttributeSet)
{
    if (IsValid(AttributeSet) == false)
    {
        return;
    }
    mSpawnedAttributes.AddUnique(AttributeSet);
}

void UAttributeSetComponentModel::RemoveSpawnedAttributeSet(UTacticalAttributeSet* AttributeSet)
{
    if (mSpawnedAttributes.RemoveSingle(AttributeSet) == 1)
    {
        /* 모든 클래스 속성 가져오기 */

        TArray<FTacticalAttribute> Attributes;
        UTacticalAttributeSet::GetAttributesFromSetClass(AttributeSet->GetClass(), Attributes);
        for (const FTacticalAttribute& Attribute : Attributes)
        {
            // 제거되는 Set 클래스가 정의한 모든 속성을 열거해 각각의 Aggregator를 정리

            UE_LOG(LogAttributeSetComp, Log, TEXT("계산 객체 Aggregator에서 해당 속성 값 [%s] 제거"), *Attribute.GetName());
            mActiveAttributeEffects.CleanupAttributeAggregator(Attribute);
        }
    }
}

const UTacticalAttributeSet* UAttributeSetComponentModel::GetOrCreateAttributeSet_Internal(TSubclassOf<UTacticalAttributeSet> Class)
{
    UActorModel* OwningActorModel = GetOwnerModel();
    const UTacticalAttributeSet* OwnedAttributes = nullptr;
    if (OwningActorModel != nullptr && Class != nullptr)
    {
        /* 기존 속성 객체 찾아보기 */

        OwnedAttributes = GetAttributeSet_Internal(Class);
        if (OwnedAttributes == nullptr)
        {
            /* 발견 못해서 생성 */

            UTacticalAttributeSet* Attributes = NewObject<UTacticalAttributeSet>(OwningActorModel, Class);
            AddSpawnedAttributeSet(Attributes);
            OwnedAttributes = Attributes;
        }
    }

    return OwnedAttributes;
}

void UAttributeSetComponentModel::CaptureAllStates(UBoardCombatTargetSnapshotData* Snapshot) const
{
    // 각 AttributeSet의 속성값 캡처
    for (const TObjectPtr<UTacticalAttributeSet>& SpawnedAttribute : mSpawnedAttributes)
    {
        SpawnedAttribute->CaptureAllAttributes(Snapshot); 
    }
    // 이펙트 카운트 캡처
    mActiveAttributeEffects.CaptureAllEffectStacks(Snapshot);
    // 태그 카운트 캡처
    mTacticalTagCountContainer.CaptureAllTags(Snapshot);

    // 타일 위치 캡처
    UBoardActorModel* OwnerActorModel = GetOwnerModel<UBoardActorModel>();
    if (OwnerActorModel != nullptr)
    {
        Snapshot->mTileTransform = OwnerActorModel->GetTileTransform();
    }
}

void UAttributeSetComponentModel::SetAttributeBaseValue(const FTacticalAttribute& Attribute, float BaseValue)
{
    mActiveAttributeEffects.SetAttributeBaseValue(Attribute, BaseValue);
}

float UAttributeSetComponentModel::GetAttributeBaseValue(const FTacticalAttribute& Attribute) const
{
    return mActiveAttributeEffects.GetAttributeBaseValue(Attribute);
}

float UAttributeSetComponentModel::GetAttributeCurrentValue(FTacticalAttribute Attribute, bool& Found) const
{
    if (Attribute.IsValid() == true)
    {
        const UTacticalAttributeSet* FoundAttributeSet = GetAttributeSet_Internal(Attribute.GetAttributeSetClass());
        if (FoundAttributeSet != nullptr)
        {
            Found = true;
            return Attribute.GetNumericValue(FoundAttributeSet);
        }
    }

    Found = false;
    return 0.0f;
}

float UAttributeSetComponentModel::GetAttributeCurrentValue(const FTacticalAttribute& Attribute) const
{
    const UTacticalAttributeSet* FoundAttributeSet = GetAttributeSet_Internal(Attribute.GetAttributeSetClass());
    if (FoundAttributeSet == nullptr)
    {
        return 0.f;
    }
    return Attribute.GetNumericValue(FoundAttributeSet);
}

void UAttributeSetComponentModel::SetAttributeCurrentValue_Internal(const FTacticalAttribute& Attribute, float& NewValue)
{
    const UTacticalAttributeSet* AttributeSet = GetAttributeSet_Internal(Attribute.GetAttributeSetClass());
    checkf(AttributeSet != nullptr, TEXT("변경하려는 AttributeSet이 nullptr"));

    // const 컨테이너에서 얻은 포인터지만 값 변경이 필요하므로 const_cast로 쓰기 허용
    Attribute.SetNumericValueChecked(NewValue, const_cast<UTacticalAttributeSet*>(AttributeSet));
}

FOnChangeAttributeValue& UAttributeSetComponentModel::GetTacticalAttributeValueChangeDelegate(FTacticalAttribute Attribute)
{
    return mActiveAttributeEffects.GetTacticalAttributeValueChangeDelegate(Attribute);
}

void UAttributeSetComponentModel::OnTacticalEffectAppliedToTarget(UAttributeSetComponentModel* Model, const FTacticalEffectSpec& SpecApplied, FActiveTacticalEffectHandle ActiveHandle)
{
    OnTacticalEffectAppliedDelegateToTarget.Broadcast(Model, SpecApplied, ActiveHandle);
}

void UAttributeSetComponentModel::OnTacticalEffectAppliedToSelf(UAttributeSetComponentModel* Model, const FTacticalEffectSpec& SpecApplied, FActiveTacticalEffectHandle ActiveHandle)
{
    OnTacticalEffectAppliedDelegateToSelf.Broadcast(Model, SpecApplied, ActiveHandle);
}

void UAttributeSetComponentModel::ApplyModToAttribute(const FTacticalAttribute& Attribute, TEnumAsByte<ETacticalModOp::Type> ModifierOp, float ModifierMagnitude)
{
    mActiveAttributeEffects.ApplyModToAttribute(Attribute, ModifierOp, ModifierMagnitude);
}

UTacticalEffectContext* UAttributeSetComponentModel::MakeEffectContext() const
{
    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
    check(TacticalFrameworkModel != nullptr);
    UTacticalEffectContext* Context = TacticalFrameworkModel->AllocTacticalEffectContext();

    Context->SetInstigator(GetOwnerModel());
    Context->SetAttributeSetComponentModel(this);

    return Context;
}

TSharedPtr<FTacticalEffectSpec> UAttributeSetComponentModel::MakeOutgoingSpec(TSubclassOf<UTacticalEffect> EffectClass, UTacticalEffectContext* Context) const
{
    if (Context == nullptr)
    {
        Context = MakeEffectContext();
    }

    if (EffectClass != nullptr)
    {
        UTacticalEffect* Effect = EffectClass->GetDefaultObject<UTacticalEffect>();

        TSharedPtr<FTacticalEffectSpec> NewSpec = MakeShared<FTacticalEffectSpec>(Effect, Context);
        return NewSpec;
    }

    return nullptr;
}

FActiveTacticalEffectHandle UAttributeSetComponentModel::ApplyTacticalEffectSpecToTarget(const FTacticalEffectSpec& Spec, UAttributeSetComponentModel* Target)
{
    FActiveTacticalEffectHandle ReturnHandle;
    if (Target != nullptr)
    {
        ReturnHandle = Target->ApplyTacticalEffectSpecToSelf(Spec);
    }
    return ReturnHandle;
}

FActiveTacticalEffectHandle UAttributeSetComponentModel::ApplyTacticalEffectSpecToSelf(const FTacticalEffectSpec& Spec)
{
	// 이펙트 적용 중 활성 이펙트 컨테이너 변경을 방지하는 스코프 락
	FScopedActiveTacticalEffectLock ScopeLock(mActiveAttributeEffects);
	// "현재 적용 중인 이펙트" 컨텍스트를 스코프 동안 설정(중첩 적용 추적용)
	FScopeCurrentTacticalEffectBeingApplied ScopedTEApplication(GetWorld(), &Spec, this);

	/* 진입 검사 */

	// 이펙트 자체의 적용 조건(태그 요구사항 등)을 통과하지 못하면 무효 핸들 반환
	if (Spec.mEffectClass->CanApply(mActiveAttributeEffects, Spec) == false)
	{
		return FActiveTacticalEffectHandle();
	}

	// 모디파이어가 가리키는 속성이 하나라도 무효면 적용 중단
	for (const FTacticalModifierInfo& Mod : Spec.mEffectClass->mModifiers)
	{
		if (Mod.mAttribute.IsValid() == false)
		{
			return FActiveTacticalEffectHandle();
		}
	}

	FActiveTacticalEffectHandle	MyHandle(INDEX_NONE, GetWorld());
	bool FoundExistingStackableGE = false;

	FActiveTacticalEffect* AppliedEffect = nullptr;
	FTacticalEffectSpec* OurCopyOfSpec = nullptr;
	TUniquePtr<FTacticalEffectSpec> StackSpec;
	{
		if (Spec.mEffectClass->mDurationPolicy != ETacticalEffectDurationType::Instant)
		{
			AppliedEffect = mActiveAttributeEffects.ApplyTacticalEffectSpec(Spec, FoundExistingStackableGE);
			if (AppliedEffect == nullptr)
			{
				return FActiveTacticalEffectHandle();
			}

			MyHandle = AppliedEffect->mHandle;
			OurCopyOfSpec = &(AppliedEffect->mSpec);
		}

		if (OurCopyOfSpec == nullptr)
		{
			/* 인스턴스 Effect */

			StackSpec = MakeUnique<FTacticalEffectSpec>(Spec);
			OurCopyOfSpec = StackSpec.Get();

			UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
			check(TacticalFrameworkModel != nullptr);
			TacticalFrameworkModel->GlobalPreTacticalEffectSpecApply(*OurCopyOfSpec, this);
		}
	}

	UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
	check(TacticalFrameworkModel != nullptr);
	TacticalFrameworkModel->SetCurrentAppliedTE(OurCopyOfSpec);

	if (Spec.mEffectClass->mDurationPolicy == ETacticalEffectDurationType::Instant)
	{
        ExecuteTacticalEffect(*OurCopyOfSpec);
	}

	Spec.mEffectClass->OnApplied(mActiveAttributeEffects, *OurCopyOfSpec);

    UAttributeSetComponentModel* InstigatorASCModel = Spec.GetContext()->GetAttributeSetComponentModel();
    OnTacticalEffectAppliedToSelf(InstigatorASCModel, *OurCopyOfSpec, MyHandle);

	if (InstigatorASCModel != nullptr)
	{
        InstigatorASCModel->OnTacticalEffectAppliedToTarget(this, *OurCopyOfSpec, MyHandle);
	}

	return MyHandle;
}

void UAttributeSetComponentModel::ExecuteTacticalEffect(FTacticalEffectSpec& Spec)
{
    checkf(Spec.GetDuration() == FTacticalEffectConstants::NO_DURATION, TEXT("즉발 이펙트만 가능"));
    mActiveAttributeEffects.ExecuteActiveEffectsFrom(Spec);
}

FActiveTacticalEffectHandle UAttributeSetComponentModel::SetActiveTacticalEffect(FActiveTacticalEffectHandle&& ActiveHandle)
{
    FActiveTacticalEffect* ActiveEffect = mActiveAttributeEffects.GetActiveTacticalEffect(ActiveHandle);
    checkf(ActiveEffect != nullptr, TEXT("활성화할 Effect 미존재"));

    // 이펙트 적용 중 활성 이펙트 컨테이너 변경을 방지하는 스코프 락
    FScopedActiveTacticalEffectLock ScopeLockActiveGameplayEffects(mActiveAttributeEffects);
    // dirty 콜백을 배치로 묶어 배칭 처리용 RALL 객체
    FScopedTacticalAggregatorOnDirtyBatch AggregatorOnDirtyBatcher(GetWorld()); 
    mActiveAttributeEffects.AddActiveTacticalEffectGrantedTagsAndModifiers(*ActiveEffect);

    if (ActiveEffect->mIsPendingRemove == true)
    {
        return FActiveTacticalEffectHandle();
    }
    return MoveTemp(ActiveHandle);
}

bool UAttributeSetComponentModel::RemoveActiveTacticalEffect(FActiveTacticalEffectHandle Handle, int32 StacksToRemove)
{
    return mActiveAttributeEffects.RemoveActiveTacticalEffect(Handle, StacksToRemove);
}

void UAttributeSetComponentModel::RemoveActiveTacticalEffectBySourceEffect(TSubclassOf<UTacticalEffect> TacticalEffect, UAttributeSetComponentModel* InstigatorComponentModel, int32 StacksToRemove)
{
    if (TacticalEffect != nullptr)
    {
        FTacticalEffectQuery Query;
        Query.mCustomMatchDelegate.BindLambda([&](const FActiveTacticalEffect& CurEffect)
            {
                bool IsMatches = false;

                if (CurEffect.mSpec.mEffectClass != nullptr && TacticalEffect == CurEffect.mSpec.mEffectClass->GetClass())
                {
                    if (InstigatorComponentModel != nullptr)
                    {
                        IsMatches = (InstigatorComponentModel == CurEffect.mSpec.GetContext()->GetAttributeSetComponentModel());
                    }
                    else
                    {
                        IsMatches = true;
                    }
                }

                return IsMatches;
            });
        mActiveAttributeEffects.RemoveActiveEffects(Query, StacksToRemove);
    }
}

int32 UAttributeSetComponentModel::RemoveActiveEffects(const FTacticalEffectQuery& Query, int32 StacksToRemove)
{
    return mActiveAttributeEffects.RemoveActiveEffects(Query, StacksToRemove);
}

int32 UAttributeSetComponentModel::RemoveActiveEffectsWithTags(FGameplayTagContainer Tags)
{
    return RemoveActiveEffects(FTacticalEffectQuery::MakeQuery_MatchAnyEffectTags(Tags));
}

int32 UAttributeSetComponentModel::RemoveActiveEffectsWithAppliedTags(FGameplayTagContainer Tags)
{
    return RemoveActiveEffects(FTacticalEffectQuery::MakeQuery_MatchAnyOwningTags(Tags));
}

const UTacticalEffect* UAttributeSetComponentModel::GetTacticalEffectDefForHandle(FActiveTacticalEffectHandle Handle)
{
    FActiveTacticalEffect* ActiveEffect = mActiveAttributeEffects.GetActiveTacticalEffect(Handle);
    if (ActiveEffect != nullptr)
    {
        return ActiveEffect->mSpec.mEffectClass;
    }
    return nullptr;
}

const FActiveTacticalEffect* UAttributeSetComponentModel::GetActiveTacticalEffect(const FActiveTacticalEffectHandle Handle) const
{
    return mActiveAttributeEffects.GetActiveTacticalEffect(Handle);
}

TArray<FActiveTacticalEffectHandle> UAttributeSetComponentModel::GetActiveEffects(const FTacticalEffectQuery& Query) const
{
    return mActiveAttributeEffects.GetActiveEffects(Query);
}

TArray<FActiveTacticalEffectHandle> UAttributeSetComponentModel::GetActiveEffectsWithAllTags(FGameplayTagContainer Tags) const
{
    return GetActiveEffects(FTacticalEffectQuery::MakeQuery_MatchAllEffectTags(Tags));
}

TMap<FGameplayTag, int32> UAttributeSetComponentModel::GetActiveEffectTagCountsWithAllTags(FGameplayTagContainer Tags) const
{
    TMap<FGameplayTag, int32> EffectAssetTagCounts;

    const TArray<FActiveTacticalEffectHandle> Handles = GetActiveEffectsWithAllTags(Tags);
    for (const FActiveTacticalEffectHandle& Handle : Handles)
    {
        const FActiveTacticalEffect* ActiveEffect = GetActiveTacticalEffect(Handle);
        if (ActiveEffect == nullptr)
        {
            continue;
        }

        for (const FGameplayTag& AssetTag : ActiveEffect->mSpec.mEffectClass->GetAssetTags())
        {
            EffectAssetTagCounts.FindOrAdd(AssetTag) += ActiveEffect->mSpec.GetStackCount();
        }
    }

    return EffectAssetTagCounts;
}

float UAttributeSetComponentModel::GetTacticalEffectMagnitude(FActiveTacticalEffectHandle Handle, const FTacticalAttribute& Attribute) const
{
    return mActiveAttributeEffects.GetTacticalEffectMagnitude(Handle, Attribute);
}

int32 UAttributeSetComponentModel::GetCurrentStackCount(FActiveTacticalEffectHandle Handle) const
{
    if (const FActiveTacticalEffect* ActiveTE = mActiveAttributeEffects.GetActiveTacticalEffect(Handle))
    {
        return ActiveTE->mSpec.GetStackCount();
    }
    return 0;
}

int32 UAttributeSetComponentModel::GetAggregatedStackCount(const FTacticalEffectQuery& Query) const
{
    return mActiveAttributeEffects.GetActiveEffectCount(Query);
}

void UAttributeSetComponentModel::CheckDurationExpired(const int32 Time, ETacticalEffectDurationUnitType UnitType)
{
    mActiveAttributeEffects.CheckDurationExpired(Time, UnitType);
}

void UAttributeSetComponentModel::OnTacticalEffectDurationChange(FActiveTacticalEffect& ActiveEffect)
{
}

int32 UAttributeSetComponentModel::GetActiveEffectsTimeRemaining(const FActiveTacticalEffectHandle Handle) const
{
    const FActiveTacticalEffect* ActiveEffect = GetActiveTacticalEffect(Handle);
    const int32 WorldTime = mActiveAttributeEffects.GetWorldTime(ActiveEffect->GetDurationUnit());
    return ActiveEffect->GetTimeRemaining(WorldTime);
}

TArray<float> UAttributeSetComponentModel::GetActiveEffectsTimeRemaining(const FTacticalEffectQuery& Query, ETacticalEffectDurationUnitType UnitType) const
{
    return mActiveAttributeEffects.GetActiveEffectsTimeRemaining(Query, UnitType);
}

int32 UAttributeSetComponentModel::GetActiveEffectsDuration(const FActiveTacticalEffectHandle Handle) const
{
    const FActiveTacticalEffect* ActiveEffect = GetActiveTacticalEffect(Handle);
    return ActiveEffect->GetDuration();
}

TArray<float> UAttributeSetComponentModel::GetActiveEffectsDuration(const FTacticalEffectQuery& Query, ETacticalEffectDurationUnitType UnitType) const
{
    return mActiveAttributeEffects.GetActiveEffectsTimeRemaining(Query, UnitType);
}

TArray<TPair<float, float>> UAttributeSetComponentModel::GetActiveEffectsTimeRemainingAndDuration(const FTacticalEffectQuery& Query, ETacticalEffectDurationUnitType UnitType) const
{
    return mActiveAttributeEffects.GetActiveEffectsTimeRemainingAndDuration(Query, UnitType);
}

void UAttributeSetComponentModel::OnAttributeAggregatorDirty(FTacticalAggregator* Aggregator, FTacticalAttribute Attribute)
{
    mActiveAttributeEffects.OnAttributeAggregatorDirty(Aggregator, Attribute);
}

void UAttributeSetComponentModel::OnMagnitudeDependencyChange(FActiveTacticalEffectHandle Handle, const FTacticalAggregator* ChangedAggregator)
{
    mActiveAttributeEffects.OnMagnitudeDependencyChange(Handle, ChangedAggregator);
}

void UAttributeSetComponentModel::RemoveLooseGameplayTagsMatchingTag(const FGameplayTag& GameplayTag, int32 Count)
{
    // 모든 LooseGameplayTag 가져오기
    const FGameplayTagContainer& OwnedTags = GetOwnedGameplayTags();

    // 부모 태그 하위의 자식 태그들만 필터링
    FGameplayTagContainer SubTags = OwnedTags.Filter(FGameplayTagContainer(GameplayTag));

    // 필터링된 하위 태그들을 각각 원하는 스택 크기로 감소
    if (SubTags.IsEmpty() == false)
    {
        RemoveLooseGameplayTags(SubTags, Count);
    }
}

FOnTacticalEffectTagCountChanged& UAttributeSetComponentModel::RegisterTacticalTagEvent(FGameplayTag Tag, ETacticalTagEventType::Type EventType)
{
    return mTacticalTagCountContainer.RegisterGameplayTagEvent(Tag, EventType);
}

bool UAttributeSetComponentModel::UnregisterTacticalTagEvent(FDelegateHandle DelegateHandle, FGameplayTag Tag, ETacticalTagEventType::Type EventType)
{
    return mTacticalTagCountContainer.RegisterGameplayTagEvent(Tag, EventType).Remove(DelegateHandle);
}

void UAttributeSetComponentModel::NotifyTagMap_StackCountChange(const FGameplayTagContainer& Container)
{
    for (TArray<FGameplayTag>::TConstIterator TagIt = Container.CreateConstIterator(); TagIt; ++TagIt)
    {
        const FGameplayTag& Tag = *TagIt;
        mTacticalTagCountContainer.Notify_StackCountChange(Tag);
    }
}

void UAttributeSetComponentModel::UpdateTagMapSingle_Internal(const FGameplayTag& Tag, int32 CountDelta)
{
    if (CountDelta > 0)
    {
        // 카운트 증가: 0→양수로 새로 존재하게 되면 true 반환 → 추가 통지
        if (mTacticalTagCountContainer.UpdateTagCount(Tag, CountDelta))
        {
            OnTagUpdated(Tag, true);
        }
    }
    else if (CountDelta < 0)
    {
        // 카운트 감소: 부모 태그 제거를 지연 처리해 순회 중 변경을 피한다
        TArray<FDeferredTagChangeDelegate> DeferredTagChangeDelegates;
        if (mTacticalTagCountContainer.UpdateTagCount_DeferredParentRemoval(Tag, CountDelta, DeferredTagChangeDelegates))
        {
            // 부모 태그 계층 재구성
            mTacticalTagCountContainer.FillParentTags(); 
            OnTagUpdated(Tag, false);

            // 지연해 둔 부모 태그 변화 델리게이트들을 안전한 시점에 실행
            for (FDeferredTagChangeDelegate& Delegate : DeferredTagChangeDelegates)
            {
                Delegate.Execute();
            }
        }
    }
}

void UAttributeSetComponentModel::UpdateTagMap_Internal(const FGameplayTagContainer& Container, int32 CountDelta)
{
	if (CountDelta > 0)
	{
		for (TArray<FGameplayTag>::TConstIterator TagIt = Container.CreateConstIterator(); TagIt; ++TagIt)
		{
			const FGameplayTag& Tag = *TagIt;
			if (mTacticalTagCountContainer.UpdateTagCount(Tag, CountDelta))
			{
				OnTagUpdated(Tag, true);
			}
		}
	}
	else if (CountDelta < 0)
	{
		TArray<FGameplayTag> RemovedTags;
		RemovedTags.Reserve(Container.Num());
		TArray<FDeferredTagChangeDelegate> DeferredTagChangeDelegates;

		for (TArray<FGameplayTag>::TConstIterator TagIt = Container.CreateConstIterator(); TagIt; ++TagIt)
		{
			const FGameplayTag& Tag = *TagIt;
			if (mTacticalTagCountContainer.UpdateTagCount_DeferredParentRemoval(Tag, CountDelta, DeferredTagChangeDelegates))
			{
                // 0으로 떨어진 태그만 수집
				RemovedTags.Add(Tag);
			}
		}

		if (RemovedTags.Num() > 0)
		{
            // 부모 태그 계층을 한 번에 재구성
            mTacticalTagCountContainer.FillParentTags();
		}

		// 지연된 부모 태그 변화 델리게이트 실행
		for (FDeferredTagChangeDelegate& Delegate : DeferredTagChangeDelegates)
		{
			Delegate.Execute();
		}

		// 사라진 태그들에 대한 제거 통지(계층 재구성 이후라 일관된 상태에서 통지됨)
		for (FGameplayTag& Tag : RemovedTags)
		{
			OnTagUpdated(Tag, false);
		}
	}
}

void UAttributeSetComponentModel::OnTagUpdated(const FGameplayTag& Tag, bool TagExists)
{
}
