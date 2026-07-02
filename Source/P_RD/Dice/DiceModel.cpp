#include "Dice/DiceModel.h"

#include "DataAsset/DiceData/StaticDiceData.h"
#include "Engine/Texture.h"

namespace
{
	/**
	 * @brief 면 수를 최소 2면으로 강제한다.
	 * @details 0/1면은 굴림 자체가 성립하지 않으므로(동전=2면이 최소) 하한을 둔다.
	 */
	int32 SanitizeDiceFaceCount(int32 FaceCount)
	{
		return FMath::Max(2, FaceCount);
	}

	/**
	 * @brief 면 값이 주어지지 않을 때 쓰는 기본 면 값을 만든다.
	 * @details 1번 면=1 … N번 면=N. 배열 index는 0-base, 값은 1-base다.
	 */
	void BuildDefaultFaceValues(int32 FaceCount, TArray<int32>& OutFaceValues)
	{
		OutFaceValues.Reset(FaceCount);
		for (int32 FaceIndex = 0; FaceIndex < FaceCount; ++FaceIndex)
		{
			OutFaceValues.Add(FaceIndex + 1);
		}
	}
}

/**
 * @details 기본 면 값(1..N)을 만들어 면별 초기화 경로(InitWithFaces)로 위임한다.
 * 모든 초기화 로직을 InitWithFaces 한 곳으로 모으기 위한 얇은 진입점이다.
 */
void UDiceModel::Init(ERarityType Rarity, const FPrimaryAssetId& SourceDiceId, int32 FaceCount)
{
	TArray<int32> DefaultValues;
	BuildDefaultFaceValues(SanitizeDiceFaceCount(FaceCount), DefaultValues);
	InitWithFaces(Rarity, SourceDiceId, FaceCount, DefaultValues, TArray<TObjectPtr<UTexture>>());
}

/**
 * @details 모든 초기화가 최종적으로 도달하는 함수다.
 * 면 값/텍스처를 면 수만큼 맞추고(모자란 면은 기본값/nullptr로 메움), 굴림 상태를 "아직 안 굴림"으로 리셋한다.
 */
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

	// 굴림 상태 리셋.
	mRolledFaceIndex = INDEX_NONE;
	mCurrentValue = 0;
	mIsRolled = false;
	mIsUsed = false;
}

/**
 * @details 고정 템플릿의 면 배열을 면 수만큼 펼쳐 InitWithFaces로 넘긴다.
 * 텍스처는 TSoftObjectPtr라 여기서 동기 로드(LoadSynchronous)해 채운다.
 * 템플릿이 null이거나 면을 면 수보다 적게 정의했으면 기본값으로 안전하게 메운다.
 */
void UDiceModel::InitFromStatic(const UStaticDiceData* StaticData, const FPrimaryAssetId& SourceDiceId)
{
	const ERarityType Rarity = StaticData != nullptr ? StaticData->mRarityType : ERarityType::Common;

	// 템플릿이 없으면 희귀도만 들고 기본 주사위로 초기화한다(방어적 처리).
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
			// 템플릿이 면 수보다 적게 정의했으면 모자란 면은 기본값으로 메운다.
			FaceValues.Add(FaceIndex + 1);
			FaceTextures.Add(nullptr);
		}
	}

	InitWithFaces(Rarity, SourceDiceId, FaceCount, FaceValues, FaceTextures);
}

/**
 * @details 면 index를 추첨한 뒤 그 면에 적힌 값을 이번 굴림 결과로 확정한다.
 * index와 값을 분리해 두어 숫자 결과와 별개로 3D 주사위 면 정합(어느 면이 위로 멈출지)을 맞출 수 있다.
 * 난수 스트림을 외부에서 주입받으므로 런 시드 기반 재현이 가능하다.
 */
int32 UDiceModel::Roll(const FRandomStream& Stream)
{
	// 면 값이 비어 있는 비정상 상태면 기본값으로 복구해 크래시를 막는다.
	if (mFaceValues.IsEmpty())
	{
		BuildDefaultFaceValues(mFaceCount, mFaceValues);
	}

	mRolledFaceIndex = Stream.RandRange(0, mFaceValues.Num() - 1);
	mCurrentValue = mFaceValues[mRolledFaceIndex];
	mIsRolled = true;
	mIsUsed = false;   // 새 턴 굴림에서 이전 턴 사용 잠금을 해제.
	mIsSelected = false;   // 새 굴림이면 값이 바뀌므로 이전 스킬 빌드 선택도 무효화한다.
	return mCurrentValue;
}

/**
 * @details 예측용 깊은 복제본을 만든다. 사본에서만 Roll/계산을 돌려 라이브 주사위 상태를 보존한다.
 * Outer를 지정하지 않으면 원본과 같은 Outer에 붙여 GC 수명을 일관되게 둔다.
 */
UDiceModel* UDiceModel::Clone(UObject* InOuter) const
{
	return DuplicateObject<UDiceModel>(this, InOuter != nullptr ? InOuter : GetOuter());
}
