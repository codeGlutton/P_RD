#include "TAS/Effect/ActiveTacticalEffect.h"

#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

FActiveTacticalEffectHandle::FActiveTacticalEffectHandle(int32 Index, UWorld* World) :
    mIndex(Index),
    mWorld(World)
{
}

FActiveTacticalEffectHandle FActiveTacticalEffectHandle::GenerateNewHandle(UWorld* World, UAttributeSetComponentModel* OwningModel)
{
    static int32 MaxHandleIndex = 0;
    FActiveTacticalEffectHandle NewHandle(MaxHandleIndex++, World);

    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(World);
    checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

    TWeakObjectPtr<UAttributeSetComponentModel> WeakPtr(OwningModel);
    TacticalFrameworkModel->mEffectOwningModelMap.Add(NewHandle, WeakPtr);

    return NewHandle;
}

void FActiveTacticalEffectHandle::ResetGlobalHandleMap(UWorld* World)
{
    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(World);
    checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

    TacticalFrameworkModel->mEffectOwningModelMap.Reset();
}

UAttributeSetComponentModel* FActiveTacticalEffectHandle::GetOwningAttributeSetComponentModel() const
{
    if (mWorld == nullptr)
    {
        return nullptr;
    }

    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mWorld.Get());
    if (TacticalFrameworkModel == nullptr)
    {
        return nullptr;
    }

    TWeakObjectPtr<UAttributeSetComponentModel>* Ptr = TacticalFrameworkModel->mEffectOwningModelMap.Find(*this);
    if (Ptr != nullptr)
    {
        return Ptr->Get();
    }
    return nullptr;
}

void FActiveTacticalEffectHandle::RemoveFromGlobalMap()
{
    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mWorld.Get());
    checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

    TacticalFrameworkModel->mEffectOwningModelMap.Remove(*this);
}

FActiveTacticalEffect::FActiveTacticalEffect(FActiveTacticalEffectHandle Handle, const FTacticalEffectSpec& Spec, float CurrentTime) :
    mHandle(Handle),
    mSpec(Spec),
    mStartTime(CurrentTime)
{
}

int32 FActiveTacticalEffect::GetTimeRemaining(int32 WorldTime) const
{
    int32 Duration = GetDuration();
    return (Duration == FTacticalEffectConstants::INFINITE_DURATION ? -1 : Duration - (WorldTime - mStartTime));
}

int32 FActiveTacticalEffect::GetDuration() const
{
    return mSpec.GetDuration();
}

ETacticalEffectDurationUnitType FActiveTacticalEffect::GetDurationUnit() const
{
    return mSpec.mEffectClass->mDurationUnitPolicy;
}

int32 FActiveTacticalEffect::GetEndTime() const
{
    int32 Duration = GetDuration();
    return (Duration == FTacticalEffectConstants::INFINITE_DURATION ? -1.f : Duration + mStartTime);
}
