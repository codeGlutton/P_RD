#include "Pawn/Player/PlayerUnitModel.h"
#include "Actor/Party/PartyModel.h"
#include "Setting/GameTeamType.h"

#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "AttributeSet/UnitAttributeSet.h"

#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"

#include "Component/ArtifactComponent/ArtifactComponentModel.h"

UPlayerUnitModel::UPlayerUnitModel()
{
    SetGenericTeamId(EGameTeamType::Adventurer);
    
    mUnitAttributeSet = CreateDefaultSubobject<UPlayerUnitAttributeSet>(TEXT("PlayerUnitAttributeSet"));

    // 아티펙트 컴포넌트 모델 등록
    mArtifactCompModel = CreateDefaultSubobject<UArtifactComponentModel>(TEXT("ArtifactComponentModel"));
}

int32 UPlayerUnitModel::GetBoardActorLevel() const
{
    return mPlayerLevel;
}

EUnitJobType UPlayerUnitModel::GetUnitJobType() const
{
    UStaticPlayerUnitSpawnData* PlayerUnitSpawnData = Cast<UStaticPlayerUnitSpawnData>(mStaticSpawnData);
    if (PlayerUnitSpawnData == nullptr)
    {
        return EUnitJobType::None;
    }
    return PlayerUnitSpawnData->mJobType;
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

void UPlayerUnitModel::SetOwnerParty(UPartyModel* PartyModel)
{
    mOwnerParty = PartyModel;
    if (mOwnerParty != nullptr)
    {
        UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
        checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

        TacticalFrameworkModel->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAttributeComponentModel(), GetBoardActorKeyName(), GetDifficulty(), true);
    }
}

void UPlayerUnitModel::SetPlayerLevel(int32 PlayerLevel)
{
    mPlayerLevel = PlayerLevel;
    OnChangePlayerLevel.Broadcast(this, mPlayerLevel);
}

UPartyModel* UPlayerUnitModel::GetOwnerParty() const
{
    return mOwnerParty.Get();
}

int32 UPlayerUnitModel::GetPlayerLevel() const
{
    return mPlayerLevel;
}

UArtifactComponentModel* UPlayerUnitModel::GetArtifactComponentModel() const
{
    return mArtifactCompModel;
}
