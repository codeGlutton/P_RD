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

    if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UStaticUnitSpawnData, mUnitClass))
    {
        if (mUnitClass.IsNull() == true)
        {
            mJobType = EPlayerJobType::None;
        }
        else
        {
            mJobType = GetDefault<APlayerUnit>(mUnitClass.Get())->GetPlayerJobType();
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
 *
 * @details
 * 캐릭터 선택 UI는 시작 HP를 표시해야 하지만, UI가 AttributeSet 초기화 테이블이나 GAS 초기화 순서를
 * 직접 알면 안 된다. 그래서 PlayerUnit DataAsset이 자신의 기본 스탯 조회 책임을 갖고, UI는
 * FrontendGameMode가 만들어준 FFrontendCharacterOption 값만 표시한다.
 *
 * 패키징된 APK에서는 AttributeSet initter가 아직 준비되지 않은 시점에 UI가 이 값을 요청할 수 있으므로
 * 조회 전에 AbilitySystemGlobals::InitGlobalData()를 호출해 PM/GAS 쪽 공식 초기화 데이터를 사용할 수 있게 한다.
 *
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
 *
 * @details
 * 이 함수는 캐릭터 선택 UI가 골드 계산식을 직접 복제하지 않게 하기 위한 DataAsset 쪽 조회 API다.
 * 실제 값은 GAS AttributeSet 기본값 테이블에서 오며, UI 브랜치는 그 값을 화면에 전달할 뿐 시작 자원 규칙을
 * 새로 정의하지 않는다.
 *
 * GetDefaultMaxHP()와 마찬가지로 APK 런타임에서 AttributeSet 기본값 테이블을 안정적으로 읽기 위해
 * AbilitySystemGlobals::InitGlobalData() 이후에 값을 조회한다.
 *
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
