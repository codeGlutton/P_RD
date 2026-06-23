#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "Actor/ActorModel.h"

DEFINE_LOG_CATEGORY(LogAttributeSetComp)

FActiveAttributeEffectHandle::FActiveAttributeEffectHandle(int32 Index, UAttributeSetComponentModel* OwningModel) : mIndex(Index), mOwningModel(OwningModel)
{
}

FActiveAttributeEffectHandle FActiveAttributeEffectHandle::GenerateNewHandle(UAttributeSetComponentModel* OwningModel)
{
    static int32 MaxHandleIndex = 0;
    FActiveAttributeEffectHandle NewHandle(MaxHandleIndex++, OwningModel);

    return NewHandle;
}

float FAttributeAggregator::GetAttributeBaseValue() const
{
    return mBaseValue;
}

void FAttributeAggregator::SetAttributeBaseValue(float BaseValue, bool BroadcastDirtyEvent = true)
{
    mBaseValue = BaseValue;
    if (BroadcastDirtyEvent == true)
    {
        BroadcastOnDirty();
    }
}

void FAttributeAggregator::BroadcastOnDirty()
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

    TArray<FActiveGameplayEffectHandle> DependantsLocalCopy = Dependents;
    Dependents.Empty(); // We will add valid handles back as we process the local list

    for (FActiveGameplayEffectHandle Handle : DependantsLocalCopy)
    {
        UAbilitySystemComponent* ASC = Handle.GetOwningAbilitySystemComponent();
        if (ASC)
        {
            ASC->OnMagnitudeDependencyChange(Handle, this);
            Dependents.Add(Handle);
        }
    }

    mDirtyCount--;
}

void FActiveAttributeEffectsContainer::CleanupAttributeAggregator(const FGameplayAttribute& Attribute)
{
    TSharedPtr<FAttributeAggregator>* AggregatorPtr = mAttributeAggregatorMap.Find(Attribute);
    if (AggregatorPtr != nullptr)
    {
        (*AggregatorPtr)->OnDirty.RemoveAll(Owner);
        (*AggregatorPtr)->OnDirtyRecursive.RemoveAll(Owner);

        mAttributeAggregatorMap.Remove(Attribute);
    }
}

void UAttributeSetComponentModel::Initialize()
{
	Super::Initialize();
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

void UAttributeSetComponentModel::AddSpawnedAttribute(UAttributeSet* Attribute)
{
    if (IsValid(Attribute) == false)
    {
        return;
    }
    mSpawnedAttributes.AddUnique(Attribute);
}

void UAttributeSetComponentModel::RemoveSpawnedAttribute(UAttributeSet* Attribute)
{
    if (mSpawnedAttributes.RemoveSingle(Attribute) == 1)
    {
        /* 모든 클래스 속성 가져오기 */

        TArray<FGameplayAttribute> Attributes;
        UAttributeSet::GetAttributesFromSetClass(Attribute->GetClass(), Attributes);
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
            AddSpawnedAttribute(Attributes);
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

float UAttributeSetComponentModel::GetAttributeCurrentValue(const FGameplayAttribute& Attribute) const
{
    const UAttributeSet* FoundAttributeSet = GetAttributeSet_Internal(Attribute.GetAttributeSetClass());
    if (FoundAttributeSet == nullptr)
    {
        return 0.f;
    }
    return Attribute.GetNumericValue(FoundAttributeSet);
}

