#include "AttributeSet/LevelAttributeCache.h"

FRarityRate FLevelAttributeCache::GetRarityRate(int32 Level) const
{
	const int32 Index = Level - 1;
	if (mRarityRates.IsValidIndex(Index) == true)
	{
		return mRarityRates[Index];
	}
	return FRarityRate();
}

float FLevelAttributeCache::GetPrice(int32 Level) const
{
	const int32 Index = Level - 1;
	if (mPrices.IsValidIndex(Index) == true)
	{
		return mPrices[Index];
	}
	return 0.f;
}
