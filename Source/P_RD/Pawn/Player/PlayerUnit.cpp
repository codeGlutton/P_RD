#include "Pawn/Player/PlayerUnit.h"
#include "GAS/Attribute/UnitAttributeSet.h"
#include "GAS/Attribute/LevelAttributeSet.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

#include "GameMode/RDGameModeBase.h"

#include "Setting/UnitTeamType.h"

APlayerUnit::APlayerUnit()
{
    SetGenericTeamId(EUnitTeamType::Adventurer);
    mUnitAttributeSet = CreateDefaultSubobject<UPlayerUnitAttributeSet>(TEXT("PlayerUnitAttributeSet"));
    mLevelAttributeSet = CreateDefaultSubobject<ULevelAttributeSet>(TEXT("LevelAttributeSet"));
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
