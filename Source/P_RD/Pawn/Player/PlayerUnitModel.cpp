#include "Pawn/Player/PlayerUnitModel.h"
#include "Setting/GameTeamType.h"

#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

#include "Dice/DicePoolModel.h"

/** @brief 플레이어 전용 AttributeSet과 DiceComponent를 서브오브젝트로 생성한다. */
UPlayerUnitModel::UPlayerUnitModel()
{
    SetGenericTeamId(EGameTeamType::Adventurer);
    // mUnitAttributeSet = CreateDefaultSubobject<UPlayerUnitAttributeSet>(TEXT("PlayerUnitAttributeSet"));
    // mLevelAttributeSet = CreateDefaultSubobject<ULevelAttributeSet>(TEXT("LevelAttributeSet"));
    // 보유 주사위의 진짜 런타임 상태는 플레이어 유닛이 소유하고, CombatUIAdapter는 읽어서 UIModel로 변환한다.
    mDicePool = CreateDefaultSubobject<UDicePoolModel>(TEXT("DiceComp"));
}

void UPlayerUnitModel::PostInitializeComponentModels()
{
    Super::PostInitializeComponentModels();

    // Level값에 따라 ASC Attribute 초기화
    // auto* AbilitySystemGlobals = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals();
    // AbilitySystemGlobals->InitGlobalData();
    // AbilitySystemGlobals->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAbilitySystemComponent(), TEXT("PlayerLevel"), GetPlayerLevel(), true);
}

UUserWidget* UPlayerUnitModel::GetInfoPanel() const
{
    return nullptr;
}

EPlayerJobType UPlayerUnitModel::GetPlayerJobType() const
{
    return mJobType;
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

/** @brief 전투 UI/어댑터가 플레이어 보유 주사위 상태를 읽기 위한 접근자다. */
UDicePoolModel* UPlayerUnitModel::GetDicePool() const
{
    return mDicePool;
}