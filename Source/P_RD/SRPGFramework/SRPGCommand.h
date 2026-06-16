/*****************************************************************//**
 * @file   SRPGCommand.h
 * @brief  SRPG의 사용자 명령 객체 구현 헤더
 * @author 모호재
 * @date   2026-06-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"

struct FSRPGAction;

/**
 * @brief  사용자 입력 명령 객체
 */
struct FSRPGCommand : TSharedFromThis<FSRPGCommand>
{
public:
	virtual ~FSRPGCommand() = default;

public:
	ESRPGCommandType GetCommandType() const;
	virtual TSharedPtr<FSRPGAction> CreateAction() const;

protected:
	ESRPGCommandType mCommandType = ESRPGCommandType::None;
};

/**
 * @brief  사용자 월드 입력 명령 객체
 */
struct FSRPGWorldTraceCommand : public FSRPGCommand
{
public:
	FSRPGWorldTraceCommand();

public:
	bool mIsLongPress = false;
};

/**
 * @brief  액션 생성 명령 객체
 */
template<typename ActionType>
struct FSRPGActionCreationCommand : public FSRPGCommand
{
protected:
	TSharedPtr<FSRPGAction> CreateAction() const override
	{
		TSharedPtr<ActionType> NewAction = TSharedPtr<ActionType>(new ActionType(), [](ActionType* Action) {
			delete Action;
			});
		NewAction->ReserveCommand(AsShared());

		return NewAction;
	}
};

