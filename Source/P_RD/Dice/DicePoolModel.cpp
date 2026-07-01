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
	for (const TObjectPtr<UDiceModel>& Dice : mDices)
	{
		if (Dice != nullptr)
		{
			Dice->Roll(Stream);
		}
	}

	OnRollAllDicesUI.Broadcast(mDices);
}

/** @brief 지정 index 주사위를 스킬 빌드 선택 상태로 표시하고 UI에 알린다. */
void UDicePoolModel::MarkDiceSelected(int32 DiceIndex)
{
	if (mDices.IsValidIndex(DiceIndex) && mDices[DiceIndex] != nullptr)
	{
		mDices[DiceIndex]->SetSelected(true);

		OnSelectedDiceUI.Broadcast(mDices[DiceIndex]);
	}
}

/** @brief 지정 index 주사위의 선택을 해제하고 UI에 알린다. */
void UDicePoolModel::MarkDiceUnselected(int32 DiceIndex)
{
	if (mDices.IsValidIndex(DiceIndex) && mDices[DiceIndex] != nullptr)
	{
		mDices[DiceIndex]->SetSelected(false);

		OnUnselectedDiceUI.Broadcast(mDices[DiceIndex]);
	}
}

/** @brief 스킬 취소 등으로 모든 주사위 선택을 해제한다(선택돼 있던 것만 UI에 알림). */
void UDicePoolModel::ResetSelected()
{
	for (const TObjectPtr<UDiceModel>& Dice : mDices)
	{
		if (Dice != nullptr && Dice->IsSelected())
		{
			Dice->SetSelected(false);

			OnUnselectedDiceUI.Broadcast(Dice);
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

/**
 * @brief 확정된 스킬에 올린(선택된) 주사위들을 이번 턴 사용됨으로 잠근다.
 * @details 선택 표시는 여기서 지우지 않는다 — 호출자(BuildSkill)가 직후 GetSelectedDiceSum()으로
 *          확정 주사위 합을 다시 읽기 때문이다. 선택 해제는 ResetSelected()가 담당한다.
 */
void UDicePoolModel::MarkSelectedDiceAsUsed()
{
	for (const TObjectPtr<UDiceModel>& Dice : mDices)
	{
		if (Dice != nullptr && Dice->IsSelected())
		{
			Dice->SetUsed(true);

			OnUseDiceUI.Broadcast(Dice);
		}
	}
}

/** @brief 턴 종료/새 턴 시작 시 모든 주사위 사용 잠금을 해제한다. */
void UDicePoolModel::ResetUsed()
{
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

/** @brief 해당 index 주사위가 현재 스킬 빌드에 선택돼 있는지. */
bool UDicePoolModel::IsSelectedDice(int32 DiceIndex) const
{
	if (mDices.IsValidIndex(DiceIndex) == false || mDices[DiceIndex] == nullptr)
	{
		return false;
	}
	return mDices[DiceIndex]->IsSelected();
}

/** @brief 현재 선택된 주사위 개수(필요 주사위 수 게이트/상한 판정용). */
int32 UDicePoolModel::GetSelectedDiceNum() const
{
	int32 SelectedNum = 0;
	for (const TObjectPtr<UDiceModel>& Dice : mDices)
	{
		if (Dice != nullptr && Dice->IsSelected())
		{
			++SelectedNum;
		}
	}
	return SelectedNum;
}

/** @brief 현재 선택된 주사위들의 굴림값 합(스킬 dice point/사거리/범위 계산용). */
int32 UDicePoolModel::GetSelectedDiceSum() const
{
	int32 SelectedSum = 0;
	for (const TObjectPtr<UDiceModel>& Dice : mDices)
	{
		if (Dice != nullptr && Dice->IsSelected())
		{
			SelectedSum += Dice->GetCurrentValue();
		}
	}
	return SelectedSum;
}
