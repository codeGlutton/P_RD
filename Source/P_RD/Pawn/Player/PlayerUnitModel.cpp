#include "Pawn/Player/PlayerUnitModel.h"
#include "Actor/Party/PartyModel.h"
#include "Setting/GameTeamType.h"

#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "AttributeSet/LevelAttributeSet.h"

#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"

#include "Component/ArtifactComponent/ArtifactComponentModel.h"

UPlayerUnitModel::UPlayerUnitModel()
{
    SetGenericTeamId(EGameTeamType::Adventurer);
    
    mUnitAttributeSet = CreateDefaultSubobject<UPlayerUnitAttributeSet>(TEXT("PlayerUnitAttributeSet"));
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
	int32 PrePlayerLevel = mPlayerLevel;
	mPlayerLevel = PlayerLevel;

	const bool IsLevelChanged = mPlayerLevel != PrePlayerLevel;
	if (IsLevelChanged == true)
	{
		OnChangePlayerLevel.Broadcast(this, mPlayerLevel);
	}
}

UPartyModel* UPlayerUnitModel::GetOwnerParty() const
{
    return mOwnerParty.Get();
}

int32 UPlayerUnitModel::GetPlayerLevel() const
{
    return mPlayerLevel;
}

TArray<FPlayerLevelUpData> UPlayerUnitModel::PredictLevelChange(float ExpGain) const
{
	UAttributeSetComponentModel* AttributeSetCompModel = GetAttributeComponentModel();
	checkf(AttributeSetCompModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	TArray<FPlayerLevelUpData> LevelUpDatas = CalculateLevelChange(mPlayerLevel, AttributeSetCompModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetExpAttribute()), ExpGain);
	return LevelUpDatas;
}

void UPlayerUnitModel::PostChangeExperience(float OldExp, float NewExp)
{
	UAttributeSetComponentModel* AttributeSetCompModel = GetAttributeComponentModel();
	checkf(AttributeSetCompModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	const TArray<FPlayerLevelUpData> LevelUpDatas = CalculateLevelChange(mPlayerLevel, OldExp, NewExp - OldExp);
	if (LevelUpDatas.IsEmpty() == false)
	{
		AttributeSetCompModel->ApplyModToAttribute(UPlayerUnitAttributeSet::GetExpAttribute(), ETacticalModOp::Override, LevelUpDatas.Last().mCarryExp);
		for (const FPlayerLevelUpData& Data : LevelUpDatas)
		{
			LevelUp(Data);
		}
	}
}

TArray<FPlayerLevelUpData> UPlayerUnitModel::CalculateLevelChange(int32 StartLevel, float StartExp, float ExpGain) const
{
	TArray<FPlayerLevelUpData> Result;

	if (ExpGain <= 0.f)
	{
		return Result;
	}

	const int32 SafeMaxLevel = FMath::Max(1, ULevelAttributeSet::GetMaxLevel(this));

	const int32 SafeStartLevel = FMath::Max(1, StartLevel);
	const float SafeStartExp = FMath::Max(0.f, StartExp);
	const float SafeExpGain = FMath::Max(0.f, ExpGain);

	int32 CurLevel = SafeStartLevel;
	float CurStartExp = SafeStartExp;
	float CurCarryExp = SafeStartExp + SafeExpGain;

	while (SafeMaxLevel > CurLevel)
	{
		float CurMaxExp = ULevelAttributeSet::GetMaxExp(this, CurLevel);
		if (FMath::IsWithinInclusive(CurMaxExp, 1.f, CurCarryExp) == false)
		{
			break;
		}

		FPlayerLevelUpData LevelUpData;
		LevelUpData.mMaxExp = CurMaxExp;

		LevelUpData.mPreLevel = CurLevel;
		LevelUpData.mPreExp = CurStartExp;

		{
			++CurLevel;
			CurCarryExp -= CurMaxExp;
			CurStartExp = 0.f;
		}

		LevelUpData.mCurLevel = CurLevel;
		LevelUpData.mCurExp = CurMaxExp;
		LevelUpData.mCarryExp = CurCarryExp;

		Result.Add(LevelUpData);
	}

	return Result;
}

void UPlayerUnitModel::LevelUp(const FPlayerLevelUpData& LevelUpData)
{
	FPlayerLevelUpEvent Event;
	Event.mData = LevelUpData;
	Event.mHasRarityRate = ULevelAttributeSet::GetRarityRate(this, Event.mData.mCurLevel, OUT Event.mSkillRarityRate);

	SetPlayerLevel(Event.mData.mCurLevel);
	OnPlayerLevelUp.Broadcast(this, Event);
}

UArtifactComponentModel* UPlayerUnitModel::GetArtifactComponentModel() const
{
    return mArtifactCompModel;
}
