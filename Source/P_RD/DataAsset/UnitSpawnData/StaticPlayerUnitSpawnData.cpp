#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"

#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "GAS/GASMinimal.h"
#include "GAS/Attribute/UnitAttributeSet.h"

#include "Pawn/Player/PlayerUnit.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"

/**
 * @brief 에디터에서 생성 클래스가 변경되면 플레이어 직업 타입을 동기화한다.
 * @param PropertyChangedEvent 변경된 프로퍼티 정보
 */
void UStaticPlayerUnitSpawnData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UStaticUnitSpawnData, mClass))
    {
        if (mClass.IsNull() == true)
        {
            mJobType = EPlayerJobType::None;
        }
        else
        {
            mJobType = GetDefault<APlayerUnit>(mClass.Get())->GetPlayerJobType();
        }
    }
}

/**
 * @brief 플레이어 유닛 DataAsset의 에디터 유효성을 검사한다.
 * @param Context 유효성 검사 결과 메시지를 누적할 컨텍스트
 * @return 부모 검사 결과와 직업 타입 검사 결과를 합친 Data Validation 결과
 */
EDataValidationResult UStaticPlayerUnitSpawnData::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult SuperResult = Super::IsDataValid(Context);
    EDataValidationResult ThisResult = EDataValidationResult::Valid;

    if (mJobType >= EPlayerJobType::Count)
    {
        Context.AddError(FText::FromString(TEXT("잘못된 직업 타입")));
        ThisResult = EDataValidationResult::Invalid;
    }

    return CombineDataValidationResults(SuperResult, ThisResult);
}
#endif

/**
 * @brief 지정 난이도에서 이 플레이어 유닛의 기본 최대 체력을 조회한다.
 * @param Difficulty 조회할 난이도 index
 * @return AttributeSet 초기화 테이블에 정의된 최대 체력. 유효하지 않은 난이도면 0 반환
 */
float UStaticPlayerUnitSpawnData::GetDefaultMaxHP(int32 Difficulty) const
{
    UAbilitySystemGlobals* AbilitySystemGlobals = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals();
    AbilitySystemGlobals->InitGlobalData();
    auto MaxHPArray = AbilitySystemGlobals->GetAttributeSetInitter()->GetAttributeSetValues(
        UPlayerUnitAttributeSet::StaticClass(), 
        UPlayerUnitAttributeSet::GetMaxHPAttribute().GetUProperty(), 
        GetKeyName()
    );

    if (MaxHPArray.IsValidIndex(Difficulty) == false)
    {
        return 0.f;
    }
    return MaxHPArray[Difficulty];
}

/**
 * @brief 지정 난이도에서 이 플레이어 유닛의 기본 보유 골드를 조회한다.
 * @param Difficulty 조회할 난이도 index
 * @return AttributeSet 초기화 테이블에 정의된 보유 골드. 유효하지 않은 난이도면 0 반환
 */
float UStaticPlayerUnitSpawnData::GetDefaultMoney(int32 Difficulty) const
{
    UAbilitySystemGlobals* AbilitySystemGlobals = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals();
    AbilitySystemGlobals->InitGlobalData();
    auto MoneyArray = AbilitySystemGlobals->GetAttributeSetInitter()->GetAttributeSetValues(
        UPlayerUnitAttributeSet::StaticClass(),
        UPlayerUnitAttributeSet::GetMoneyAttribute().GetUProperty(),
        GetKeyName()
    );

    if (MoneyArray.IsValidIndex(Difficulty) == false)
    {
        return 0.f;
    }
    return MoneyArray[Difficulty];
}
