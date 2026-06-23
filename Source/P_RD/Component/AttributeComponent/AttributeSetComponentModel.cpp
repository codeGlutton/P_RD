#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "Actor/ActorModel.h"

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

void UAttributeSetComponentModel::OnAttributeAggregatorDirty(FTacticalAggregator* Aggregator, FGameplayAttribute Attribute)
{
    mActiveAttributeEffects.OnAttributeAggregatorDirty(Aggregator, Attribute);
}

void UAttributeSetComponentModel::OnMagnitudeDependencyChange(FActiveTacticalEffectHandle Handle, const FTacticalAggregator* ChangedAggregator)
{
    mActiveAttributeEffects.OnMagnitudeDependencyChange(Handle, ChangedAggregator);
}

