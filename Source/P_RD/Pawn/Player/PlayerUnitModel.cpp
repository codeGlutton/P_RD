#include "Pawn/Player/PlayerUnitModel.h"
#include "Actor/Party/PartyModel.h"
#include "Setting/GameTeamType.h"

#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"

#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"

#include "AttributeSet/LevelAttributeSet.h"
#include "AttributeSet/UnitAttributeSet.h"

#include "Component/ArtifactComponent/ArtifactComponentModel.h"

UPlayerUnitModel::UPlayerUnitModel()
{
    SetGenericTeamId(EGameTeamType::Adventurer);
    
    mUnitAttributeSet = CreateDefaultSubobject<UPlayerUnitAttributeSet>(TEXT("PlayerUnitAttributeSet"));
    mLevelAttributeSet = CreateDefaultSubobject<ULevelAttributeSet>(TEXT("LevelAttributeSet"));

    // 아티펙트 컴포넌트 모델 등록
    mArtifactCompModel = CreateDefaultSubobject<UArtifactComponentModel>(TEXT("ArtifactComponentModel"));
}

int32 UPlayerUnitModel::GetBoardActorLevel() const
{
    return mPlayerLevel;
}

void UPlayerUnitModel::SetOwnerParty(UPartyModel* PartyModel)
{
    mOwnerParty = PartyModel;

    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
    checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

    TacticalFrameworkModel->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAttributeComponentModel(), ULevelAttributeSet::KeyName, GetPlayerLevel(), true);
}

void UPlayerUnitModel::SetPlayerLevel(int32 PlayerLevel)
{
    mPlayerLevel = PlayerLevel;

    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
    checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

    TacticalFrameworkModel->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAttributeComponentModel(), ULevelAttributeSet::KeyName, GetPlayerLevel(), true);
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
    return mPlayerLevel;
}

int32 UPlayerUnitModel::GetDifficulty() const
{
    if (mOwnerParty.IsValid() == false)
    {
        return INDEX_NONE;
    }
    return mOwnerParty->GetDifficulty();
}

bool UPlayerUnitModel::IsPlayerUnitModel() const
{
    return true;
}

UArtifactComponentModel* UPlayerUnitModel::GetArtifactComponentModel() const
{
    return mArtifactCompModel;
}
