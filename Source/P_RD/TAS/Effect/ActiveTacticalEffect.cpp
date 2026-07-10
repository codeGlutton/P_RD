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
    if (::IsValid(World) == false || ::IsValid(OwningModel) == false)
    {
        return FActiveTacticalEffectHandle();
    }

    static int32 MaxHandleIndex = 0;
    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(World);
    if (TacticalFrameworkModel == nullptr)
    {
        return FActiveTacticalEffectHandle();
    }

    FActiveTacticalEffectHandle NewHandle(MaxHandleIndex++, World);

    TWeakObjectPtr<UAttributeSetComponentModel> WeakPtr(OwningModel);
    TacticalFrameworkModel->mEffectOwningModelMap.Add(NewHandle, WeakPtr);

    return NewHandle;
}

void FActiveTacticalEffectHandle::ResetGlobalHandleMap(UWorld* World)
{
    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(World);
    if (TacticalFrameworkModel != nullptr)
    {
        TacticalFrameworkModel->mEffectOwningModelMap.Reset();
    }
}

UAttributeSetComponentModel* FActiveTacticalEffectHandle::GetOwningAttributeSetComponentModel() const
{
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
    if (TacticalFrameworkModel != nullptr)
    {
        TacticalFrameworkModel->mEffectOwningModelMap.Remove(*this);
    }
}

FActiveTacticalEffect::FActiveTacticalEffect(FActiveTacticalEffectHandle Handle, const FTacticalEffectSpec& Spec) :
    mHandle(Handle),
    mSpec(Spec)
{
}
