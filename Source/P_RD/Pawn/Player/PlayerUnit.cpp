#include "Pawn/Player/PlayerUnit.h"
#include "GAS/Attribute/LevelAttributeSet.h"

#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"

APlayerUnit::APlayerUnit()
{
    mLevelAttributeSet = CreateDefaultSubobject<ULevelAttributeSet>(TEXT("LevelAttributeSet"));
}

void APlayerUnit::PostInitializeComponents()
{
    Super::PostInitializeComponents();

#ifndef WITH_EDITOR
    // Level값에 따라 ASC Attribute 초기화
    IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals()->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAbilitySystemComponent(), TEXT("PlayerLevel"), GetPlayerLevel(), true);
#endif
}

void APlayerUnit::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

#ifdef WITH_EDITOR
    // Level값에 따라 ASC Attribute 초기화 (에디터 환경 내 Level 변경 테스트를 위해 Construction 위치)
    IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals()->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAbilitySystemComponent(), TEXT("PlayerLevel"), GetPlayerLevel(), true);
#endif
}

int32 APlayerUnit::GetPlayerLevel() const
{
    UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
    checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

    return PersistentDataSubsystem->GetRunPersistData()->GetPlayerLevel();
}

int32 APlayerUnit::GetDifficulty() const
{
    UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
    checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

    return PersistentDataSubsystem->GetRunPersistData()->GetDifficulty();
}
