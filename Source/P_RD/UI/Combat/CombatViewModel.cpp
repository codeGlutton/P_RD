#include "UI/Combat/CombatViewModel.h"

// ───────── UI → gameplay : 의도만 브로드캐스트 (실행은 게임플레이가) ─────────

void UCombatViewModel::RequestSelectSkill(int32 SkillIndex)
{
	OnCombatCommand.Broadcast(ECombatInputType::SelectSkill, SkillIndex);
}

void UCombatViewModel::RequestToggleDice(int32 DiceIndex)
{
	OnCombatCommand.Broadcast(ECombatInputType::ToggleDice, DiceIndex);
}

void UCombatViewModel::RequestRollDice()
{
	OnCombatCommand.Broadcast(ECombatInputType::RollDice, INDEX_NONE);
}

void UCombatViewModel::RequestLongPressSkill(int32 SkillIndex)
{
	OnCombatCommand.Broadcast(ECombatInputType::LongPressSkill, SkillIndex);
}

void UCombatViewModel::RequestLongPressUnit(int32 UnitId)
{
	OnCombatCommand.Broadcast(ECombatInputType::LongPressUnit, UnitId);
}

void UCombatViewModel::RequestMove()
{
	OnCombatCommand.Broadcast(ECombatInputType::Move, INDEX_NONE);
}

void UCombatViewModel::RequestEndTurn()
{
	OnCombatCommand.Broadcast(ECombatInputType::EndTurn, INDEX_NONE);
}

void UCombatViewModel::RequestCancel()
{
	OnCombatCommand.Broadcast(ECombatInputType::Cancel, INDEX_NONE);
}

void UCombatViewModel::RequestWorldTouch(FVector2D ScreenPosition, bool bLongPress)
{
	OnCombatWorldTouch.Broadcast(ScreenPosition, bLongPress);
}

// ───────── gameplay → UI : 표시값을 캐시에 넣고 도메인 갱신을 알린다 ─────────

void UCombatViewModel::SetUnitViews(const TArray<FUnitView>& Units)
{
	mUnitViews = Units;
	OnViewChanged.Broadcast(ECombatViewDomain::Unit);
}

void UCombatViewModel::SetUnitDetail(const FUnitDetailView& Detail)
{
	mUnitDetail = Detail;
	OnViewChanged.Broadcast(ECombatViewDomain::Unit);
}

void UCombatViewModel::SetDiceViews(const TArray<FDiceSlotView>& Dice)
{
	mDiceViews = Dice;
	OnViewChanged.Broadcast(ECombatViewDomain::Dice);
}

void UCombatViewModel::SetSelectedDice(const TArray<int32>& SelectedIndices, int32 SelectedSum)
{
	mSelectedDiceIndices = SelectedIndices;
	mSelectedDiceSum = SelectedSum;
	OnViewChanged.Broadcast(ECombatViewDomain::Dice);
}

void UCombatViewModel::SetSkillViews(const TArray<FSkillView>& Skills)
{
	mSkillViews = Skills;
	OnViewChanged.Broadcast(ECombatViewDomain::Skill);
}

void UCombatViewModel::SetSkillDetail(const FSkillDetailView& Detail)
{
	mSkillDetail = Detail;
	OnViewChanged.Broadcast(ECombatViewDomain::Skill);
}

void UCombatViewModel::SetTileStates(const TArray<FTileViewState>& Tiles)
{
	mTileStates = Tiles;
	OnViewChanged.Broadcast(ECombatViewDomain::Tile);
}

void UCombatViewModel::SetTurnView(const FTurnView& Turn)
{
	mTurnView = Turn;
	OnViewChanged.Broadcast(ECombatViewDomain::Turn);
}

void UCombatViewModel::SetActionQueue(const TArray<FCombatQueueNode>& Queue)
{
	mActionQueue = Queue;
	OnViewChanged.Broadcast(ECombatViewDomain::Queue);
}

void UCombatViewModel::ResolveFrontQueueNode()
{
	if (mActionQueue.Num() == 0)
	{
		return;
	}

	// 맨 앞 노드 하나를 빼고, 그 노드를 UI에 알린다(머리 위 숫자 표시 등). 연속 공격을 한 칸씩 재생한다.
	const FCombatQueueNode Resolved = mActionQueue[0];
	mActionQueue.RemoveAt(0);

	OnQueueNodeResolved.Broadcast(Resolved);
	OnViewChanged.Broadcast(ECombatViewDomain::Queue);
}
