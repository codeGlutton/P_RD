#include "Dice/DicePoolModel.h"

#include "DataAsset/DiceData/StaticDiceData.h"
#include "Dice/DiceModel.h"
#include "Engine/AssetManager.h"

namespace
{
	// 보유 주사위 id로 정적 데이터를 얻는다. 이미 로드돼 있으면 그걸, 아니면 PrimaryAsset 경로로 동기 로드.
	// 전투 진입 시 런 보유 주사위를 구성하는 좁은 경로라 동기 로드를 허용한다.
	const UStaticDiceData* LoadStaticDiceData(const FPrimaryAssetId& DiceId)
	{
		if (DiceId.IsValid() == false)
		{
			return nullptr;
		}

		UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
		if (AssetManager == nullptr)
		{
			return nullptr;
		}

		if (const UStaticDiceData* Loaded = AssetManager->GetPrimaryAssetObject<UStaticDiceData>(DiceId))
		{
			return Loaded;
		}

		// 아직 메모리에 없으면 경로로 동기 로드(전투 진입 시 1회).
		const FSoftObjectPath AssetPath = AssetManager->GetPrimaryAssetPath(DiceId);
		return Cast<UStaticDiceData>(AssetPath.TryLoad());
	}
}


/** @brief 런 보유 DiceId 목록을 UDiceModel 런타임 인스턴스로 변환한다. */
void UDicePoolModel::BuildFromDiceIds(const TArray<FPrimaryAssetId>& DiceIds)
{
	mDice.Reset();
	mDice.Reserve(DiceIds.Num());

	/*
	 * 보유 주사위는 전부 데이터에서 가져온다 — 직업 mDiceDatas → 런 mDiceIds로 채워진 그대로.
	 * 개수·면 수·면값·희귀도 모두 각 UStaticDiceData가 정의한다(코드가 종류를 만들지 않는다).
	 * 정적 데이터를 못 찾으면 InitFromStatic 내부가 기본 d6로 폴백한다.
	 */
	for (const FPrimaryAssetId& DiceId : DiceIds)
	{
		const UStaticDiceData* StaticDiceData = LoadStaticDiceData(DiceId);

		UDiceModel* Dice = NewObject<UDiceModel>(this);
		Dice->InitFromStatic(StaticDiceData, DiceId);
		mDice.Add(Dice);
	}
}

/** @brief 보유 주사위를 모두 같은 난수 스트림 흐름으로 굴린다. */
void UDicePoolModel::RollAll(const FRandomStream& Stream)
{
	for (const TObjectPtr<UDiceModel>& Dice : mDice)
	{
		if (Dice != nullptr)
		{
			Dice->Roll(Stream);
		}
	}
}

/** @brief 확정된 스킬에 사용한 주사위를 이번 턴 잠금 상태로 표시한다. */
void UDicePoolModel::MarkDiceUsed(int32 DiceIndex)
{
	if (mDice.IsValidIndex(DiceIndex) && mDice[DiceIndex] != nullptr)
	{
		mDice[DiceIndex]->SetUsed(true);
	}
}

/** @brief 턴 종료/새 턴 시작 시 모든 주사위 사용 잠금을 해제한다. */
void UDicePoolModel::ResetUsed()
{
	for (const TObjectPtr<UDiceModel>& Dice : mDice)
	{
		if (Dice != nullptr)
		{
			Dice->SetUsed(false);
		}
	}
}

/** @brief 어댑터가 index 기반 UI payload를 굴림 결과로 되돌릴 때 쓰는 읽기 API다. */
int32 UDicePoolModel::GetRolledDiceValue(int32 DiceIndex) const
{
	if (mDice.IsValidIndex(DiceIndex) == false || mDice[DiceIndex] == nullptr)
	{
		return 0;
	}
	const UDiceModel* Dice = mDice[DiceIndex];
	return Dice->IsRolled() ? Dice->GetCurrentValue() : 0;
}

FPrimaryAssetId UDicePoolModel::GetDiceId(int32 DiceIndex) const
{
	if (mDice.IsValidIndex(DiceIndex) == false || mDice[DiceIndex] == nullptr)
	{
		return FPrimaryAssetId();
	}
	return mDice[DiceIndex]->GetSourceDiceId();
}
