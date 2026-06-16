/*****************************************************************//**
 * @file   SRPGCommandHandler.h
 * @brief  SRPG 명령을 처리할 수 있는 객체 인터페이스 정의 헤더
 * @author 모호재
 * @date   2026-06-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"

struct FSRPGCommand;

/**
 * @brief  SRPG 명령을 처리할 수 있는 객체 인터페이스
 */
struct ISRPGCommandHandler
{
	friend class USRPGCommandRouterSubsystem;

public:
	virtual ~ISRPGCommandHandler() = default;

protected:
	virtual int8 GetCommandPriority() const = 0;
	virtual ESRPGCommandResult HandleCommand(TSharedPtr<const FSRPGCommand> Command) = 0;

public:
	static constexpr int8 HIGHEST_PRIORITY = INT8_MAX;
	static constexpr int8 LOWEST_PRIORITY = INT8_MIN;
};
