#include "Dice/DiceModel.h"

#include "DataAsset/DiceData/StaticDiceData.h"
#include "Engine/Texture.h"

namespace
{
	// 면 수는 최소 2면(동전)으로 강제한다. 0/1면은 굴림 자체가 성립하지 않기 때문이다.
	int32 SanitizeDiceFaceCount(int32 FaceCount)
	{
		return FMath::Max(2, FaceCount);
	}

	// 면 값이 따로 주어지지 않을 때 쓰는 기본값: 1번 면=1 … N번 면=N (1-base 값, 0-base index).
	void BuildDefaultFaceValues(int32 FaceCount, TArray<int32>& OutFaceValues)
	{
		OutFaceValues.Reset(FaceCount);
		for (int32 FaceIndex = 0; FaceIndex < FaceCount; ++FaceIndex)
		{
			OutFaceValues.Add(FaceIndex + 1);
		}
	}
}

// 희귀도/식별자만으로 초기화한다. 면 값은 1..N 기본값, 텍스처는 비움.
void UDiceModel::Init(ERarityType Rarity, const FPrimaryAssetId& SourceDiceId, int32 FaceCount)
{
	// 기본 면 값(1..N)을 만들어 면별 초기화 경로(InitWithFaces)로 위임한다 — 초기화 로직 일원화.
	TArray<int32> DefaultValues;
	BuildDefaultFaceValues(SanitizeDiceFaceCount(FaceCount), DefaultValues);
	InitWithFaces(Rarity, SourceDiceId, FaceCount, DefaultValues, TArray<TObjectPtr<UTexture>>());
}

// 면별 값/텍스처까지 받아 초기 상태를 채운다. 모든 초기화의 최종 종착 함수다.
void UDiceModel::InitWithFaces(ERarityType Rarity, const FPrimaryAssetId& SourceDiceId, int32 FaceCount, const TArray<int32>& FaceValues, const TArray<TObjectPtr<UTexture>>& FaceTextures)
{
	mRarityType = Rarity;
	mSourceDiceId = SourceDiceId;
	mFaceCount = SanitizeDiceFaceCount(FaceCount);

	// 면 값: 들어온 배열을 면 수만큼 채우되, 모자란 면은 기본값(index+1)으로 메운다.
	mFaceValues.Reset(mFaceCount);
	for (int32 FaceIndex = 0; FaceIndex < mFaceCount; ++FaceIndex)
	{
		const int32 FaceValue = FaceValues.IsValidIndex(FaceIndex) ? FaceValues[FaceIndex] : FaceIndex + 1;
		mFaceValues.Add(FaceValue);
	}

	// 면 텍스처: 면 값과 같은 길이로 맞춘다. 없는 면은 nullptr(=기본 면 텍스처 사용).
	mFaceTextures.Reset(mFaceCount);
	for (int32 FaceIndex = 0; FaceIndex < mFaceCount; ++FaceIndex)
	{
		mFaceTextures.Add(FaceTextures.IsValidIndex(FaceIndex) ? FaceTextures[FaceIndex] : nullptr);
	}

	// 굴림 상태는 "아직 안 굴림"으로 리셋한다.
	mRolledFaceIndex = INDEX_NONE;
	mCurrentValue = 0;
	mIsRolled = false;
	mIsUsed = false;
}

// 고정 템플릿(UStaticDiceData)에서 면/희귀도를 읽어 런타임 상태로 초기화한다.
void UDiceModel::InitFromStatic(const UStaticDiceData* StaticData, const FPrimaryAssetId& SourceDiceId)
{
	const ERarityType Rarity = StaticData != nullptr ? StaticData->mRarityType : ERarityType::Common;

	// 템플릿이 없으면 희귀도만 들고 기본 주사위로 초기화한다(방어적 처리).
	if (StaticData == nullptr)
	{
		Init(Rarity, SourceDiceId);
		return;
	}

	// 템플릿의 면 배열을 면 수만큼 펼친다. 텍스처는 SoftPtr라 여기서 동기 로드해 채운다.
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
			// 템플릿이 면 수보다 적게 정의했으면 모자란 면은 기본값으로 메운다.
			FaceValues.Add(FaceIndex + 1);
			FaceTextures.Add(nullptr);
		}
	}

	InitWithFaces(Rarity, SourceDiceId, FaceCount, FaceValues, FaceTextures);
}

// 난수 스트림으로 물리 면 하나를 뽑고, 그 면의 값을 현재 결과로 확정한다.
int32 UDiceModel::Roll(const FRandomStream& Stream)
{
	// 면 값이 비어 있는 비정상 상태면 기본값으로 복구해 크래시를 막는다.
	if (mFaceValues.IsEmpty())
	{
		BuildDefaultFaceValues(mFaceCount, mFaceValues);
	}

	// 면 index를 추첨 → 그 면에 적힌 값이 이번 굴림 결과다(index와 값을 분리해 3D 면 정합도 가능).
	mRolledFaceIndex = Stream.RandRange(0, mFaceValues.Num() - 1);
	mCurrentValue = mFaceValues[mRolledFaceIndex];
	mIsRolled = true;
	mIsUsed = false;   // 새 턴 굴림에서 이전 턴 사용 잠금을 해제.
	return mCurrentValue;
}

// 예측용 깊은 복제본. 사본에서만 Roll/계산을 돌려 라이브 주사위 상태를 보존한다.
UDiceModel* UDiceModel::Clone(UObject* InOuter) const
{
	// Outer 미지정 시 원본과 같은 Outer에 붙인다(GC 수명 일관성).
	return DuplicateObject<UDiceModel>(this, InOuter != nullptr ? InOuter : GetOuter());
}
