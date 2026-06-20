#include "UI/Combat/CombatUIModel.h"

// ───────── UI → gameplay : 의도만 브로드캐스트 (실행은 게임플레이가) ─────────

/** @brief 스킬 레일 index 선택 의도를 게임플레이 구독자에게 전달한다. */
void UCombatUIModel::RequestSelectSkill(int32 SkillIndex)
{
	OnCombatCommand.Broadcast(ECombatInputType::SelectSkill, SkillIndex);
}

/** @brief 주사위 슬롯 index 토글 의도를 전달한다. */
void UCombatUIModel::RequestToggleDice(int32 DiceIndex)
{
	OnCombatCommand.Broadcast(ECombatInputType::ToggleDice, DiceIndex);
}

/** @brief 보유 주사위 전체 굴림 의도를 전달한다. */
void UCombatUIModel::RequestRollDice()
{
	OnCombatCommand.Broadcast(ECombatInputType::RollDice, INDEX_NONE);
}

/** @brief 스킬 상세 요청을 SkillIndex payload로 전달한다. */
void UCombatUIModel::RequestLongPressSkill(int32 SkillIndex)
{
	OnCombatCommand.Broadcast(ECombatInputType::LongPressSkill, SkillIndex);
}

/** @brief 유닛 상세 요청을 UnitId payload로 전달한다. */
void UCombatUIModel::RequestLongPressUnit(int32 UnitId)
{
	OnCombatCommand.Broadcast(ECombatInputType::LongPressUnit, UnitId);
}

/** @brief MOVE 모드 진입 의도를 전달한다. */
void UCombatUIModel::RequestMove()
{
	OnCombatCommand.Broadcast(ECombatInputType::Move, INDEX_NONE);
}

/** @brief 턴 종료 의도를 전달한다. */
void UCombatUIModel::RequestEndTurn()
{
	OnCombatCommand.Broadcast(ECombatInputType::EndTurn, INDEX_NONE);
}

/** @brief 현재 선택/빌드 취소 의도를 전달한다. */
void UCombatUIModel::RequestCancel()
{
	OnCombatCommand.Broadcast(ECombatInputType::Cancel, INDEX_NONE);
}

/** @brief 장비 슬롯 상세 요청을 SlotIndex payload로 전달한다. */
void UCombatUIModel::RequestLongPressEquip(int32 SlotIndex)
{
	OnCombatCommand.Broadcast(ECombatInputType::LongPressEquip, SlotIndex);
}

/** @brief 월드 터치 스크린 좌표를 변환하지 않고 그대로 게임플레이 경계로 넘긴다. */
void UCombatUIModel::RequestWorldTouch(FVector2D ScreenPosition, bool bLongPress)
{
	OnCombatWorldTouch.Broadcast(ScreenPosition, bLongPress);
}

// ───────── gameplay → UI : 표시값을 캐시에 넣고 도메인 갱신을 알린다 ─────────

/** @brief 유닛 표시 스냅샷을 교체하고 Unit 도메인 갱신만 알린다. */
void UCombatUIModel::SetUnitUIs(const TArray<FUnitUI>& Units)
{
	mUnitUIs = Units;
	OnUIChanged.Broadcast(ECombatUIDomain::Unit);
}

/** @brief 유닛 상세 스냅샷을 교체하고 Unit 도메인 갱신을 알린다. */
void UCombatUIModel::SetUnitDetail(const FUnitDetailUI& Detail)
{
	mUnitDetail = Detail;
	OnUIChanged.Broadcast(ECombatUIDomain::Unit);
}

/** @brief 주사위 표시 스냅샷을 교체하고 Dice 도메인 갱신을 알린다. */
void UCombatUIModel::SetDiceUIs(const TArray<FDiceSlotUI>& Dice)
{
	mDiceUIs = Dice;
	OnUIChanged.Broadcast(ECombatUIDomain::Dice);
}

/** @brief 스킬 빌드에 올린 주사위 index 목록과 합계를 교체한다. */
void UCombatUIModel::SetSelectedDice(const TArray<int32>& SelectedIndices, int32 SelectedSum)
{
	mSelectedDiceIndices = SelectedIndices;
	mSelectedDiceSum = SelectedSum;

	// 올린 주사위의 id/눈금값을 모아 함께 알린다(UI·액션이 index→데이터 매핑 없이 바로 쓰게).
	// mDiceUIs는 직전 SetDiceUIs로 갱신돼 있어야 한다(어댑터가 SetDiceUIs→SetSelectedDice 순으로 호출).
	TArray<FPrimaryAssetId> SelectedIds;
	TArray<int32> SelectedValues;
	SelectedIds.Reserve(SelectedIndices.Num());
	SelectedValues.Reserve(SelectedIndices.Num());
	for (int32 Index : SelectedIndices)
	{
		if (mDiceUIs.IsValidIndex(Index))
		{
			SelectedIds.Add(mDiceUIs[Index].mDiceId);
			SelectedValues.Add(mDiceUIs[Index].mResultValue);
		}
	}
	OnDiceSelectionChanged.Broadcast(SelectedIds, SelectedValues);

	OnUIChanged.Broadcast(ECombatUIDomain::Dice);
}

/** @brief 스킬 레일 표시 스냅샷을 교체하고 Skill 도메인을 갱신한다. */
void UCombatUIModel::SetSkillUIs(const TArray<FSkillUI>& Skills)
{
	mSkillUIs = Skills;
	OnUIChanged.Broadcast(ECombatUIDomain::Skill);
}

/** @brief 스킬 상세 스냅샷을 교체하고 Skill 도메인을 갱신한다. */
void UCombatUIModel::SetSkillDetail(const FSkillDetailUI& Detail)
{
	mSkillDetail = Detail;
	OnUIChanged.Broadcast(ECombatUIDomain::Skill);
}


/** @brief 턴 표시 스냅샷을 교체하고 Turn 도메인을 갱신한다. */
void UCombatUIModel::SetTurnUI(const FTurnUI& Turn)
{
	mTurnUI = Turn;
	OnUIChanged.Broadcast(ECombatUIDomain::Turn);
}

/** @brief 장비 슬롯 표시 스냅샷을 교체하고 Equipment 도메인을 갱신한다. */
void UCombatUIModel::SetEquipmentUIs(const TArray<FEquipmentUI>& Equipment)
{
	mEquipmentUIs = Equipment;
	OnUIChanged.Broadcast(ECombatUIDomain::Equipment);
}

/** @brief 골드/레벨/경험치 메타 스냅샷을 교체하고 Meta 도메인을 갱신한다. */
void UCombatUIModel::SetPlayerMeta(const FPlayerMetaUI& Meta)
{
	mPlayerMeta = Meta;
	OnUIChanged.Broadcast(ECombatUIDomain::Meta);
}

/** @brief 행동 결과 큐를 통째로 교체하고 Queue 도메인 갱신을 알린다. */
void UCombatUIModel::SetActionQueue(const TArray<FCombatQueueNode>& Queue)
{
	mActionQueue = Queue;
	OnUIChanged.Broadcast(ECombatUIDomain::Queue);
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
	OnUIChanged.Broadcast(ECombatUIDomain::Queue);
}

/** @brief 액션 빌드가 끝났음을 구독 위젯에 알려 선택 강조를 정리하게 한다. */
void UCombatUIModel::NotifyActionResolved()
{
	OnActionResolved.Broadcast();
}
