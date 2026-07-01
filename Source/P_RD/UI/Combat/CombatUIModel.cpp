#include "UI/Combat/CombatUIModel.h"

/*
 * gameplay -> UI : 표시 스냅샷 저장소.
 *
 * 이 모델은 gameplay가 넣어준 표시 스냅샷을 저장하고, Set*() 직후 변경 도메인을 알린다.
 * HUD는 GameMode refresh 대리자가 아니라 이 모델을 구독해 필요한 영역만 다시 읽는다.
 *
 * 흐름:
 * ACombatGameMode::PushXxxUI()
 * -> UCombatUIModel::SetXxx()
 * -> UCombatUIModel::OnCombatUIChanged.Broadcast(Domain)
 * -> HUD/TopBar가 GetXxx()로 다시 읽어 redraw
 */

/** @brief 유닛 표시 스냅샷 캐시를 교체하고 Unit 도메인 갱신을 알린다. */
void UCombatUIModel::SetUnitUIs(const TArray<FUnitUI>& Units)
{
	mUnitUIs = Units;
	OnCombatUIChanged.Broadcast(ECombatUIDomain::Unit);
}

/** @brief 유닛 상세 스냅샷 캐시를 교체한다. */
void UCombatUIModel::SetUnitDetail(const FUnitDetailUI& Detail)
{
	mUnitDetail = Detail;
	OnCombatUIChanged.Broadcast(ECombatUIDomain::Unit);
}

/** @brief 주사위 표시 스냅샷 캐시를 교체한다. */
void UCombatUIModel::SetDiceUIs(const TArray<FDiceSlotUI>& Dice)
{
	mDiceUIs = Dice;
	OnCombatUIChanged.Broadcast(ECombatUIDomain::Dice);
}

/** @brief 스킬 빌드에 올린 주사위 index 목록과 합계를 교체한다. */
void UCombatUIModel::SetSelectedDice(const TArray<int32>& SelectedIndices, int32 SelectedSum)
{
	mSelectedDiceIndices = SelectedIndices;
	mSelectedDiceSum = SelectedSum;
	OnCombatUIChanged.Broadcast(ECombatUIDomain::SelectedDice);
}

/** @brief 스킬 레일 표시 스냅샷 캐시를 교체한다. */
void UCombatUIModel::SetSkillUIs(const TArray<FSkillUI>& Skills)
{
	mSkillUIs = Skills;
	OnCombatUIChanged.Broadcast(ECombatUIDomain::Skill);
}

/** @brief 턴 표시 스냅샷 캐시를 교체한다. */
void UCombatUIModel::SetTurnUI(const FTurnUI& Turn)
{
	mTurnUI = Turn;
	OnCombatUIChanged.Broadcast(ECombatUIDomain::Turn);
}

/** @brief 스킬 빌드 phase 스냅샷을 교체한다. */
void UCombatUIModel::SetSkillBuildPhase(ECombatBuildPhaseUI Phase)
{
	mSkillBuildPhase = Phase;
	OnCombatUIChanged.Broadcast(ECombatUIDomain::SkillBuildPhase);
}

/** @brief MOVE 빌드 phase 스냅샷을 교체한다. */
void UCombatUIModel::SetMoveBuildPhase(ECombatBuildPhaseUI Phase)
{
	mMoveBuildPhase = Phase;
	OnCombatUIChanged.Broadcast(ECombatUIDomain::MoveBuildPhase);
}

/** @brief 장비 슬롯 표시 스냅샷 캐시를 교체한다. (소비/표시는 탑바 담당 — 후속) */
void UCombatUIModel::SetEquipmentUIs(const TArray<FEquipmentUI>& Equipment)
{
	mEquipmentUIs = Equipment;
	OnCombatUIChanged.Broadcast(ECombatUIDomain::Equipment);
}

/** @brief 골드/레벨/경험치 메타 스냅샷 캐시를 교체한다. */
void UCombatUIModel::SetPlayerMeta(const FPlayerMetaUI& Meta)
{
	mPlayerMeta = Meta;
	OnCombatUIChanged.Broadcast(ECombatUIDomain::Meta);
}

/** @brief 행동 결과 큐 스냅샷을 통째로 교체한다. (재생 신호는 ResolveFrontQueueNode가 발행) */
void UCombatUIModel::SetActionQueue(const TArray<FCombatQueueNode>& Queue)
{
	mActionQueue = Queue;
	OnCombatUIChanged.Broadcast(ECombatUIDomain::Queue);
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

/** @brief 주사위 굴림판 표시 요청을 HUD에 전달한다. */
void UCombatUIModel::NotifyDiceRollPresentationRequested(const USRPGTurnContext* TurnContext)
{
	OnDiceRollPresentationRequested.Broadcast(TurnContext);
}

/** @brief 월드 타겟 상세 패널 표시 요청을 HUD에 전달한다. */
void UCombatUIModel::NotifyTargetDetailPanelRequested(IBoardSelectionTarget* Target)
{
	OnTargetDetailPanelRequested.Broadcast(Target);
}
