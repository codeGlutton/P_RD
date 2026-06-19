#include "SRPGFramework/SRPGTurnContext.h"
#include "SRPGFramework/SRPGAction.h"
#include "SRPGFramework/SRPGCommand.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterSubsystem.h"
#include "Singleton/WorldSubsystem/PresentationSyncSubsystem.h"

#include "Pawn/Unit.h"

#include "FunctionLibrary/GASTargetFunctionLibrary.h"

UWorld* FSRPGActionCreationCommandHandler::GetWorld() const
{
	TSharedPtr<FSRPGTurnContext> TurnContext = GetParent().Pin();
	if (TurnContext != nullptr)
	{
		return TurnContext->GetWorld();
	}
	return nullptr;
}

TWeakPtr<FSRPGTurnContext> FSRPGActionCreationCommandHandler::GetParent() const
{
	return mParent;
}

int8 FSRPGActionCreationCommandHandler::GetCommandPriority() const
{
	return ISRPGCommandHandler::LOWEST_PRIORITY;
}

ESRPGCommandResult FSRPGActionCreationCommandHandler::HandleCommand(TSharedPtr<const FSRPGCommand> Command)
{
	TSharedPtr<FSRPGAction> NewAction = Command->CreateAction();
	if (NewAction == nullptr)
	{
		/* 생성 명령 외는 처리 않함 */

		return ESRPGCommandResult::Ignored;
	}

	/* 새로운 Action 생성 후 등록 */

	TSharedPtr<FSRPGTurnContext> TurnContext = mParent.Pin();
	checkf(TurnContext != nullptr, TEXT("턴 컨텍스트 nullptr"));
	USRPGCombatModel* CombatModel = TurnContext->GetParent();
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	if (CombatModel->PushAction(NewAction) == false)
	{
		return ESRPGCommandResult::Ignored;
	}
	return ESRPGCommandResult::Handled;
}

void FSRPGTurnContext::InitTurn(USRPGCombatModel* Parent, AUnit* Owner, int32 LifeCount)
{
	checkf(Parent != nullptr, TEXT("전투 서브시스템 nullptr"));
	checkf(Owner != nullptr, TEXT("턴 오너 유닛 nullptr"));

	checkf(mTurnPhase == ESRPGTurnPhase::None, TEXT("중복 초기화"));
	mTurnPhase = ESRPGTurnPhase::TurnInit;

	mParent = Parent;
	mOwner = Owner;
	mLifeCount = LifeCount;

	TSharedPtr<FSRPGActionCreationCommandHandler> ActionCreationCommandHandler = MakeShared<FSRPGActionCreationCommandHandler>();
	ActionCreationCommandHandler->mParent = AsShared();
	mTurnDefaultCommandHandlers.Add(ActionCreationCommandHandler);
}

void FSRPGTurnContext::BeginTurn()
{
	checkf(mTurnPhase == ESRPGTurnPhase::TurnInit, TEXT("이미 턴 진행 중에 재실행 오류"));
	mTurnPhase = ESRPGTurnPhase::TurnStart;

	UE_LOG(LogSRPGCombat, Log, TEXT("턴 시작"));
	
	// 턴 시작 연출
	UPresentationSyncSubsystem* PresentationSyncSubsystem = GetWorld()->GetSubsystem<UPresentationSyncSubsystem>();
	checkf(PresentationSyncSubsystem != nullptr, TEXT("연출 동기화 서브시스템 nullptr"));
	auto PresentationBarrier = PresentationSyncSubsystem->MakePresentationBarrier(FOnFinishPresentation::CreateSPLambda(AsShared(), [this]() {
		
		// 턴 실행
		checkf(mTurnPhase == ESRPGTurnPhase::TurnStart, TEXT("턴 진입 절차 오류"));
		mTurnPhase = ESRPGTurnPhase::TurnPlay;

		// 핸들러 등록
		USRPGCommandRouterSubsystem* CommandRouterSubsystem = GetWorld()->GetSubsystem<USRPGCommandRouterSubsystem>();
		checkf(CommandRouterSubsystem != nullptr, TEXT("명령 라우터 서브시스템 nullptr"));
		for (TSharedPtr<ISRPGCommandHandler>& TurnDefaultCommandHandler : mTurnDefaultCommandHandlers)
		{
			CommandRouterSubsystem->RegisterCommandHandler(TurnDefaultCommandHandler);
		}

		// 유닛 턴 시작 단계
		mOwner->OnBeginTurn();
		
		// 현 스텟을 캡처
		FGameplayEventData EventData;
		EventData.TargetData = UGASTargetFunctionLibrary::MakeSnapshotTargetDataHandle(mOwner.Get());
		EventData.Instigator = mOwner.Get();
		EventData.Target = mOwner.Get();

		// On Start Turn 패시브 실행
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(mOwner.Get(), AbilityTags::GameplayAbility_Passive_OnStartTurn, MoveTemp(EventData));

		USRPGCombatModel* CombatModel = mParent.Get();
		checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

		// 전투 상태 평가
		CombatModel->EvaluateCombatStates();

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
	if (mTurnPhase == ESRPGTurnPhase::TurnPlay && mReservedActions.IsEmpty() == false)
	{
		(*mReservedActions.Peek())->TickAction(DeltaTime);
	}
}

void FSRPGTurnContext::EndTurn()
{
	checkf(mTurnPhase == ESRPGTurnPhase::TurnAbort, TEXT("턴 종료 절차 오류"));
	mTurnPhase = ESRPGTurnPhase::TurnEnd;

	UE_LOG(LogSRPGCombat, Log, TEXT("턴 종료"));

	// 현 스텟을 캡처
	FGameplayEventData EventData;
	EventData.TargetData = UGASTargetFunctionLibrary::MakeSnapshotTargetDataHandle(mOwner.Get());
	EventData.Instigator = mOwner.Get();
	EventData.Target = mOwner.Get();

	// On End Turn 패시브 실행
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(mOwner.Get(), AbilityTags::GameplayAbility_Passive_OnEndTurn, MoveTemp(EventData));
	mOwner->OnEndTurn();

	// 턴 종료 연출
	UPresentationSyncSubsystem* PresentationSyncSubsystem = GetWorld()->GetSubsystem<UPresentationSyncSubsystem>();
	checkf(PresentationSyncSubsystem != nullptr, TEXT("연출 동기화 서브시스템 nullptr"));
	auto PresentationBarrier = PresentationSyncSubsystem->MakePresentationBarrier(FOnFinishPresentation::CreateSPLambda(AsShared(), [this]() {
		if (IsPermanent() == false)
		{
			--mLifeCount;
		}
		mReservedActions.Empty();

		// 핸들러 등록 해제
		USRPGCommandRouterSubsystem* CommandRouterSubsystem = GetWorld()->GetSubsystem<USRPGCommandRouterSubsystem>();
		checkf(CommandRouterSubsystem != nullptr, TEXT("명령 라우터 서브시스템 nullptr"));
		for (TSharedPtr<ISRPGCommandHandler>& TurnDefaultCommandHandler : mTurnDefaultCommandHandlers)
		{
			CommandRouterSubsystem->UnregisterCommandHandler(TurnDefaultCommandHandler);
		}

		checkf(mTurnPhase == ESRPGTurnPhase::TurnEnd, TEXT("턴 종료 절차 오류"));
		mTurnPhase = ESRPGTurnPhase::TurnInit;

		USRPGCombatModel* CombatModel = mParent.Get();
		checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));
		CombatModel->OnEndCurrentTurn(AsShared(), mTurnResult);
		}));
	OnEndTurnUI.Broadcast(PresentationBarrier, *this, mTurnResult);
}

void FSRPGTurnContext::EvaluateTurnStates(bool ForceAbort)
{
	EvaluateTurnEndState(ForceAbort);

	if (mReservedActions.IsEmpty() == false)
	{
		const bool ForceAbortAction = ForceAbort && mTurnPhase == ESRPGTurnPhase::TurnAbort;
		(*mReservedActions.Peek())->EvaluateActionEndState(ForceAbortAction);
	}
}

void FSRPGTurnContext::OnEndCurrentAction(TSharedRef<FSRPGAction> Action, ESRPGActionResult ActionResult)
{
	// 액션 등록 해제
	mReservedActions.Pop();

	// 턴 소모 액션 처리
	bool IsBlockTurnSuccessfully = Action->ConsumesTurn() == true && ActionResult == ESRPGActionResult::Succeeded;
	if (IsBlockTurnSuccessfully == true)
	{
		mTurnPhase = ESRPGTurnPhase::TurnAbort;
		mTurnResult = ESRPGTurnResult::Succeeded;
	}

	USRPGCombatModel* CombatModel = mParent.Get();
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	// 전투 상태 평가
	CombatModel->EvaluateCombatStates();

	// 턴 종료 여부 체크
	if (mTurnPhase == ESRPGTurnPhase::TurnAbort)
	{
		EndTurn();
		return;
	}

	// 예약된 액션 꺼내기
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

void FSRPGTurnContext::EnqueueAction(TSharedPtr<FSRPGAction> NewAction)
{
	const bool IsWaitingNewAction = mReservedActions.IsEmpty() == true;

	NewAction->OnBeginActionUI.AddSPLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const FSRPGAction& Action) {
		OnBeginAnyActionUI.Broadcast(Barrier, *this, Action);
		});
	NewAction->OnEndActionUI.AddSPLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const FSRPGAction& Action, ESRPGActionResult Result) {
		OnEndAnyActionUI.Broadcast(Barrier, *this, Action, Result);
		});

	// 액션 등록
	mReservedActions.Enqueue(NewAction);
	// 액션 초기화
	NewAction->InitAction(AsShared(), mOwner.Get());

	// 액션 대기 중 경우, 새로운 액션 즉시 진행
	if (IsWaitingNewAction == true)
	{
		DequeueAction();
	}
}

void FSRPGTurnContext::DequeueAction()
{
	// 비어있는 경우는 그대로 대기
	if (mReservedActions.IsEmpty() == true)
	{
		return;
	}

	checkf(mTurnPhase == ESRPGTurnPhase::TurnPlay, TEXT("액션 가능 상태에서만 다음 액션 진행 가능"));
	mTurnPhase = ESRPGTurnPhase::TurnPlay;

	// 액션 시작
	TSharedRef<FSRPGAction> CurAction = mReservedActions.Peek()->ToSharedRef();
	CurAction->BeginAction();
}

UWorld* FSRPGTurnContext::GetWorld() const
{
	return mOwner->GetWorld();
}

USRPGCombatModel* FSRPGTurnContext::GetParent() const
{
	return mParent.Get();
}

AUnit* FSRPGTurnContext::GetOwner() const
{
	return mOwner.Get();
}

bool FSRPGTurnContext::IsPermanent() const
{
	return mLifeCount == PERMENENT_TURN;
}

int32 FSRPGTurnContext::GetLifeCount() const
{
	return mLifeCount;
}


