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

	if (WorldContextObject == nullptr)
	{
		return Rate;
	}

	const UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(WorldContextObject);
	if (TacticalFrameworkModel == nullptr)
	{
		return Rate;
	}

	FTacticalAttributeSetInitter* Initter = const_cast<UTacticalFrameworkModel*>(TacticalFrameworkModel)->GetAttributeSetInitter();
	if (Initter == nullptr)
	{
		return Rate;
	}

	const float CommonWeight = Initter->GetAttributeSetValue(ULevelAttributeSet::StaticClass(), GetCommonSkillWeightAttribute().GetUProperty(), KEY_NAME, Level);
	const float RareWeight = Initter->GetAttributeSetValue(ULevelAttributeSet::StaticClass(), GetRareSkillWeightAttribute().GetUProperty(), KEY_NAME, Level);
	const float EpicWeight = Initter->GetAttributeSetValue(ULevelAttributeSet::StaticClass(), GetEpicSkillWeightAttribute().GetUProperty(), KEY_NAME, Level);

	Rate.mWeights[static_cast<uint8>(ERarityType::Common)] = CommonWeight;
	Rate.mWeights[static_cast<uint8>(ERarityType::Rare)] = RareWeight;
	Rate.mWeights[static_cast<uint8>(ERarityType::Epic)] = EpicWeight;

	return Rate;
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

