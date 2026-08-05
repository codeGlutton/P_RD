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

void UPlayerUnitModel::PostInitializeComponentModels()
{
    Super::PostInitializeComponentModels();

    /* 죽음으로 인한 파티 해제 처리 */

    GetAttributeComponentModel()->RegisterTacticalTagEvent(EffectTags::GameplayEffect_ActorState_Dead, ETacticalTagEventType::NewOrRemoved).AddWeakLambda(
        this, [this](const FGameplayTag Tag, int32 Count) {
            if (Count <= 0)
            {
                return;
            }

            UPartyModel* OwnerParty = mOwnerParty.Get();
            if (OwnerParty != nullptr)
            {
                OwnerParty->RemovePlayerUnitModel(this);
                mOwnerParty = nullptr;
            }
        }
    );
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

    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
    checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

    TacticalFrameworkModel->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAttributeComponentModel(), GetBoardActorKeyName(), GetDifficulty(), true);
}

void UPlayerUnitModel::SetPlayerLevel(int32 PlayerLevel)
{
    mPlayerLevel = PlayerLevel;

    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
    checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

    TacticalFrameworkModel->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAttributeComponentModel(), ULevelAttributeSet::KeyName, GetPlayerLevel(), true);
}

int32 UPlayerUnitModel::GetPlayerLevel() const
{
    return mPlayerLevel;
}

UArtifactComponentModel* UPlayerUnitModel::GetArtifactComponentModel() const
{
    return mArtifactCompModel;
}
