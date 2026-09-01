#include "AttributeSet/LevelAttributeSet.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "TAS/AttributeSet/TacticalAttributeSet.h"

const FName ULevelAttributeSet::KEY_NAME = TEXT("PlayerLevel");

ULevelAttributeSet::ULevelAttributeSet()
{
}

FLevelAttributeCache ULevelAttributeSet::MakeCache(const UObject* WorldContextObject)
{
	FLevelAttributeCache Cache;
	Cache.mRarityRates = GetRarityRates(WorldContextObject);
	Cache.mPrices = GetPrices(WorldContextObject);
	return Cache;
}

TArray<FRarityRate> ULevelAttributeSet::GetRarityRates(const UObject* WorldContextObject)
{
	TArray<FRarityRate> RarityRates;

	if (WorldContextObject == nullptr)
	{
		return RarityRates;
	}

	const UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(WorldContextObject);
	if (TacticalFrameworkModel == nullptr)
	{
		return RarityRates;
	}

	FTacticalAttributeSetInitter* Initter = const_cast<UTacticalFrameworkModel*>(TacticalFrameworkModel)->GetAttributeSetInitter();
	if (Initter == nullptr)
	{
		return RarityRates;
	}

	const TArray<float> CommonWeights = Initter->GetAttributeSetValues(ULevelAttributeSet::StaticClass(), GetCommonSkillWeightAttribute().GetUProperty(), KEY_NAME);
	const TArray<float> RareWeights = Initter->GetAttributeSetValues(ULevelAttributeSet::StaticClass(), GetRareSkillWeightAttribute().GetUProperty(), KEY_NAME);
	const TArray<float> EpicWeights = Initter->GetAttributeSetValues(ULevelAttributeSet::StaticClass(), GetEpicSkillWeightAttribute().GetUProperty(), KEY_NAME);

	const int32 LevelCount = FMath::Min(CommonWeights.Num(), FMath::Min(RareWeights.Num(), EpicWeights.Num()));
	RarityRates.Reserve(LevelCount);

	for (int32 LevelIndex = 0; LevelIndex < LevelCount; ++LevelIndex)
	{
		FRarityRate Rate;
		Rate.mWeights[static_cast<uint8>(ERarityType::Common)] = CommonWeights[LevelIndex];
		Rate.mWeights[static_cast<uint8>(ERarityType::Rare)] = RareWeights[LevelIndex];
		Rate.mWeights[static_cast<uint8>(ERarityType::Epic)] = EpicWeights[LevelIndex];
		RarityRates.Add(Rate);
	}

	return RarityRates;
}

FRarityRate ULevelAttributeSet::GetRarityRate(const UObject* WorldContextObject, int32 Level)
{
	FRarityRate Rate;
	TryGetRarityRate(WorldContextObject, Level, OUT Rate);
	return Rate;
}

bool ULevelAttributeSet::TryGetRarityRate(const UObject* WorldContextObject, int32 Level, OUT FRarityRate& OutRate)
{
	OutRate = FRarityRate();
	if (WorldContextObject == nullptr || Level < 1)
	{
		return false;
	}

	const TArray<FRarityRate> RarityRates = GetRarityRates(WorldContextObject);
	const int32 LevelIndex = Level - 1;
	if (RarityRates.IsValidIndex(LevelIndex) == false)
	{
		return false;
	}

	const FRarityRate& Candidate = RarityRates[LevelIndex];
	bool HasPositiveWeight = false;
	for (uint8 RarityIndex = 0; RarityIndex < static_cast<uint8>(ERarityType::Count); ++RarityIndex)
	{
		const float Weight = Candidate.mWeights[RarityIndex];
		if (FMath::IsFinite(Weight) == false || Weight < 0.f)
		{
			return false;
		}
		HasPositiveWeight |= Weight > 0.f;
	}

	if (HasPositiveWeight == false)
	{
		return false;
	}

	OutRate = Candidate;
	return true;
}

TArray<float> ULevelAttributeSet::GetPrices(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		return TArray<float>();
	}

	const UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(WorldContextObject);
	if (TacticalFrameworkModel == nullptr)
	{
		return TArray<float>();
	}

	FTacticalAttributeSetInitter* Initter = const_cast<UTacticalFrameworkModel*>(TacticalFrameworkModel)->GetAttributeSetInitter();
	if (Initter == nullptr)
	{
		return TArray<float>();
	}

	return Initter->GetAttributeSetValues(ULevelAttributeSet::StaticClass(), GetPriceAttribute().GetUProperty(), KEY_NAME);
}

float ULevelAttributeSet::GetPrice(const UObject* WorldContextObject, int32 Level)
{
	if (WorldContextObject == nullptr)
	{
		return 0.f;
	}

	const UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(WorldContextObject);
	if (TacticalFrameworkModel == nullptr)
	{
		return 0.f;
	}

	FTacticalAttributeSetInitter* Initter = const_cast<UTacticalFrameworkModel*>(TacticalFrameworkModel)->GetAttributeSetInitter();
	if (Initter == nullptr)
	{
		return 0.f;
	}

	return Initter->GetAttributeSetValue(ULevelAttributeSet::StaticClass(), GetPriceAttribute().GetUProperty(), KEY_NAME, Level);
}

