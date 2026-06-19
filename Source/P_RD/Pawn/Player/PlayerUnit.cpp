#include "Pawn/Player/PlayerUnit.h"
#include "GAS/Attribute/UnitAttributeSet.h"
#include "GAS/Attribute/LevelAttributeSet.h"
#include "Dice/DicePoolModel.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

#include "GameMode/RDGameModeBase.h"

#include "Setting/UnitTeamType.h"

/** @brief 플레이어 전용 AttributeSet과 DiceComponent를 서브오브젝트로 생성한다. */
APlayerUnit::APlayerUnit()
{
    SetGenericTeamId(EUnitTeamType::Adventurer);
    mUnitAttributeSet = CreateDefaultSubobject<UPlayerUnitAttributeSet>(TEXT("PlayerUnitAttributeSet"));
    mLevelAttributeSet = CreateDefaultSubobject<ULevelAttributeSet>(TEXT("LevelAttributeSet"));
    // 보유 주사위의 진짜 런타임 상태는 플레이어 유닛이 소유하고, CombatUIAdapter는 읽어서 UIModel로 변환한다.
    mDicePool = CreateDefaultSubobject<UDicePoolModel>(TEXT("DiceComp"));
}

/** @brief 전투 UI/어댑터가 플레이어 보유 주사위 상태를 읽기 위한 접근자다. */
UDicePoolModel* APlayerUnit::GetDicePool() const
{
    return mDicePool;
}

void APlayerUnit::PostInitializeComponents()
{
    Super::PostInitializeComponents();

#ifndef WITH_EDITOR
    // Level값에 따라 ASC Attribute 초기화
    auto* AbilitySystemGlobals = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals();
    AbilitySystemGlobals->InitGlobalData();
    AbilitySystemGlobals->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAbilitySystemComponent(), TEXT("PlayerLevel"), GetPlayerLevel(), true);
#endif
}

void APlayerUnit::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

#ifdef WITH_EDITOR
    // Level값에 따라 ASC Attribute 초기화 (에디터 환경 내 Level 변경 테스트를 위해 Construction 위치)
    auto* AbilitySystemGlobals = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals();
    AbilitySystemGlobals->InitGlobalData();
    AbilitySystemGlobals->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAbilitySystemComponent(), TEXT("PlayerLevel"), GetPlayerLevel(), true);
#endif
}

UUserWidget* APlayerUnit::GetInfoPanel() const
{
    return nullptr;
}

EPlayerJobType APlayerUnit::GetPlayerJobType() const
{
    return mJobType;
}

int32 APlayerUnit::GetPlayerLevel() const
{
    ARDGameModeBase* RDGameMode = GetWorld()->GetAuthGameMode<ARDGameModeBase>();
    checkf(RDGameMode != nullptr, TEXT("게임 모드 nullptr"));

    return RDGameMode->GetRunPersistData()->GetPlayerLevel();
}

int32 APlayerUnit::GetDifficulty() const
{
    ARDGameModeBase* RDGameMode = GetWorld()->GetAuthGameMode<ARDGameModeBase>();
    checkf(RDGameMode != nullptr, TEXT("게임 모드 nullptr"));

    return RDGameMode->GetRunPersistData()->GetDifficulty();
}

bool APlayerUnit::IsPlayerUnit() const
{
    return true;
}
