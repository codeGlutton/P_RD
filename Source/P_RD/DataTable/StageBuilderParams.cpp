#include "DataTable/StageBuilderParams.h"

float FRoomBuildRate::GetRate(ERoomBuildType Type) const
{
	const float TotalWeight = GetTotalWeight();
	if (TotalWeight > 0.f)
	{
		return mWeights[StaticCast<uint8>(Type)] / TotalWeight;
	}
	return 0.f;
}

float FRoomBuildRate::GetRate(ERoomBuildType Type, const TArray<ERoomBuildType>& IncludedTypes) const
{
	const float TotalWeight = GetTotalWeight(IncludedTypes);
	if (TotalWeight > 0.f)
	{
		return mWeights[StaticCast<uint8>(Type)] / TotalWeight;
	}
	return 0.f;
}

ERoomType FRoomBuildRate::GetType(float Rate) const
{
	float Weight = Rate * GetTotalWeight();

	const uint8 Count = StaticCast<uint8>(ERoomBuildType::Count);
	for (uint8 i = 0; i < Count; ++i)
	{
		if (Weight < mWeights[i])
		{
			return ConventToRoomType(StaticCast<ERoomBuildType>(i));
		}

		Weight -= mWeights[i];
	}

	checkf(false, TEXT("1을 벗어난 잘못된 Rate 입력"));
	return ERoomType::None;
}

ERoomType FRoomBuildRate::GetType(float Rate, const TArray<ERoomBuildType>& IncludedTypes) const
{
	float Weight = Rate * GetTotalWeight(IncludedTypes);

	for (ERoomBuildType Type : IncludedTypes)
	{
		const uint8 TypeIndex = StaticCast<uint8>(Type);
		if (Weight < mWeights[TypeIndex])
		{
			return ConventToRoomType(Type);
		}

		Weight -= mWeights[TypeIndex];
	}

	checkf(false, TEXT("1을 벗어난 잘못된 Rate 입력"));
	return ERoomType::None;
}

float FRoomBuildRate::GetTotalWeight() const
{
	float TotalWeight = 0.f;

	const uint8 Count = StaticCast<uint8>(ERoomBuildType::Count);
	for (uint8 i = 0; i < Count; ++i)
	{
		TotalWeight += mWeights[i];
	}

	return TotalWeight;
}

float FRoomBuildRate::GetTotalWeight(const TArray<ERoomBuildType>& IncludedTypes) const
{
	float TotalWeight = 0.f;

	for (ERoomBuildType Type : IncludedTypes)
	{
		TotalWeight += mWeights[StaticCast<uint8>(Type)];
	}

	return TotalWeight;
}

ERoomType FRoomBuildRate::ConventToRoomType(ERoomBuildType Type) const
{
	switch (Type)
	{
	case ERoomBuildType::Shop:
		return ERoomType::Shop;
	case ERoomBuildType::Monster:
		return ERoomType::Monster;
	case ERoomBuildType::EliteMonster:
		return ERoomType::EliteMonster;
	}
	return ERoomType::None;
}
