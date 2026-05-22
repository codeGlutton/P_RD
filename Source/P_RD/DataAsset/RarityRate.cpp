#include "DataAsset/RarityRate.h"

float FRarityRate::GetRate(ERarityType Type) const
{
	const float TotalWeight = GetTotalWeight();
	if (TotalWeight > 0.f)
	{
		return mWeights[StaticCast<uint8>(Type)] / TotalWeight;
	}
	return 0.f;
}

float FRarityRate::GetRate(ERarityType Type, const TArray<ERarityType>& IncludedTypes) const
{
	const float TotalWeight = GetTotalWeight(IncludedTypes);
	if (TotalWeight > 0.f)
	{
		return mWeights[StaticCast<uint8>(Type)] / TotalWeight;
	}
	return 0.f;
}

ERarityType FRarityRate::GetType(float Rate) const
{
	float Weight = Rate * GetTotalWeight();

	const uint8 Count = StaticCast<uint8>(ERarityType::Count);
	for (uint8 i = 0; i < Count; ++i)
	{
		if (Weight < mWeights[i])
		{
			return StaticCast<ERarityType>(i);
		}

		Weight -= mWeights[i];
	}

	checkf(false, TEXT("1을 벗어난 잘못된 Rate 입력"));
	return ERarityType::Common;
}

ERarityType FRarityRate::GetType(float Rate, const TArray<ERarityType>& IncludedTypes) const
{
	float Weight = Rate * GetTotalWeight(IncludedTypes);

	for (ERarityType Type : IncludedTypes)
	{
		const uint8 TypeIndex = mWeights[StaticCast<uint8>(Type)];
		if (Weight < mWeights[TypeIndex])
		{
			return Type;
		}

		Weight -= mWeights[TypeIndex];
	}

	checkf(false, TEXT("1을 벗어난 잘못된 Rate 입력"));
	return ERarityType::Common;
}

float FRarityRate::GetTotalWeight() const
{
	float TotalWeight = 0.f;

	const uint8 Count = StaticCast<uint8>(ERarityType::Count);
	for (uint8 i = 0; i < Count; ++i)
	{
		TotalWeight += mWeights[i];
	}

	return TotalWeight;
}

float FRarityRate::GetTotalWeight(const TArray<ERarityType>& IncludedTypes) const
{
	float TotalWeight = 0.f;

	for (ERarityType Type : IncludedTypes)
	{
		TotalWeight += mWeights[StaticCast<uint8>(Type)];
	}

	return TotalWeight;
}
