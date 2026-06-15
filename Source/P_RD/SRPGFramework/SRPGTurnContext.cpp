#include "SRPGFramework/SRPGTurnContext.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/PresentationSyncSubsystem.h"

#include "Pawn/Unit.h"
#include "SRPGFramework/SRPGSkillBuildAction.h"

#include "FunctionLibrary/GASTargetFunctionLibrary.h"

void FSRPGTurnContext::InitTurn(USRPGCombatSubsystem* Parent, AUnit* Owner, int32 LifeCount)
{
	checkf(Parent != nullptr, TEXT("전투 서브시스템 nullptr"));
	checkf(Owner != nullptr, TEXT("턴 오너 유닛 nullptr"));

	checkf(mTurnPhase == ESRPGTurnPhase::None, TEXT("중복 초기화"));
	mTurnPhase = ESRPGTurnPhase::TurnInit;

	mParent = Parent;
	mOwner = Owner;
	mLifeCount = LifeCount;
}

void FSRPGTurnContext::BeginTurn()
{
	checkf(mTurnPhase == ESRPGTurnPhase::TurnInit, TEXT("이미 턴 진행 중에 재실행 오류"));
	mTurnPhase = ESRPGTurnPhase::TurnStart;

	UE_LOG(LogSRPGCombat, Log, TEXT("턴 시작"));
	
	UPresentationSyncSubsystem* PresentationSyncSubsystem = GetWorld()->GetSubsystem<UPresentationSyncSubsystem>();
	checkf(PresentationSyncSubsystem != nullptr, TEXT("연출 동기화 서브시스템 nullptr"));

	auto PresentationBarrier = PresentationSyncSubsystem->MakePresentationBarrier(FOnFinishPresentation::CreateSPLambda(AsShared(), [this]() {
		mOwner->OnBeginTurn();
		
		// 현 스텟을 캡처
		FGameplayEventData EventData;
		EventData.TargetData = UGASTargetFunctionLibrary::MakeSnapshotTargetDataHandle(mOwner);
		EventData.Instigator = mOwner;
		EventData.Target = mOwner;

		// On Start Turn 패시브 실행
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(mOwner, AbilityTags::GameplayAbility_Passive_OnStartTurn, MoveTemp(EventData));

		// 턴 실행
		checkf(mTurnPhase == ESRPGTurnPhase::TurnStart, TEXT("턴 진입 절차 오류"));
		mTurnPhase = ESRPGTurnPhase::TurnPlay;

		USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
		checkf(CombatSubsystem != nullptr, TEXT("전투 서브시스템 nullptr"));

		// 전투 상태 평가
		CombatSubsystem->EvaluateCombatStates();

		// 턴 종료 여부 체크
		if (mTurnPhase == ESRPGTurnPhase::TurnAbort)
		{
			EndTurn();
			return;
		}

		// 플레이어의 경우 주사위 굴리기 액션 추가
		if (mOwner->IsPlayerUnit() == true)
		{
			// 액션 추가
		}
		// AI의 경우 움직임 판단 로직 시작
		else
		{
			// 액션 추가
		}
		}));
	OnBeginTurnUI.Broadcast(PresentationBarrier, *this);
}

void FSRPGTurnContext::TickTurn(float DeltaTime)
{
	if (mTurnPhase == ESRPGTurnPhase::TurnPlay && mActions.IsEmpty() == false)
	{
		(*mActions.Peek())->TickAction(DeltaTime);
	}
}

void FSRPGTurnContext::EndTurn()
{
	checkf(mTurnPhase == ESRPGTurnPhase::TurnAbort, TEXT("턴 종료 절차 오류"));
	mTurnPhase = ESRPGTurnPhase::TurnEnd;

	UPresentationSyncSubsystem* PresentationSyncSubsystem = GetWorld()->GetSubsystem<UPresentationSyncSubsystem>();
	checkf(PresentationSyncSubsystem != nullptr, TEXT("연출 동기화 서브시스템 nullptr"));

	// 현 스텟을 캡처
	FGameplayEventData EventData;
	EventData.TargetData = UGASTargetFunctionLibrary::MakeSnapshotTargetDataHandle(mOwner);
	EventData.Instigator = mOwner;
	EventData.Target = mOwner;

	// On End Turn 패시브 실행
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(mOwner, AbilityTags::GameplayAbility_Passive_OnEndTurn, MoveTemp(EventData));
	mOwner->OnEndTurn();

	auto PresentationBarrier = PresentationSyncSubsystem->MakePresentationBarrier(FOnFinishPresentation::CreateSPLambda(AsShared(), [this]() {
		if (IsPermanent() == false)
		{
			--mLifeCount;
		}

		checkf(mTurnPhase == ESRPGTurnPhase::TurnEnd, TEXT("턴 종료 절차 오류"));
		mTurnPhase = ESRPGTurnPhase::TurnInit;
		mActions.Empty();

		UE_LOG(LogSRPGCombat, Log, TEXT("턴 종료"));

		USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
		checkf(CombatSubsystem != nullptr, TEXT("전투 서브시스템 nullptr"));
		CombatSubsystem->OnEndCurrentTurn(AsShared(), mTurnResult);
		}));
	OnEndTurnUI.Broadcast(PresentationBarrier, *this, mTurnResult);
}

ESRPGActionCommandResult FSRPGTurnContext::RouteCommand(TSharedPtr<const FSRPGActionCommand> Command)
{
	ESRPGActionCommandResult Result = ESRPGActionCommandResult::Ignored;

	if (mTurnPhase != ESRPGTurnPhase::TurnPlay)
	{
		// 턴 실행 외 상황에서 진입한 명령은 무시 처리
		return Result;
	}

	TSharedPtr<FSRPGAction> CurAction = nullptr;
	mActions.Peek(CurAction);
	if (CurAction != nullptr)
	{
		/* 실행 중인 액션 객체에 커맨드 처리 요청 */

		Result = CombineSRPGActionCommandResult(CurAction->HandleCommand(Command), Result);
		if (Result == ESRPGActionCommandResult::Handled)
		{
			return Result;
		}
	}

	/* 액션 대기 중인 경우 직접 처리 */

	Result = CombineSRPGActionCommandResult(HandleActionCreationCommand(Command), Result);
	if (Result == ESRPGActionCommandResult::Handled)
	{
		return Result;
	}

	/* 커맨드 처리 실패 시 Fallback 처리 */

	return CombineSRPGActionCommandResult(HandleFallbackCommand(Command), Result);
}

ESRPGActionCommandResult FSRPGTurnContext::HandleActionCreationCommand(TSharedPtr<const FSRPGActionCommand> Command)
{
	TSharedPtr<FSRPGAction> NewAction = Command->CreateAction();
	if (NewAction == nullptr)
	{
		/* 생성 명령 외는 처리 않함 */

		return ESRPGActionCommandResult::Ignored;
	}

	/* 새로운 Action 생성 후, 예약 걸어두기 */

	NewAction->ReserveCommand(Command);
	EnqueueAction(NewAction);
	
	return ESRPGActionCommandResult::Handled;
}

ESRPGActionCommandResult FSRPGTurnContext::HandleFallbackCommand(TSharedPtr<const FSRPGActionCommand> Command)
{
	if (Command->GetActionCommandType() == ESRPGActionCommandType::WorldTrace)
	{
		// TODO : 꾹 눌렀을 때 정보 처리 부분
	}

	return ESRPGActionCommandResult::Ignored;
}

void FSRPGTurnContext::EnqueueAction(TSharedPtr<FSRPGAction> NewAction)
{
	const bool IsWaitingNewAction = mActions.IsEmpty() == true;

	NewAction->OnBeginActionUI.AddSPLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const FSRPGAction& Action) {
		OnBeginAnyActionUI.Broadcast(Barrier, *this, Action);
		});
	NewAction->OnEndActionUI.AddSPLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const FSRPGAction& Action, ESRPGActionResult Result) {
		OnEndAnyActionUI.Broadcast(Barrier, *this, Action, Result);
		});
	mActions.Enqueue(NewAction);

	// 액션 대기 중 경우, 새로운 액션 즉시 진행
	if (IsWaitingNewAction == true)
	{
		DequeueAction();
	}
}

void FSRPGTurnContext::DequeueAction()
{
	checkf(mTurnPhase == ESRPGTurnPhase::TurnPlay, TEXT("액션 가능 상태에서만 다음 액션 진행 가능"));

	if (mActions.IsEmpty() == true)
	{
		return;
	}

	mTurnPhase = ESRPGTurnPhase::TurnPlay;

	TSharedRef<FSRPGAction> CurAction = mActions.Peek()->ToSharedRef();
	CurAction->InitAction(AsShared(), mOwner);
	CurAction->BeginAction();
}

void FSRPGTurnContext::EvaluateTurnStates(bool ForceAbort)
{
	EvaluateTurnEndState(ForceAbort);

	if (mActions.IsEmpty() == false)
	{
		const bool ForceAbortAction = ForceAbort && mTurnPhase == ESRPGTurnPhase::TurnAbort;
		(*mActions.Peek())->EvaluateActionEndState(ForceAbortAction);
	}
}

void FSRPGTurnContext::OnEndCurrentAction(TSharedRef<FSRPGAction> Action, ESRPGActionResult ActionResult)
{
	mActions.Pop();

	// 턴 소모 액션 처리
	bool IsBlockTurnSuccessfully = Action->ConsumesTurn() == true && ActionResult == ESRPGActionResult::Succeeded;
	if (IsBlockTurnSuccessfully == true)
	{
		mTurnPhase = ESRPGTurnPhase::TurnAbort;
		mTurnResult = ESRPGTurnResult::Succeeded;
	}

	USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	checkf(CombatSubsystem != nullptr, TEXT("전투 서브시스템 nullptr"));

	// 전투 상태 평가
	CombatSubsystem->EvaluateCombatStates();

	// 턴 종료 여부 체크
	if (mTurnPhase == ESRPGTurnPhase::TurnAbort)
	{
		EndTurn();
		return;
	}

	DequeueAction();
}

void FSRPGTurnContext::EvaluateTurnEndState(bool ForceAbort)
{
	checkf(mOwner != nullptr, TEXT("턴 주인 미존재"));

	if (mTurnPhase != ESRPGTurnPhase::TurnPlay)
	{
		return;
	}

	/* 외부에서 강제 종료되는가? */
	/* 플레이어가 죽어서 턴이 종료되는가? */

	if (ForceAbort == true || mOwner->IsDead() == true)
	{
		mTurnResult = ESRPGTurnResult::Cancelled;
		mTurnPhase = ESRPGTurnPhase::TurnAbort;
		return;
	}
}

UWorld* FSRPGTurnContext::GetWorld() const
{
	return mOwner->GetWorld();
}

USRPGCombatSubsystem* FSRPGTurnContext::GetParent() const
{
	return mParent;
}

AUnit* FSRPGTurnContext::GetOwner() const
{
	return mOwner;
}

bool FSRPGTurnContext::IsPermanent() const
{
	return mLifeCount == PERMENENT_TURN;
}

int32 FSRPGTurnContext::GetLifeCount() const
{
	return mLifeCount;
}

