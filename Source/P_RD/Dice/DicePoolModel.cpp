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
	mDices.Reset();
	mSelectedDiceIndices.Reset();
	mDices.Reserve(DiceIds.Num());

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
		mDices.Add(Dice);
	}
}

/** @brief 보유 주사위를 모두 같은 난수 스트림 흐름으로 굴린다. */
void UDicePoolModel::RollAll(const FRandomStream& Stream)
{
	mSelectedDiceIndices.Reset();

	for (const TObjectPtr<UDiceModel>& Dice : mDices)
	{
		if (Dice != nullptr)
		{
			Dice->Roll(Stream);
		}
	}

	OnRollAllDicesUI.Broadcast(mDices);
}

void UDicePoolModel::MarkDiceSelected(int32 DiceIndex)
{
	if (mDices.IsValidIndex(DiceIndex) == false || mDices[DiceIndex] == nullptr)
	{
		return;
	}

	if (mSelectedDiceIndices.Contains(DiceIndex))
	{
		return;
	}

	if (mDices[DiceIndex]->IsUsed())
	{
		return;
	}

	mSelectedDiceIndices.Add(DiceIndex);
	OnSelectedDiceUI.Broadcast(mDices[DiceIndex]);
}

void UDicePoolModel::MarkDiceUnselected(int32 DiceIndex)
{
	if (mDices.IsValidIndex(DiceIndex) == false || mDices[DiceIndex] == nullptr)
	{
		return;
	}

	if (mSelectedDiceIndices.Remove(DiceIndex) > 0)
	{
		OnUnselectedDiceUI.Broadcast(mDices[DiceIndex]);
	}
}

void UDicePoolModel::ResetSelected()
{
	TArray<int32> SelectedDiceIndices;
	SelectedDiceIndices = mSelectedDiceIndices.Array();

	mSelectedDiceIndices.Reset();

	for (int32 DiceIndex : SelectedDiceIndices)
	{
		if (mDices.IsValidIndex(DiceIndex) && mDices[DiceIndex] != nullptr)
		{
			OnUnselectedDiceUI.Broadcast(mDices[DiceIndex]);
		}
	}
}

/** @brief 확정된 스킬에 사용한 주사위를 이번 턴 잠금 상태로 표시한다. */
void UDicePoolModel::MarkDiceUsed(int32 DiceIndex)
{
	if (mDices.IsValidIndex(DiceIndex) && mDices[DiceIndex] != nullptr)
	{
		mDices[DiceIndex]->SetUsed(true);

		OnUseDiceUI.Broadcast(mDices[DiceIndex]);
	}
}

void UDicePoolModel::MarkSelectedDiceAsUsed()
{
	TArray<int32> SelectedDiceIndices;
	SelectedDiceIndices = mSelectedDiceIndices.Array();

	for (int32 DiceIndex : SelectedDiceIndices)
	{
		MarkDiceUsed(DiceIndex);
	}

	ResetSelected();
}

/** @brief 턴 종료/새 턴 시작 시 모든 주사위 사용 잠금을 해제한다. */
void UDicePoolModel::ResetUsed()
{
	ResetSelected();

	for (const TObjectPtr<UDiceModel>& Dice : mDices)
	{
		if (Dice != nullptr)
		{
			Dice->SetUsed(false);
		}
	}

	OnResetAllDiceUI.Broadcast(mDices);
}

/** @brief 어댑터가 index 기반 UI payload를 굴림 결과로 되돌릴 때 쓰는 읽기 API다. */
int32 UDicePoolModel::GetRolledDiceValue(int32 DiceIndex) const
{
	if (mDices.IsValidIndex(DiceIndex) == false || mDices[DiceIndex] == nullptr)
	{
		return 0;
	}
	const UDiceModel* Dice = mDices[DiceIndex];
	return Dice->IsRolled() ? Dice->GetCurrentValue() : 0;
}

bool UDicePoolModel::IsSelectedDice(int32 DiceIndex) const
{
	return mSelectedDiceIndices.Contains(DiceIndex);
}

int32 UDicePoolModel::GetSelectedDiceNum() const
{
	return mSelectedDiceIndices.Num();
}

int32 UDicePoolModel::GetSelectedDiceSum() const
{
	int32 SelectedDiceSum = 0;
	for (int32 DiceIndex : mSelectedDiceIndices)
	{
		SelectedDiceSum += GetRolledDiceValue(DiceIndex);
	}
	return SelectedDiceSum;
}
