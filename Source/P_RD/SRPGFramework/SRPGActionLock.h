/*****************************************************************//**
 * @file   SRPGActionLock.h
 * @brief  턴 진행을 막는 RAII 방식의 Lock 객체 구현 헤더
 * @author 모호재
 * @date   2026-06-07
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"

struct FSRPGTurnContext;

/**
 * @brief  턴 진행을 막는 RAII 방식의 Lock 객체
 */
struct FSRPGActionLock
{
public:
	FSRPGActionLock(TSharedPtr<FSRPGTurnContext> TurnContext);
	~FSRPGActionLock();

private:
	TWeakPtr<FSRPGTurnContext> mTurnContext;
};