#include "SRPGFramework/SRPGAction.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/PresentationSyncSubsystem.h"

#include "Pawn/Unit.h"

#include "FunctionLibrary/GASTargetFunctionLibrary.h"

void FSRPGAction::InitAction(TSharedRef<FSRPGTurnContext> Owner, AUnit* Instigator, TArray<AUnit*>& Targets)
{
	checkf(mPhase == ESRPGActionPhase::None, TEXT("중복 초기화"));
	mPhase = ESRPGActionPhase::ActionInit;

	mOwner = Owner;
	mInstigator = Instigator;
	mTargets = Targets;
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

	mOwner.Pin()->OnEndCurrentAction(AsShared(), mResult);
}

bool FSRPGAction::IsBlockTurnProgression() const
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
