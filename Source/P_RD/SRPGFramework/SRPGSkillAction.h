/*****************************************************************//**
 * @file   SRPGSkillAction.h
 * @brief  스킬에 대한 SRPG 행동 객체 구현 헤더
 * @author 모호재
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGAction.h"

struct FSRPGSkillAction;

struct FSRPGSkillCastCommand : public FSRPGActionCreationCommand<FSRPGSkillAction>
{
public:
	FSRPGSkillCastCommand();

public:
	// TArray<FSRPGCalculatedData> mCalculateDatas;
};

/**
 * @brief  사용자 입력에 따른 정해진 SRPG 행동 객체
 */
struct FSRPGSkillAction : public FSRPGAction
{
	template<typename ActionType>
	friend struct FSRPGActionCreationCommand;
	using Super = FSRPGAction;

protected:
	FSRPGSkillAction();

protected:
	void OnBeginAction() override;
	void OnTickAction(float DeltaTime) override;
	void OnEndAction() override;

protected:
	// TArray<FSRPGCalculatedData> mCalculateDatas;
};

