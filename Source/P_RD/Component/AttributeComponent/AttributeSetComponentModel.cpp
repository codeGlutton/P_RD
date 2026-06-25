#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "TAS/Effect/TacticalEffectContext.h"

#include "Actor/ActorModel.h"

void UAttributeSetComponentModel::Initialize()
{
	Super::Initialize();

    for (int32 Idx = mSpawnedAttributes.Num() - 1; Idx >= 0; --Idx)
    {
        if (mSpawnedAttributes[Idx] == nullptr)
        {
            mSpawnedAttributes.RemoveAt(Idx);
        }
    }

    TArray<UObject*> ChildObjects;
    GetObjectsWithOuter(GetOwnerModel(), ChildObjects, false, RF_NoFlags, EInternalObjectFlags::Garbage);

    for (UObject* Obj : ChildObjects)
    {
        UAttributeSet* Set = Cast<UAttributeSet>(Obj);
        if (Set)
        {
            mSpawnedAttributes.AddUnique(Set);
        }
    }

    mActiveAttributeEffects.RegisterWithOwnerModel(this);
    mTacticalTagCountContainer.SetOwner(this);
}

void UAttributeSetComponentModel::Uninitialize()
{
    Super::Uninitialize();
}

void UAttributeSetComponentModel::BeginPlay()
{
	Super::BeginPlay();
}

void UAttributeSetComponentModel::EndPlay()
{
    Super::EndPlay();
}

const UAttributeSet* UAttributeSetComponentModel::GetAttributeSet_Internal(TSubclassOf<UAttributeSet> Class) const
{
    for (const UAttributeSet* Set : mSpawnedAttributes)
    {
        if (Set != nullptr && Set->IsA(Class) == true)
        {
            return Set;
        }
    }
    return nullptr;
}

bool UAttributeSetComponentModel::HasAttributeSetForAttribute(FGameplayAttribute Attribute) const
{
    return (Attribute.IsValid() == true && (GetAttributeSet_Internal(Attribute.GetAttributeSetClass()) != nullptr));
}

void UAttributeSetComponentModel::AddSpawnedAttributeSet(UAttributeSet* AttributeSet)
{
    if (IsValid(AttributeSet) == false)
    {
        return;
    }
    mSpawnedAttributes.AddUnique(AttributeSet);
}

void UAttributeSetComponentModel::RemoveSpawnedAttributeSet(UAttributeSet* AttributeSet)
{
    if (mSpawnedAttributes.RemoveSingle(AttributeSet) == 1)
    {
        /* 모든 클래스 속성 가져오기 */

        TArray<FGameplayAttribute> Attributes;
        UAttributeSet::GetAttributesFromSetClass(AttributeSet->GetClass(), Attributes);
        for (const FGameplayAttribute& Attribute : Attributes)
        {
            UE_LOG(LogAttributeSetComp, Log, TEXT("계산 객체 Aggregator에서 해당 속성 값 [%s] 제거"), *Attribute.GetName());
            mActiveAttributeEffects.CleanupAttributeAggregator(Attribute);
        }
    }
}

const UAttributeSet* UAttributeSetComponentModel::GetOrCreateAttributeSet_Internal(TSubclassOf<UAttributeSet> Class)
{
    UActorModel* OwningActorModel = GetOwnerModel();
    const UAttributeSet* OwnedAttributes = nullptr;
    if (OwningActorModel != nullptr && Class != nullptr)
    {
        /* 기존 속성 객체 찾아보기 */

        OwnedAttributes = GetAttributeSet_Internal(Class);
        if (OwnedAttributes == nullptr)
        {
            /* 발견 못해서 생성 */

            UAttributeSet* Attributes = NewObject<UAttributeSet>(OwningActorModel, Class);
            AddSpawnedAttributeSet(Attributes);
            OwnedAttributes = Attributes;
        }
    }

    return OwnedAttributes;
}

void UAttributeSetComponentModel::SetAttributeBaseValue(const FGameplayAttribute& Attribute, float BaseValue)
{
    mActiveAttributeEffects.SetAttributeBaseValue(Attribute, BaseValue);
}

float UAttributeSetComponentModel::GetAttributeBaseValue(const FGameplayAttribute& Attribute) const
{
    return mActiveAttributeEffects.GetAttributeBaseValue(Attribute);
}

float UAttributeSetComponentModel::GetAttributeCurrentValue(FGameplayAttribute Attribute, bool& Found) const
{
    if (Attribute.IsValid() == true)
    {
        const UAttributeSet* FoundAttributeSet = GetAttributeSet_Internal(Attribute.GetAttributeSetClass());
        if (FoundAttributeSet != nullptr)
        {
            Found = true;
            return Attribute.GetNumericValue(FoundAttributeSet);
        }
    }

    Found = false;
    return 0.0f;
}

float UAttributeSetComponentModel::GetAttributeCurrentValue(const FGameplayAttribute& Attribute) const
{
    const UAttributeSet* FoundAttributeSet = GetAttributeSet_Internal(Attribute.GetAttributeSetClass());
    if (FoundAttributeSet == nullptr)
    {
        return 0.f;
    }
    return Attribute.GetNumericValue(FoundAttributeSet);
}

void UAttributeSetComponentModel::SetAttributeCurrentValue_Internal(const FGameplayAttribute& Attribute, float& NewValue)
{
    const UAttributeSet* AttributeSet = GetAttributeSet_Internal(Attribute.GetAttributeSetClass());
    checkf(AttributeSet != nullptr, TEXT("변경하려는 AttributeSet이 nullptr"));

    Attribute.SetNumericValueChecked(NewValue, const_cast<UAttributeSet*>(AttributeSet));
}

FOnChangeAttributeValue& UAttributeSetComponentModel::GetTacticalAttributeValueChangeDelegate(FGameplayAttribute Attribute)
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

void UAttributeSetComponentModel::ApplyModToAttribute(const FGameplayAttribute& Attribute, TEnumAsByte<EGameplayModOp::Type> ModifierOp, float ModifierMagnitude)
{
    mActiveAttributeEffects.ApplyModToAttribute(Attribute, ModifierOp, ModifierMagnitude);
}

UTacticalEffectContext* UAttributeSetComponentModel::MakeEffectContext() const
{
    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
    check(TacticalFrameworkModel != nullptr);
    UTacticalEffectContext* Context = TacticalFrameworkModel->AllocTacticalEffectContext();

    Context->SetInstigator(GetOwnerModel());

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
	FScopedActiveTacticalEffectLock ScopeLock(mActiveAttributeEffects);
	FScopeCurrentTacticalEffectBeingApplied ScopedGEApplication(GetWorld(), &Spec, this);

	/* 진입 검사 */

	if (Spec.mEffectClass->CanApply(mActiveAttributeEffects, Spec) == false)
	{
		return FActiveTacticalEffectHandle();
	}

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
	TacticalFrameworkModel->SetCurrentAppliedGE(OurCopyOfSpec);

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
    check(Spec.mIsInfinite == false);
    mActiveAttributeEffects.ExecuteActiveEffectsFrom(Spec);
}

bool UAttributeSetComponentModel::RemoveActiveTacticalEffect(FActiveTacticalEffectHandle Handle, int32 StacksToRemove)
{
    return mActiveAttributeEffects.RemoveActiveTacticalEffect(Handle, StacksToRemove);
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

void UAttributeSetComponentModel::OnAttributeAggregatorDirty(FTacticalAggregator* Aggregator, FGameplayAttribute Attribute)
{
    mActiveAttributeEffects.OnAttributeAggregatorDirty(Aggregator, Attribute);
}

void UAttributeSetComponentModel::OnMagnitudeDependencyChange(FActiveTacticalEffectHandle Handle, const FTacticalAggregator* ChangedAggregator)
{
    mActiveAttributeEffects.OnMagnitudeDependencyChange(Handle, ChangedAggregator);
}

FOnTacticalEffectTagCountChanged& UAttributeSetComponentModel::RegisterTacticalTagEvent(FGameplayTag Tag, EGameplayTagEventType::Type EventType)
{
    return mTacticalTagCountContainer.RegisterGameplayTagEvent(Tag, EventType);
}

bool UAttributeSetComponentModel::UnregisterTacticalTagEvent(FDelegateHandle DelegateHandle, FGameplayTag Tag, EGameplayTagEventType::Type EventType)
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
        if (mTacticalTagCountContainer.UpdateTagCount(Tag, CountDelta))
        {
            OnTagUpdated(Tag, true);
        }
    }
    else if (CountDelta < 0)
    {
        TArray<FDeferredTagChangeDelegate> DeferredTagChangeDelegates;
        if (mTacticalTagCountContainer.UpdateTagCount_DeferredParentRemoval(Tag, CountDelta, DeferredTagChangeDelegates))
        {
            mTacticalTagCountContainer.FillParentTags();
            OnTagUpdated(Tag, false);

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
				RemovedTags.Add(Tag);
			}
		}

		if (RemovedTags.Num() > 0)
		{
            mTacticalTagCountContainer.FillParentTags();
		}

		for (FDeferredTagChangeDelegate& Delegate : DeferredTagChangeDelegates)
		{
			Delegate.Execute();
		}

		for (FGameplayTag& Tag : RemovedTags)
		{
			OnTagUpdated(Tag, false);
		}
	}
}

void UAttributeSetComponentModel::OnTagUpdated(const FGameplayTag& Tag, bool TagExists)
{
}
