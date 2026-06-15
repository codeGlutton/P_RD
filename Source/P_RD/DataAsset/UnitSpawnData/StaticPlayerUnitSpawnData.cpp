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
 *
 * @details
 * 캐릭터 선택 UI는 시작 HP를 표시해야 하지만, UI가 AttributeSet 초기화 테이블이나 GAS 초기화 순서를
 * 직접 알면 안 된다. 그래서 PlayerUnit DataAsset이 자신의 기본 스탯 조회 책임을 갖고, UI는
 * FrontendGameMode가 만들어준 FFrontendCharacterOption 값만 표시한다.
 *
 * HP/MaxHP는 플레이어 전용 자원이 아니라 모든 유닛이 공유하는 전투 스탯이다.
 * 따라서 PlayerUnitAttributeSet이 아니라 실제 유닛이 보유한 UnitAttributeSet 기준으로 CurveTable 값을 조회한다.
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
        UUnitAttributeSet::StaticClass(),
        UUnitAttributeSet::GetMaxHPAttribute().GetUProperty(),
        GetKeyName()
    );

    if (MaxHPArray.IsValidIndex(Difficulty) == false)
    {
        return 0.f;
    }
    return MaxHPArray[Difficulty];
}

/**
 * @brief 이 플레이어 유닛으로 새 런을 시작할 때의 초기 골드를 조회한다.
 *
 * @details
 * 골드는 HP처럼 전투 유닛이 공통으로 갖는 기본 전투 스탯이 아니라, 새 런을 시작할 때 플레이어에게
 * 지급할 초기 자원에 가깝다. 그래서 Character Select 미리보기에서는 GAS CurveTable을 읽지 않고,
 * DA_TestPlayerUnit 같은 PlayerUnit DataAsset에 저장된 시작 골드 값을 그대로 보여준다.
 *
 * Difficulty 파라미터는 기존 호출부와 API 형태를 맞추기 위해 유지한다. 난이도별 시작 골드가 필요해지면
 * 이 함수 안에서 DataAsset 값을 난이도별 구조로 확장하면 된다.
 *
 * @param Difficulty 현재는 사용하지 않는 난이도 index
 * @return PlayerUnit DataAsset에 정의된 새 런 시작 골드
 */
float UStaticPlayerUnitSpawnData::GetDefaultMoney(int32 /*Difficulty*/) const
{
    return StaticCast<float>(mStartingGold);
}
