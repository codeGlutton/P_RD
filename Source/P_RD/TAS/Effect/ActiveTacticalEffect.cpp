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
    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mWorld.Get());
    checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

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

