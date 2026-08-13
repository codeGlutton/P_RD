#include "UI/Combat/CombatUIModel.h"

// ───────── UI → gameplay : 의도만 브로드캐스트 (실행은 게임플레이가) ─────────

/** @brief 스킬 레일 index 선택 의도를 게임플레이 구독자에게 전달한다. */
void UCombatUIModel::RequestSelectSkill(int32 SkillIndex)
{
	OnCombatCommand.Broadcast(ECombatInputType::SelectSkill, SkillIndex);
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

/** @brief 상세창에 뜬 유닛의 스킬 상세 요청을 SkillIndex payload로 전달한다. */
void UCombatUIModel::RequestInspectUnitSkill(int32 SkillIndex)
{
	OnCombatCommand.Broadcast(ECombatInputType::InspectUnitSkill, SkillIndex);
}

/** @brief 그 유닛을 화면 가운데로 데려오라고 청한다. */
void UCombatUIModel::RequestFocusUnit(int32 UnitId)
{
	OnCombatCommand.Broadcast(ECombatInputType::FocusUnit, UnitId);
}

void UCombatUIModel::RequestFocusUnitInstant(int32 UnitId)
{
	OnCombatCommand.Broadcast(ECombatInputType::FocusUnitInstant, UnitId);
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
void UCombatUIModel::RequestInspectUnit(int32 UnitId)
{
	OnCombatCommand.Broadcast(ECombatInputType::InspectUnit, UnitId);
}

void UCombatUIModel::RequestCancel()
{
	OnCombatCommand.Broadcast(ECombatInputType::Cancel, INDEX_NONE);
}

/** @brief 겨냥해 둔 칸을 확정한다. */
void UCombatUIModel::RequestConfirm()
{
	OnCombatCommand.Broadcast(ECombatInputType::Confirm, INDEX_NONE);
}

/** @brief 장비 슬롯 상세 요청을 SlotIndex payload로 전달한다. */
void UCombatUIModel::RequestLongPressEquip(int32 SlotIndex)
{
	OnCombatCommand.Broadcast(ECombatInputType::LongPressEquip, SlotIndex);
}

/** @brief 월드 터치 스크린 좌표를 변환하지 않고 그대로 게임플레이 경계로 넘긴다. */
/**
 * @brief 겨냥한 자리를 갈아 끼운다.
 * @param Target 새로 겨냥한 자리. mIsValid 가 false 면 겨냥을 푼다
 */
void UCombatUIModel::SetTarget(const FCombatTargetUI& Target)
{
	if (mTarget.mIsValid == Target.mIsValid
		&& mTarget.mTile == Target.mTile
		&& mTarget.mUnitId == Target.mUnitId)
	{
		return;
	}
	mTarget = Target;
	OnUIChanged.Broadcast(ECombatUIDomain::Unit);
}

void UCombatUIModel::SetFocusScreenAnchor(FVector2D AnchorFraction)
{
	OnChangeFocusScreenAnchor.Broadcast(AnchorFraction);
}

void UCombatUIModel::RequestWorldTouch(FVector2D ScreenPosition, bool bLongPress)
{
	OnCombatWorldTouch.Broadcast(ScreenPosition, bLongPress);
}

void UCombatUIModel::RequestAbandonRun()
{
	OnAbandonRun.Broadcast();
}

void UCombatUIModel::RequestSaveAndExitRun()
{
	OnSaveAndExitRun.Broadcast();
}

void UCombatUIModel::NotifySaveAndExitCompleted(const bool bSuccess)
{
	OnSaveAndExitCompleted.Broadcast(bSuccess);
}

void UCombatUIModel::NotifyAbandonRunCompleted(const bool bSuccess)
{
	OnAbandonRunCompleted.Broadcast(bSuccess);
}

// ───────── gameplay → UI : 표시값을 캐시에 넣고 도메인 갱신을 알린다 ─────────

void UCombatUIModel::SetCombatResultUI(const FCombatResultUI& Result)
{
	mCombatResultUI = Result;
}

/** @brief 유닛 표시 스냅샷을 교체하고 Unit 도메인 갱신만 알린다. */
void UCombatUIModel::SetUnitUIs(const TArray<FUnitUI>& Units)
{
	mUnitUIs = Units;
	OnUIChanged.Broadcast(ECombatUIDomain::Unit);
}

/** @brief 유닛 상세 스냅샷을 교체하고 UnitDetail 도메인 갱신을 알린다. */
// Unit 도메인은 HP 변경마다 오므로, 상세는 전용 도메인으로 알려야 상세 패널이
// "이번에 새로 요청된 상세"에만 반응할 수 있다.
void UCombatUIModel::SetUnitDetail(const FUnitDetailUI& Detail)
{
	mUnitDetail = Detail;
	OnUIChanged.Broadcast(ECombatUIDomain::UnitDetail);
}

/** @brief 스킬 레일 표시 스냅샷을 교체하고 Skill 도메인을 갱신한다. */
void UCombatUIModel::SetSkillUIs(const TArray<FSkillUI>& Skills)
{
	mSkillUIs = Skills;
	OnUIChanged.Broadcast(ECombatUIDomain::Skill);
}

/** @brief 선택된 스킬 index를 교체한다. */
void UCombatUIModel::SetSelectedSkill(int32 SelectedIndex)
{
	mSelectedSkillIndex = SelectedIndex;
	OnUIChanged.Broadcast(ECombatUIDomain::Skill);
}

/** @brief 스킬 상세 스냅샷을 교체하고 SkillDetail 도메인을 갱신한다. */
void UCombatUIModel::SetSkillDetail(const FSkillDetailUI& Detail)
{
	mSkillDetail = Detail;
	OnUIChanged.Broadcast(ECombatUIDomain::SkillDetail);
}


/** @brief 턴 표시 스냅샷을 교체하고 Turn 도메인을 갱신한다. */
void UCombatUIModel::SetTurnUI(const FTurnUI& Turn)
{
	mTurnUI = Turn;
	OnUIChanged.Broadcast(ECombatUIDomain::Turn);
}

/** @brief 조작 빌드 페이즈만 바꾸고 Turn 도메인을 갱신한다(스킬/이동 빌드 공용). */
void UCombatUIModel::SetBuildPhase(ECombatBuildPhaseUI Phase)
{
	mTurnUI.mPhase = Phase;
	OnUIChanged.Broadcast(ECombatUIDomain::Turn);
}

/** @brief 이동 경로 또는 선택 스킬이 확정 전에 소비할 AP를 캐시한다. */
void UCombatUIModel::SetPendingAction(const FCombatPendingActionUI& PendingAction)
{
	mPendingAction = PendingAction;
	if (mPendingAction.mType == ECombatPendingActionType::None)
	{
		mPendingAction.mActionPointCost = 0;
	}
	else
	{
		mPendingAction.mActionPointCost = FMath::Max(mPendingAction.mActionPointCost, 0);
	}
	OnUIChanged.Broadcast(ECombatUIDomain::Turn);
}

/** @brief 현재 유닛 스냅샷의 실제 이동 AP를 화면용 정수로 정규화한다. */
int32 UCombatUIModel::GetDisplayedMovementPoint(const FUnitUI& Unit) const
{
	return FMath::Max(FMath::RoundToInt(Unit.mMovementPoint), 0);
}

/** @brief 장비 슬롯 표시 스냅샷을 교체하고 Equipment 도메인을 갱신한다. */
void UCombatUIModel::SetEquipmentUIs(const TArray<FEquipmentUI>& Equipment)
{
	mEquipmentUIs = Equipment;
	OnUIChanged.Broadcast(ECombatUIDomain::Equipment);
}

/** @brief 장비 롱프레스 상세 스냅샷을 교체하고 Equipment 도메인을 갱신한다. */
void UCombatUIModel::SetEquipmentDetail(const FEquipmentDetailUI& Detail)
{
	mEquipmentDetail = Detail;
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

/** @brief 월드 위치 기준 플로팅 로그 한 건을 구독 위젯에 알린다. */
void UCombatUIModel::NotifyCombatFloatingLog(const FCombatFloatingLogRequest& Request)
{
	OnCombatFloatingLog.Broadcast(Request);
}

/** @brief 월드 위치 기준 플로팅 로그 여러 건을 구독 위젯에 알린다. */
void UCombatUIModel::NotifyCombatFloatingLogs(const TArray<FCombatFloatingLogRequest>& Requests)
{
	struct FIndexedFloatingLogRequest
	{
		const FCombatFloatingLogRequest* Request = nullptr;
		int32 InputIndex = 0;
	};

	TArray<FIndexedFloatingLogRequest> SortedRequests;
	SortedRequests.Reserve(Requests.Num());
	for (int32 RequestIndex = 0; RequestIndex < Requests.Num(); ++RequestIndex)
	{
		SortedRequests.Add(FIndexedFloatingLogRequest{ &Requests[RequestIndex], RequestIndex });
	}
	SortedRequests.Sort([](const FIndexedFloatingLogRequest& Lhs, const FIndexedFloatingLogRequest& Rhs) {
		if (Lhs.Request->mSequence != Rhs.Request->mSequence)
		{
			return Lhs.Request->mSequence < Rhs.Request->mSequence;
		}
		return Lhs.InputIndex < Rhs.InputIndex;
		});

	for (const FIndexedFloatingLogRequest& SortedRequest : SortedRequests)
	{
		if (SortedRequest.Request != nullptr)
		{
			NotifyCombatFloatingLog(*SortedRequest.Request);
		}
	}
}

/** @brief 특정 모션 인덱스에 묶인 플로팅 로그를 정리하라고 구독 위젯에 알린다. */
void UCombatUIModel::NotifyCombatFloatingLogMotionFinished(int32 MotionIndex)
{
	if (MotionIndex == INDEX_NONE)
	{
		return;
	}

	OnCombatFloatingLogMotionFinished.Broadcast(MotionIndex);
}

/** @brief 현재/대기 중인 플로팅 로그를 전부 지우라고 구독 위젯에 알린다. */
void UCombatUIModel::NotifyCombatFloatingLogsCleared()
{
	OnCombatFloatingLogsCleared.Broadcast();
}

void UCombatUIModel::NotifyCombatResultOpenRequested()
{
	OnCombatResultOpenRequested.Broadcast();
}
