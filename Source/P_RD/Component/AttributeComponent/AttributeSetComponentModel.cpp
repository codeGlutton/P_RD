#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "Actor/ActorModel.h"

DEFINE_LOG_CATEGORY(LogAttributeSetComp)

FActiveTacticalEffectHandle::FActiveTacticalEffectHandle(int32 Index, UAttributeSetComponentModel* OwningModel) : mIndex(Index), mOwningModel(OwningModel)
{
}

FActiveTacticalEffectHandle FActiveTacticalEffectHandle::GenerateNewHandle(UAttributeSetComponentModel* OwningModel)
{
    static int32 MaxHandleIndex = 0;
    FActiveTacticalEffectHandle NewHandle(MaxHandleIndex++, OwningModel);

    return NewHandle;
}

UAttributeSetComponentModel* FActiveTacticalEffectHandle::GetOwningAttributeSetComponentModel() const
{
    return mOwningModel.Get();
}

float FTacticalAggregator::GetAttributeBaseValue() const
{
    return mBaseValue;
}

void FTacticalAggregator::SetAttributeBaseValue(float BaseValue, bool BroadcastDirtyEvent = true)
{
    mBaseValue = BaseValue;
    if (BroadcastDirtyEvent == true)
    {
        BroadcastOnDirty();
    }
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
    DependentEffectsCopy.Empty();

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

    const float NewCurrentValue = Aggregator->Evaluate(EvaluationParameters);
    const float OldCurrentValue = mOwner->GetAttributeCurrentValue(Attribute);
    UE_LOG(LogAttributeSetComp, Log, TEXT("현재 값 변경 %.2f -> %.2f"), *Attribute.GetName(), OldCurrentValue, NewCurrentValue);

    InternalUpdateNumericalAttribute(Attribute, NewValue, nullptr, bFromRecursiveCall);
}

void FActiveTacticalEffectsContainer::OnMagnitudeDependencyChange(FActiveTacticalEffectHandle Handle, const FTacticalAggregator* ChangedAgg)
{

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

void UAttributeSetComponentModel::OnAttributeAggregatorDirty(FTacticalAggregator* Aggregator, FGameplayAttribute Attribute, bool FromRecursiveCall)
{
    mActiveAttributeEffects.OnAttributeAggregatorDirty(Aggregator, Attribute);
}

void UAttributeSetComponentModel::OnMagnitudeDependencyChange(FActiveTacticalEffectHandle Handle, const FTacticalAggregator* ChangedAggregator)
{
    mActiveAttributeEffects.OnMagnitudeDependencyChange(Handle, ChangedAggregator);
}

