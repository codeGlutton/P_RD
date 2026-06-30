#include "Pawn/Player/PlayerUnitModel.h"
#include "Setting/GameTeamType.h"

#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkSubsystem.h"

#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"

#include "AttributeSet/LevelAttributeSet.h"
#include "AttributeSet/UnitAttributeSet.h"

#include "Dice/DicePoolModel.h"

UPlayerUnitModel::UPlayerUnitModel()
{
    SetGenericTeamId(EGameTeamType::Adventurer);
    
    mUnitAttributeSet = CreateDefaultSubobject<UPlayerUnitAttributeSet>(TEXT("PlayerUnitAttributeSet"));
    mLevelAttributeSet = CreateDefaultSubobject<ULevelAttributeSet>(TEXT("LevelAttributeSet"));

    mDicePool = CreateDefaultSubobject<UDicePoolModel>(TEXT("DiceComp"));
}

void UPlayerUnitModel::PostInitializeComponentModels()
{
    Super::PostInitializeComponentModels();

    UTacticalFrameworkSubsystem* TacticalFrameworkSubsystem = GetWorld()->GetSubsystem<UTacticalFrameworkSubsystem>();
    checkf(TacticalFrameworkSubsystem != nullptr, TEXT("전략 프레임워크 서브시스템 nullptr"));

    UTacticalFrameworkModel* TacticalFrameworkModel = TacticalFrameworkSubsystem->GetModel<UTacticalFrameworkModel>();
    checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

    TacticalFrameworkModel->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAttributeComponentModel(), TEXT("PlayerLevel"), GetPlayerLevel(), true);
}

EPlayerJobType UPlayerUnitModel::GetPlayerJobType() const
{
    UStaticPlayerUnitSpawnData* PlayerUnitSpawnData = Cast<UStaticPlayerUnitSpawnData>(mStaticSpawnData);
    if (PlayerUnitSpawnData == nullptr)
    {
        return EPlayerJobType::None;
    }
    return PlayerUnitSpawnData->mJobType;
}

int32 UPlayerUnitModel::GetPlayerLevel() const
{
    ARDGameModeBase* RDGameMode = GetWorld()->GetAuthGameMode<ARDGameModeBase>();
    checkf(RDGameMode != nullptr, TEXT("게임 모드 nullptr"));

    return RDGameMode->GetRunPersistData()->GetPlayerLevel();
}

int32 UPlayerUnitModel::GetDifficulty() const
{
    ARDGameModeBase* RDGameMode = GetWorld()->GetAuthGameMode<ARDGameModeBase>();
    checkf(RDGameMode != nullptr, TEXT("게임 모드 nullptr"));

    return RDGameMode->GetRunPersistData()->GetDifficulty();
}

bool UPlayerUnitModel::IsPlayerUnitModel() const
{
    return true;
}

UDicePoolModel* UPlayerUnitModel::GetDicePoolModel() const
{
    return mDicePool;
}
