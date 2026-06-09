#include "SRPGFramework/SRPGActionLock.h"
#include "SRPGFramework/SRPGTurnContext.h"

FSRPGActionLock::FSRPGActionLock(TSharedPtr<FSRPGTurnContext> TurnContext) : mTurnContext(TurnContext)
{
	if (TurnContext != nullptr)
	{
		checkf(TurnContext->mPhase == ESRPGTurnPhase::ActionSelect, TEXT("액션 선택 대기 중에만 락 가능"));
		TurnContext->mPhase = ESRPGTurnPhase::ActionLock;
	}
}

FSRPGActionLock::~FSRPGActionLock()
{
	TSharedPtr<FSRPGTurnContext> TurnContext = mTurnContext.Pin();
	if (TurnContext != nullptr)
	{
		checkf(TurnContext->mPhase == ESRPGTurnPhase::ActionLock, TEXT("액션 락 중에만 언락 가능"));
		TurnContext->mPhase = ESRPGTurnPhase::ActionSelect;
	}
}

