#include "UI/Combat/CombatUIModel.h"

// ───────── gameplay → UI : 표시값을 캐시에 넣는다 ─────────
// 화면 갱신 알림은 CombatGameMode의 OnRefresh*UI 대리자가 담당한다.

/** @brief 유닛 표시 스냅샷 캐시를 교체한다. (화면 갱신 알림은 CombatGameMode의 OnRefresh*UI 담당) */
void UCombatUIModel::SetUnitUIs(const TArray<FUnitUI>& Units)
{
	mUnitUIs = Units;
}

/** @brief 유닛 상세 스냅샷 캐시를 교체한다. */
void UCombatUIModel::SetUnitDetail(const FUnitDetailUI& Detail)
{
	mUnitDetail = Detail;
}

/** @brief 주사위 표시 스냅샷 캐시를 교체한다. */
void UCombatUIModel::SetDiceUIs(const TArray<FDiceSlotUI>& Dice)
{
	mDiceUIs = Dice;
}

/** @brief 스킬 빌드에 올린 주사위 index 목록과 합계를 교체한다. */
void UCombatUIModel::SetSelectedDice(const TArray<int32>& SelectedIndices, int32 SelectedSum)
{
	mSelectedDiceIndices = SelectedIndices;
	mSelectedDiceSum = SelectedSum;
}

/** @brief 스킬 레일 표시 스냅샷 캐시를 교체한다. */
void UCombatUIModel::SetSkillUIs(const TArray<FSkillUI>& Skills)
{
	mSkillUIs = Skills;
}

/** @brief 턴 표시 스냅샷 캐시를 교체한다. */
void UCombatUIModel::SetTurnUI(const FTurnUI& Turn)
{
	mTurnUI = Turn;
}

/** @brief 장비 슬롯 표시 스냅샷 캐시를 교체한다. (소비/표시는 탑바 담당 — 후속) */
void UCombatUIModel::SetEquipmentUIs(const TArray<FEquipmentUI>& Equipment)
{
	mEquipmentUIs = Equipment;
}

/** @brief 골드/레벨/경험치 메타 스냅샷 캐시를 교체한다. */
void UCombatUIModel::SetPlayerMeta(const FPlayerMetaUI& Meta)
{
	mPlayerMeta = Meta;
}

/** @brief 행동 결과 큐 스냅샷을 통째로 교체한다. (재생 신호는 ResolveFrontQueueNode가 발행) */
void UCombatUIModel::SetActionQueue(const TArray<FCombatQueueNode>& Queue)
{
	mActionQueue = Queue;
}

/** @brief 큐 앞 노드 하나를 제거한 뒤 해당 노드를 재생 이벤트로 내보낸다. */
void UCombatUIModel::ResolveFrontQueueNode()
{
	if (mActionQueue.Num() == 0)
	{
		return;
	}

	// 맨 앞 노드 하나를 빼고, 그 노드를 UI에 알린다(머리 위 숫자 표시 등). 연속 공격을 한 칸씩 재생한다.
	const FCombatQueueNode Resolved = mActionQueue[0];
	mActionQueue.RemoveAt(0);

	OnQueueNodeResolved.Broadcast(Resolved);
}
