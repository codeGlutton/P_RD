#include "AttributeSet/LevelAttributeSet.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "TAS/AttributeSet/TacticalAttributeSet.h"

#include "Setting/GameBalanceSettings.h"

const FName ULevelAttributeSet::KEY_NAME = TEXT("PlayerLevel");

ULevelAttributeSet::ULevelAttributeSet()
{
}

FLevelAttributeCache ULevelAttributeSet::MakeCache(const UObject* WorldContextObject)
{
	FLevelAttributeCache Cache;
	Cache.mMaxExps = GetMaxExps(WorldContextObject);
	Cache.mRarityRates = GetRarityRates(WorldContextObject);
	Cache.mPrices = GetPrices(WorldContextObject);
	return Cache;
}

int32 ULevelAttributeSet::GetMaxLevel(const UObject* WorldContextObject)
{
	const UGameBalanceSettings* GameBalanceSettings = GetDefault<UGameBalanceSettings>();
	checkf(GameBalanceSettings != nullptr, TEXT("게임 밸런스 세팅 nullptr"));

	return GameBalanceSettings->mPlayerMaxLevel;
}

TArray<float> ULevelAttributeSet::GetMaxExps(const UObject* WorldContextObject)
{
	TArray<float> MaxExps;
	GetMaxExps(WorldContextObject, OUT MaxExps);
	return MaxExps;
}

bool ULevelAttributeSet::GetMaxExps(const UObject* WorldContextObject, OUT TArray<float>& OutMaxExps)
{
	OutMaxExps = TArray<float>();

	if (WorldContextObject == nullptr)
	{
		return false;
	}

	const UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(WorldContextObject);
	if (TacticalFrameworkModel == nullptr)
	{
		return false;
	}

	FTacticalAttributeSetInitter* Initter = const_cast<UTacticalFrameworkModel*>(TacticalFrameworkModel)->GetAttributeSetInitter();
	if (Initter == nullptr)
	{
		return false;
	}

	OutMaxExps = Initter->GetAttributeSetValues(ULevelAttributeSet::StaticClass(), GetMaxExpAttribute().GetUProperty(), KEY_NAME);
	return true;
}

float ULevelAttributeSet::GetMaxExp(const UObject* WorldContextObject, int32 Level)
{
	float MaxExp = 0.f;
	GetMaxExp(WorldContextObject, Level, OUT MaxExp);
	return MaxExp;
}

bool ULevelAttributeSet::GetMaxExp(const UObject* WorldContextObject, int32 Level, OUT float& OutMaxExp)
{
	OutMaxExp = 0.f;

	if (WorldContextObject == nullptr)
	{
		return false;
	}

	const UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(WorldContextObject);
	if (TacticalFrameworkModel == nullptr)
	{
		return false;
	}

	FTacticalAttributeSetInitter* Initter = const_cast<UTacticalFrameworkModel*>(TacticalFrameworkModel)->GetAttributeSetInitter();
	if (Initter == nullptr)
	{
		return false;
	}

	OutMaxExp = Initter->GetAttributeSetValue(ULevelAttributeSet::StaticClass(), GetMaxExpAttribute().GetUProperty(), KEY_NAME, Level);
	return true;
}

TArray<FRarityRate> ULevelAttributeSet::GetRarityRates(const UObject* WorldContextObject)
{
	TArray<FRarityRate> Rates;
	GetRarityRates(WorldContextObject, OUT Rates);
	return Rates;
}

bool ULevelAttributeSet::GetRarityRates(const UObject* WorldContextObject, OUT TArray<FRarityRate>& OutRates)
{
	OutRates = TArray<FRarityRate>();

	if (WorldContextObject == nullptr)
	{
		return false;
	}

	const UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(WorldContextObject);
	if (TacticalFrameworkModel == nullptr)
	{
		return false;
	}

	FTacticalAttributeSetInitter* Initter = const_cast<UTacticalFrameworkModel*>(TacticalFrameworkModel)->GetAttributeSetInitter();
	if (Initter == nullptr)
	{
		return false;
	}

	const TArray<float> CommonWeights = Initter->GetAttributeSetValues(ULevelAttributeSet::StaticClass(), GetCommonSkillWeightAttribute().GetUProperty(), KEY_NAME);
	const TArray<float> RareWeights = Initter->GetAttributeSetValues(ULevelAttributeSet::StaticClass(), GetRareSkillWeightAttribute().GetUProperty(), KEY_NAME);
	const TArray<float> EpicWeights = Initter->GetAttributeSetValues(ULevelAttributeSet::StaticClass(), GetEpicSkillWeightAttribute().GetUProperty(), KEY_NAME);

	const int32 LevelCount = FMath::Min(CommonWeights.Num(), FMath::Min(RareWeights.Num(), EpicWeights.Num()));
	OutRates.Reserve(LevelCount);

	for (int32 LevelIndex = 0; LevelIndex < LevelCount; ++LevelIndex)
	{
		FRarityRate Rate;
		Rate.mWeights[static_cast<uint8>(ERarityType::Common)] = CommonWeights[LevelIndex];
		Rate.mWeights[static_cast<uint8>(ERarityType::Rare)] = RareWeights[LevelIndex];
		Rate.mWeights[static_cast<uint8>(ERarityType::Epic)] = EpicWeights[LevelIndex];
		OutRates.Add(Rate);
	}

	return true;
}

FRarityRate ULevelAttributeSet::GetRarityRate(const UObject* WorldContextObject, int32 Level)
{
	FRarityRate Rate;
	GetRarityRate(WorldContextObject, Level, OUT Rate);
	return Rate;
}

bool ULevelAttributeSet::GetRarityRate(const UObject* WorldContextObject, int32 Level, OUT FRarityRate& OutRate)
{
	OutRate = FRarityRate();
	if (WorldContextObject == nullptr || Level < 1)
	{
		return false;
	}

	const TArray<FRarityRate> RarityRates = GetRarityRates(WorldContextObject);
	const int32 LevelIndex = Level - 1;
	if (RarityRates.IsValidIndex(LevelIndex) == false || RarityRates[LevelIndex].IsValid() == false)
	{
		return false;
	}

	OutRate = RarityRates[LevelIndex];
	return true;
}

TArray<float> ULevelAttributeSet::GetPrices(const UObject* WorldContextObject)
{
	TArray<float> Prices;
	GetPrices(WorldContextObject, OUT Prices);
	return Prices;
}

bool ULevelAttributeSet::GetPrices(const UObject* WorldContextObject, OUT TArray<float>& OutPrices)
{
	OutPrices = TArray<float>();

	if (WorldContextObject == nullptr)
	{
		return false;
	}

	const UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(WorldContextObject);
	if (TacticalFrameworkModel == nullptr)
	{
		return false;
	}

	FTacticalAttributeSetInitter* Initter = const_cast<UTacticalFrameworkModel*>(TacticalFrameworkModel)->GetAttributeSetInitter();
	if (Initter == nullptr)
	{
		return false;
	}

	OutPrices = Initter->GetAttributeSetValues(ULevelAttributeSet::StaticClass(), GetPriceAttribute().GetUProperty(), KEY_NAME);
	return true;
}

float ULevelAttributeSet::GetPrice(const UObject* WorldContextObject, int32 Level)
{
	float Price = 0.f;
	GetPrice(WorldContextObject, Level, OUT Price);
	return Price;
}

bool ULevelAttributeSet::GetPrice(const UObject* WorldContextObject, int32 Level, OUT float& OutPrice)
{
	OutPrice = 0.f;

	if (WorldContextObject == nullptr)
	{
		return false;
	}

	const UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(WorldContextObject);
	if (TacticalFrameworkModel == nullptr)
	{
		return false;
	}

	FTacticalAttributeSetInitter* Initter = const_cast<UTacticalFrameworkModel*>(TacticalFrameworkModel)->GetAttributeSetInitter();
	if (Initter == nullptr)
	{
		return false;
	}

	OutPrice = Initter->GetAttributeSetValue(ULevelAttributeSet::StaticClass(), GetPriceAttribute().GetUProperty(), KEY_NAME, Level);
	return true;
}

