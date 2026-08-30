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
		RefreshMaxExperienceThreshold();
    }
}

void UPlayerUnitModel::SetPlayerLevel(int32 PlayerLevel)
{
	int32 NormalizedLevel = FMath::Max(1, PlayerLevel);
	const int32 MaxPlayerLevel = GetMaxPlayerLevel();
	if (MaxPlayerLevel > 0)
	{
		NormalizedLevel = FMath::Min(NormalizedLevel, MaxPlayerLevel);
	}

	const bool LevelChanged = mPlayerLevel != NormalizedLevel;
	mPlayerLevel = NormalizedLevel;
	RefreshMaxExperienceThreshold();

	if (LevelChanged)
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

TArray<float> UPlayerUnitModel::GetExperienceThresholds() const
{
	TArray<float> Thresholds;
	if (mStaticSpawnData == nullptr)
	{
		return Thresholds;
	}

	const UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
	if (TacticalFrameworkModel == nullptr)
	{
		return Thresholds;
	}

	FTacticalAttributeSetInitter* Initter = const_cast<UTacticalFrameworkModel*>(TacticalFrameworkModel)->GetAttributeSetInitter();
	if (Initter == nullptr)
	{
		return Thresholds;
	}

	const TArray<float> ConfiguredValues = Initter->GetAttributeSetValues(
		UPlayerUnitAttributeSet::StaticClass(),
		UPlayerUnitAttributeSet::GetMaxExpAttribute().GetUProperty(),
		GetBoardActorKeyName());

	Thresholds.Reserve(ConfiguredValues.Num());
	for (const float Value : ConfiguredValues)
	{
		// 레벨 테이블은 1레벨부터 연속이어야 한다. 중간의 잘못된 값 이후는 사용하지 않는다.
		if (FMath::IsFinite(Value) == false || Value <= 0.f)
		{
			break;
		}
		Thresholds.Add(Value);
	}
	return Thresholds;
}

float UPlayerUnitModel::GetMaxExpForPlayerLevel(int32 PlayerLevel) const
{
	const TArray<float> Thresholds = GetExperienceThresholds();
	const int32 LevelIndex = PlayerLevel - 1;
	return Thresholds.IsValidIndex(LevelIndex) ? Thresholds[LevelIndex] : 0.f;
}

int32 UPlayerUnitModel::GetMaxPlayerLevel() const
{
	return GetExperienceThresholds().Num();
}

FPlayerExpProgression UPlayerUnitModel::PreviewExperienceGain(float ExperienceGain) const
{
	float CurrentExp = 0.f;
	if (const UAttributeSetComponentModel* ASC = GetAttributeComponentModel())
	{
		CurrentExp = ASC->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetExpAttribute());
	}

	return CalculateExperienceProgression(mPlayerLevel, CurrentExp, ExperienceGain, GetExperienceThresholds());
}

FPlayerExpProgression UPlayerUnitModel::CalculateExperienceProgression(
	int32 StartingLevel,
	float StartingExp,
	float ExperienceGain,
	const TArray<float>& Thresholds)
{
	FPlayerExpProgression Result;

	TArray<float> ValidThresholds;
	ValidThresholds.Reserve(Thresholds.Num());
	for (const float Threshold : Thresholds)
	{
		if (FMath::IsFinite(Threshold) == false || Threshold <= 0.f)
		{
			break;
		}
		ValidThresholds.Add(Threshold);
	}

	const float SafeStartingExp = FMath::IsFinite(StartingExp) ? FMath::Max(0.f, StartingExp) : 0.f;
	const float SafeExperienceGain = FMath::IsFinite(ExperienceGain) ? FMath::Max(0.f, ExperienceGain) : 0.f;
	const int32 RequestedStartingLevel = FMath::Max(1, StartingLevel);

	Result.mExperienceGain = SafeExperienceGain;
	Result.mExpBefore = SafeStartingExp;

	if (ValidThresholds.IsEmpty())
	{
		Result.mLevelBefore = RequestedStartingLevel;
		Result.mLevelAfter = RequestedStartingLevel;
		Result.mExpAfter = SafeStartingExp + SafeExperienceGain;
		Result.mMaxExpAfter = 0.f;

		FPlayerExpProgressStep& Step = Result.mSteps.AddDefaulted_GetRef();
		Step.mLevelBefore = RequestedStartingLevel;
		Step.mLevelAfter = RequestedStartingLevel;
		Step.mExpBefore = SafeStartingExp;
		Step.mExpAfter = Result.mExpAfter;
		return Result;
	}

	const int32 MaxPlayerLevel = ValidThresholds.Num();
	int32 CurrentLevel = FMath::Clamp(RequestedStartingLevel, 1, MaxPlayerLevel);
	float CurrentExp = SafeStartingExp;
	float RemainingExperience = SafeExperienceGain;

	Result.mLevelBefore = CurrentLevel;
	Result.mExpBefore = CurrentExp;

	while (true)
	{
		const float CurrentMaxExp = ValidThresholds[CurrentLevel - 1];
		const float TotalExp = CurrentExp + RemainingExperience;

		FPlayerExpProgressStep Step;
		Step.mLevelBefore = CurrentLevel;
		Step.mLevelAfter = CurrentLevel;
		Step.mExpBefore = CurrentExp;
		Step.mMaxExp = CurrentMaxExp;

		if (CurrentLevel < MaxPlayerLevel && TotalExp >= CurrentMaxExp)
		{
			Step.mDidLevelUp = true;
			Step.mLevelAfter = CurrentLevel + 1;
			Step.mExpAfter = CurrentMaxExp;
			Step.mCarryExp = FMath::Max(0.f, TotalExp - CurrentMaxExp);
			Result.mSteps.Add(Step);

			CurrentLevel = Step.mLevelAfter;
			CurrentExp = 0.f;
			RemainingExperience = Step.mCarryExp;
			continue;
		}

		// 마지막 레벨에서는 더 이상 레벨을 올리지 않고 막대를 최대치에서 고정한다.
		Step.mExpAfter = FMath::Min(TotalExp, CurrentMaxExp);
		Result.mSteps.Add(Step);
		CurrentExp = Step.mExpAfter;
		RemainingExperience = 0.f;
		break;
	}

	Result.mLevelAfter = CurrentLevel;
	Result.mExpAfter = CurrentExp;
	Result.mMaxExpAfter = ValidThresholds[CurrentLevel - 1];
	Result.mReachedLevelCap = CurrentLevel >= MaxPlayerLevel;
	return Result;
}

void UPlayerUnitModel::ResolveExperienceChange(float OldValue, float NewValue)
{
	if (mIsResolvingExperience || NewValue < OldValue)
	{
		return;
	}

	const TArray<float> Thresholds = GetExperienceThresholds();
	if (Thresholds.IsEmpty())
	{
		// MaxExp가 0이거나 커브가 없을 때는 값을 다시 쓰지 않아 재귀를 원천 차단한다.
		return;
	}

	const FPlayerExpProgression Progression = CalculateExperienceProgression(
		mPlayerLevel,
		OldValue,
		NewValue - OldValue,
		Thresholds);

	TGuardValue<bool> ResolvingGuard(mIsResolvingExperience, true);
	UAttributeSetComponentModel* ASC = GetAttributeComponentModel();
	if (ASC == nullptr)
	{
		return;
	}

	if (mPlayerLevel != Progression.mLevelBefore)
	{
		SetPlayerLevel(Progression.mLevelBefore);
	}

	if (FMath::IsNearlyEqual(NewValue, Progression.mExpAfter) == false)
	{
		ASC->ApplyModToAttribute(
			UPlayerUnitAttributeSet::GetExpAttribute(),
			ETacticalModOp::Override,
			Progression.mExpAfter);
	}

	for (const FPlayerExpProgressStep& Step : Progression.mSteps)
	{
		if (Step.mDidLevelUp)
		{
			LevelUp(Step.mCarryExp);
		}
	}

	RefreshMaxExperienceThreshold();
}

bool UPlayerUnitModel::LevelUp(float RemainingExperience)
{
	const int32 MaxPlayerLevel = GetMaxPlayerLevel();
	if (MaxPlayerLevel <= 0 || mPlayerLevel >= MaxPlayerLevel)
	{
		return false;
	}

	FPlayerLevelUpEvent Event;
	Event.mPreviousLevel = mPlayerLevel;
	Event.mConsumedExpThreshold = GetMaxExpForPlayerLevel(mPlayerLevel);
	Event.mRemainingExperience = FMath::Max(0.f, RemainingExperience);

	SetPlayerLevel(mPlayerLevel + 1);
	Event.mNewLevel = mPlayerLevel;
	Event.mNextMaxExp = GetMaxExpForPlayerLevel(mPlayerLevel);
	Event.mHasSkillRarityRate = ULevelAttributeSet::TryGetRarityRate(this, mPlayerLevel, OUT Event.mSkillRarityRate);
	OnPlayerLevelUp.Broadcast(this, Event);
	return true;
}

void UPlayerUnitModel::RefreshMaxExperienceThreshold()
{
	UAttributeSetComponentModel* ASC = GetAttributeComponentModel();
	const float NewMaxExp = GetMaxExpForPlayerLevel(mPlayerLevel);
	if (ASC == nullptr || FMath::IsFinite(NewMaxExp) == false || NewMaxExp <= 0.f)
	{
		return;
	}

	const float CurrentMaxExp = ASC->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetMaxExpAttribute());
	if (FMath::IsNearlyEqual(CurrentMaxExp, NewMaxExp) == false)
	{
		ASC->ApplyModToAttribute(UPlayerUnitAttributeSet::GetMaxExpAttribute(), ETacticalModOp::Override, NewMaxExp);
	}
}

UArtifactComponentModel* UPlayerUnitModel::GetArtifactComponentModel() const
{
    return mArtifactCompModel;
}
