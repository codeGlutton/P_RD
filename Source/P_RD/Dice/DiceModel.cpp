#include "Dice/DiceModel.h"

#include "DataAsset/DiceData/StaticDiceData.h"
#include "Engine/Texture.h"

namespace
{
	int32 SanitizeDiceFaceCount(int32 FaceCount)
	{
		return FMath::Max(2, FaceCount);
	}

	void BuildDefaultFaceValues(int32 FaceCount, TArray<int32>& OutFaceValues)
	{
		OutFaceValues.Reset(FaceCount);
		for (int32 FaceIndex = 0; FaceIndex < FaceCount; ++FaceIndex)
		{
			OutFaceValues.Add(FaceIndex + 1);
		}
	}
}

void UDiceModel::Init(ERarityType Rarity, const FPrimaryAssetId& SourceDiceId, int32 FaceCount)
{
	TArray<int32> DefaultValues;
	BuildDefaultFaceValues(SanitizeDiceFaceCount(FaceCount), DefaultValues);
	InitWithFaces(Rarity, SourceDiceId, FaceCount, DefaultValues, TArray<TObjectPtr<UTexture>>());
}

void UDiceModel::InitWithFaces(ERarityType Rarity, const FPrimaryAssetId& SourceDiceId, int32 FaceCount, const TArray<int32>& FaceValues, const TArray<TObjectPtr<UTexture>>& FaceTextures)
{
	mRarityType = Rarity;
	mSourceDiceId = SourceDiceId;
	mFaceCount = SanitizeDiceFaceCount(FaceCount);

	mFaceValues.Reset(mFaceCount);
	for (int32 FaceIndex = 0; FaceIndex < mFaceCount; ++FaceIndex)
	{
		const int32 FaceValue = FaceValues.IsValidIndex(FaceIndex) ? FaceValues[FaceIndex] : FaceIndex + 1;
		mFaceValues.Add(FaceValue);
	}

	mFaceTextures.Reset(mFaceCount);
	for (int32 FaceIndex = 0; FaceIndex < mFaceCount; ++FaceIndex)
	{
		mFaceTextures.Add(FaceTextures.IsValidIndex(FaceIndex) ? FaceTextures[FaceIndex] : nullptr);
	}

	mRolledFaceIndex = INDEX_NONE;
	mCurrentValue = 0;
	mIsRolled = false;
	mIsUsed = false;
}

void UDiceModel::InitFromStatic(const UStaticDiceData* StaticData, const FPrimaryAssetId& SourceDiceId)
{
	const ERarityType Rarity = StaticData != nullptr ? StaticData->mRarityType : ERarityType::Common;
	if (StaticData == nullptr)
	{
		Init(Rarity, SourceDiceId);
		return;
	}

	const int32 FaceCount = SanitizeDiceFaceCount(StaticData->mFaceCount);
	TArray<int32> FaceValues;
	TArray<TObjectPtr<UTexture>> FaceTextures;
	FaceValues.Reserve(FaceCount);
	FaceTextures.Reserve(FaceCount);
	for (int32 FaceIndex = 0; FaceIndex < FaceCount; ++FaceIndex)
	{
		if (StaticData->mFaces.IsValidIndex(FaceIndex))
		{
			FaceValues.Add(StaticData->mFaces[FaceIndex].mValue);
			FaceTextures.Add(StaticData->mFaces[FaceIndex].mTexture.LoadSynchronous());
		}
		else
		{
			FaceValues.Add(FaceIndex + 1);
			FaceTextures.Add(nullptr);
		}
	}

	InitWithFaces(Rarity, SourceDiceId, FaceCount, FaceValues, FaceTextures);
}

int32 UDiceModel::Roll(const FRandomStream& Stream)
{
	if (mFaceValues.IsEmpty())
	{
		BuildDefaultFaceValues(mFaceCount, mFaceValues);
	}

	mRolledFaceIndex = Stream.RandRange(0, mFaceValues.Num() - 1);
	mCurrentValue = mFaceValues[mRolledFaceIndex];
	mIsRolled = true;
	mIsUsed = false;   // 새 턴 굴림에서 이전 턴 사용 잠금을 해제.
	return mCurrentValue;
}

UDiceModel* UDiceModel::Clone(UObject* InOuter) const
{
	return DuplicateObject<UDiceModel>(this, InOuter != nullptr ? InOuter : GetOuter());
}
