#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"

#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "GAS/GASMinimal.h"
#include "GAS/Attribute/UnitAttributeSet.h"

#include "Pawn/Player/PlayerUnit.h"

#include "Misc/DataValidation.h"

#if WITH_EDITOR
void UStaticPlayerUnitSpawnData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UStaticUnitSpawnData, mClass))
    {
        if (mClass == nullptr)
        {
            mJobType = EPlayerJobType::None;
        }
        else
        {
            mJobType = GetDefault<APlayerUnit>(mClass.Get())->GetPlayerJobType();
        }
    }
}

EDataValidationResult UStaticPlayerUnitSpawnData::IsDataValid(FDataValidationContext& Context) const
{
    Super::IsDataValid(Context);

    if (mJobType >= EPlayerJobType::Count)
    {
        Context.AddError(FText::FromString(TEXT("잘못된 직업 타입")));
        return EDataValidationResult::Invalid;
    }
    return EDataValidationResult::Valid;
}
#endif

float UStaticPlayerUnitSpawnData::GetDefaultMaxHP(int32 Difficulty) const
{
    auto MaxHPArray = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals()->GetAttributeSetInitter()->GetAttributeSetValues(
        UPlayerUnitAttributeSet::StaticClass(), 
        UPlayerUnitAttributeSet::GetMaxHPAttribute().GetUProperty(), 
        GetDefault<AUnit>(mClass.Get())->GetUnitName()
    );

    if (MaxHPArray.IsValidIndex(Difficulty) == false)
    {
        return 0.f;
    }
    return MaxHPArray[Difficulty];
}

float UStaticPlayerUnitSpawnData::GetDefaultMoney(int32 Difficulty) const
{
    auto MoneyArray = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals()->GetAttributeSetInitter()->GetAttributeSetValues(
        UPlayerUnitAttributeSet::StaticClass(),
        UPlayerUnitAttributeSet::GetMoneyAttribute().GetUProperty(),
        GetDefault<AUnit>(mClass.Get())->GetUnitName()
    );

    if (MoneyArray.IsValidIndex(Difficulty) == false)
    {
        return 0.f;
    }
    return MoneyArray[Difficulty];
}
