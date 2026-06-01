#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"

#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "GAS/GASMinimal.h"
#include "GAS/Attribute/UnitAttributeSet.h"

#include "Pawn/Player/PlayerUnit.h"

#if WITH_EDITOR
void UStaticPlayerUnitSpawnData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UStaticUnitSpawnData, mClass))
    {
        mJobType = GetDefault<APlayerUnit>(mClass.Get())->GetPlayerJobType();
    }
}
#endif

float UStaticPlayerUnitSpawnData::GetDefaultMaxHP(int32 Difficulty) const
{
    auto MaxHPArray = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals()->GetAttributeSetInitter()->GetAttributeSetValues(
        UPlayerUnitAttributeSet::StaticClass(), 
        UPlayerUnitAttributeSet::GetMaxHPAttribute().GetUProperty(), 
        GetDefault<AUnit>(mClass.Get())->GetUnitName()
    );
    return MaxHPArray[Difficulty];
}

float UStaticPlayerUnitSpawnData::GetDefaultMoney(int32 Difficulty) const
{
    auto MoneyArray = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals()->GetAttributeSetInitter()->GetAttributeSetValues(
        UPlayerUnitAttributeSet::StaticClass(),
        UPlayerUnitAttributeSet::GetMoneyAttribute().GetUProperty(),
        GetDefault<AUnit>(mClass.Get())->GetUnitName()
    );
    return MoneyArray[Difficulty];
}
