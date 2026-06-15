#include "Dice/DiceData.h"

#include "DataAsset/DiceData/StaticDiceData.h"

void UDiceData::Init(ERarityType Rarity, const FPrimaryAssetId& SourceDiceId, int32 FaceCount)
{
	mRarityType = Rarity;
	mSourceDiceId = SourceDiceId;
	mFaceCount = FMath::Max(1, FaceCount);
	mCurrentValue = 0;
	mIsRolled = false;
}

void UDiceData::InitFromStatic(const UStaticDiceData* StaticData, const FPrimaryAssetId& SourceDiceId)
{
	const ERarityType Rarity = StaticData != nullptr ? StaticData->mRarityType : ERarityType::Common;
	Init(Rarity, SourceDiceId);
}

int32 UDiceData::Roll(const FRandomStream& Stream)
{
	mCurrentValue = Stream.RandRange(1, FMath::Max(1, mFaceCount));
	mIsRolled = true;
	return mCurrentValue;
}

UDiceData* UDiceData::Clone(UObject* InOuter) const
{
	return DuplicateObject<UDiceData>(this, InOuter != nullptr ? InOuter : GetOuter());
}
