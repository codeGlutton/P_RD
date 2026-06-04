#include "SRPGFramework/SRPGAction.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/PresentationSyncSubsystem.h"

#include "Pawn/Unit.h"

#include "FunctionLibrary/GASTargetFunctionLibrary.h"

void FSRPGActionBuilder::InitBuilder(TSharedRef<FSRPGTurnContext> Owner, AUnit* Instigator)
{
	mOwner = Owner;
	mInstigator = Instigator;
}

void FSRPGAction::InitAction(TSharedRef<FSRPGTurnContext> Owner, AUnit* Instigator)
{
	checkf(mPhase == ESRPGActionPhase::None, TEXT("중복 초기화"));
	mPhase = ESRPGActionPhase::ActionInit;

	mOwner = Owner;
	mInstigator = Instigator;
}

void FSRPGAction::BeginAction()
{
	checkf(mPhase == ESRPGActionPhase::ActionInit, TEXT("이미 액션 진행 중에 재실행 오류"));
	mPhase = ESRPGActionPhase::ActionStart;
}

void FSRPGAction::TickAction(float DeltaTime)
{
}

void FSRPGAction::EndAction()
{
	checkf(mPhase == ESRPGActionPhase::ActionAbort, TEXT("액션 종료 절차 오류"));
	mPhase = ESRPGActionPhase::ActionEnd;

	TSharedPtr<FSRPGTurnContext> TurnContext = mOwner.Pin();
	checkf(TurnContext != nullptr, TEXT("이미 제거된 턴에서 Action 종료 명령 오류"));
	TurnContext->OnEndCurrentAction(AsShared(), mResult);
}

bool FSRPGAction::IsTurnEndingAction() const
{
	return false;
}

void FSRPGAction::EvaluateActionEndState(bool ForceAbort)
{
	/* 이미 중단 */

	if (mPhase == ESRPGActionPhase::ActionAbort)
	{
		return;
	}

	/* 외부에서 강제 종료되는가? */

	if (ForceAbort == true)
	{
		mResult = ESRPGActionResult::Cancelled;
		mPhase = ESRPGActionPhase::ActionAbort;
		return;
	}
}

UWorld* FSRPGAction::GetWorld() const
{
	return mOwner.Pin()->GetWorld();
}

TWeakPtr<FSRPGTurnContext> FSRPGAction::GetOwner() const
{
	return mOwner;
}

AUnit* FSRPGAction::GetInstigator() const
{
	return mInstigator;
}

