/*****************************************************************//**
 * @file   SRPGSkillAction.h
 * @brief  스킬에 대한 SRPG 행동 객체 구현 헤더
 * @author 모호재
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGAction.h"

struct FPresentationBarrier;
struct FSRPGSkillAction;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBeginSkillUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const FSRPGSkillAction& /*Action*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEndSkillUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const FSRPGSkillAction& /*Action*/);

class UStaticSkillData;

/**
 * @brief  스킬 사용 SRPG 행동을 제작하기 위해 요구되는 절차 처리 일회성 객체
 */
struct FSRPGSkillActionDraft : public FSRPGActionDraft
{
	friend struct FSRPGTurnContext;
	using Super = FSRPGActionDraft;

protected:
	TSharedPtr<FSRPGAction> FinalizeDraft() const override;

	void OnFinalizeDraft() override;
	void OnDiscardDraft() override;

protected:
	ESRPGActionDraftType GetDraftType() const override;

public:
	void SetSkill(int32 SkillIndex);
	void SetTargetTile(const FTileIndex& TileIndex);
	FTileIndex FindTargetTileUnderCursor() const;

public:
	FOnBeginSkillUI OnBeginSkillUI;
	FOnEndSkillUI OnEndSkillUI;

protected:
	// @brief 현재 스킬 액션 초안 상태
	ESRPGSkillActionDraftPhase mPhase = ESRPGSkillActionDraftPhase::None;

protected:
	TObjectPtr<UStaticSkillData> mSelectedSkill;
};

/**
 * @brief  사용자 입력에 따른 정해진 SRPG 행동 객체
 */
struct FSRPGSkillAction : public FSRPGAction
{
	friend struct FSRPGTurnContext;
	friend struct FSRPGSkillActionDraft;
	using Super = FSRPGAction;

protected:
	FSRPGSkillAction() = default;
	virtual ~FSRPGSkillAction() = default;

protected:
	void BeginAction() override;
	void TickAction(float DeltaTime) override;
	void EndAction() override;

protected:
	bool IsTurnEndingAction() const override;
};

