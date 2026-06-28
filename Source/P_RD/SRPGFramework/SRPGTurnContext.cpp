#include "SRPGFramework/SRPGTurnContext.h"
#include "SRPGFramework/SRPGAction.h"
#include "SRPGFramework/SRPGCommand.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Singleton/WorldSubsystem/PresentationBarrier.h"

#include "Pawn/UnitModel.h"

#include "Simulation/Logger/EventLogger.h"

TWeakObjectPtr<USRPGTurnContext> USRPGActionCreationCommandHandler::GetParent() const
{
	return mParent;
}

int8 USRPGActionCreationCommandHandler::GetCommandPriority() const
{
	return ISRPGCommandHandler::LOWEST_PRIORITY;
}

ESRPGCommandResult USRPGActionCreationCommandHandler::HandleCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
	if (Command.Get().mRequestedAction == nullptr)
	{
		/* 생성 명령 외는 처리 않함 */

		return ESRPGCommandResult::Ignored;
	}

	USRPGAction* NewAction = NewObject<USRPGAction>(this, Command.Get().mRequestedAction);
	if (NewAction == nullptr)
	{
		/* 생성 명령 외는 처리 않함 */

		return ESRPGCommandResult::Ignored;
	}

	/* 새로운 액션에 명령 예약 걸고 넣기 */

	NewAction->ReserveInitializeCommand(Command);

	USRPGTurnContext* TurnContext = mParent.Get();
	checkf(TurnContext != nullptr, TEXT("턴 컨텍스트 nullptr"));
	USRPGCombatModel* CombatModel = TurnContext->GetParent();
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	if (CombatModel->PushAction(NewAction) == false)
	{
		return ESRPGCommandResult::Ignored;
	}
	return ESRPGCommandResult::Handled;
}

void USRPGTurnContext::InitTurn(USRPGCombatModel* Parent, UUnitModel* Owner, int32 TurnId, int32 LifeCount)
{
	checkf(Parent != nullptr, TEXT("전투 서브시스템 nullptr"));
	checkf(Owner != nullptr, TEXT("턴 오너 유닛 nullptr"));

	checkf(mTurnPhase == ESRPGTurnPhase::None, TEXT("중복 초기화"));
	mTurnPhase = ESRPGTurnPhase::TurnInit;

	mParent = Parent;
	mOwner = Owner;
	mTurnId = TurnId;
	mLifeCount = LifeCount;

	USRPGActionCreationCommandHandler* ActionCreationCommandHandler = NewObject<USRPGActionCreationCommandHandler>(this);
	ActionCreationCommandHandler->mParent = this;
	mTurnDefaultCommandHandlers.Add(ActionCreationCommandHandler);
}

void USRPGTurnContext::BeginTurn()
{
	checkf(mTurnPhase == ESRPGTurnPhase::TurnInit, TEXT("이미 턴 진행 중에 재실행 오류"));
	mTurnPhase = ESRPGTurnPhase::TurnStart;

	UE_LOG(LogSRPGCombat, Log, TEXT("턴 시작"));

	// 턴 시작 연출
	TSharedPtr<FPresentationBarrier> PresentationBarrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateWeakLambda(this, [this]() {

		// 턴 실행
		checkf(mTurnPhase == ESRPGTurnPhase::TurnStart, TEXT("턴 진입 절차 오류"));
		mTurnPhase = ESRPGTurnPhase::TurnPlay;

		// 핸들러 등록
		USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
		checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));
		for (TScriptInterface<ISRPGCommandHandler>& TurnDefaultCommandHandler : mTurnDefaultCommandHandlers)
		{
			CommandRouterModel->OnHandleCommand.AddDynamic(this, &USRPGTurnContext::OnHandleCommand);
			CommandRouterModel->RegisterCommandHandler(TurnDefaultCommandHandler);
		}

		// 로그 작성
		GetWorldEventLogger(this)->BeginTurnLog(mOwner->GetModelId(), mOwner->GetClass());

		// 유닛 턴 시작 단계
		mOwner->OnBeginTurn();

		// 현 스텟을 캡처
		/*FGameplayEventData EventData;
		EventData.TargetData = UGASTargetFunctionLibrary::MakeSnapshotTargetDataHandle(mOwner.Get());
		EventData.Instigator = mOwner.Get();
		EventData.Target = mOwner.Get();*/

		// On Start Turn 패시브 실행
		//UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(mOwner.Get(), AbilityTags::GameplayAbility_Passive_OnStartTurn, MoveTemp(EventData));

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
		if (mOwner->IsPlayerUnitModel() == true)
		{
			// 액션 추가
		}
		// AI의 경우 움직임 판단 로직 시작
		else
		{
			// 액션 추가
		}
		}));
	OnBeginTurnUI.Broadcast(PresentationBarrier, this);
}

void USRPGTurnContext::TickTurn(float DeltaTime)
{
	if (mTurnPhase == ESRPGTurnPhase::TurnPlay && mReservedActions.Num() > mHeadActionIndex)
	{
		mReservedActions[mHeadActionIndex]->TickAction(DeltaTime);
		mReservedActions[mHeadActionIndex]->TryEndAction();
	}
}

void USRPGTurnContext::EndTurn()
{
	checkf(mTurnPhase == ESRPGTurnPhase::TurnAbort, TEXT("턴 종료 절차 오류"));
	mTurnPhase = ESRPGTurnPhase::TurnEnd;

	UE_LOG(LogSRPGCombat, Log, TEXT("턴 종료"));

	// 현 스텟을 캡처
	/*FGameplayEventData EventData;
	EventData.TargetData = UGASTargetFunctionLibrary::MakeSnapshotTargetDataHandle(mOwner.Get());
	EventData.Instigator = mOwner.Get();
	EventData.Target = mOwner.Get();*/

	// On End Turn 패시브 실행
	//UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(mOwner.Get(), AbilityTags::GameplayAbility_Passive_OnEndTurn, MoveTemp(EventData));

	// 유닛 턴 종료 단계
	mOwner->OnEndTurn();

	// 로그 작성
	GetWorldEventLogger(this)->EndTurnLog();

	// 턴 종료 연출
	TSharedPtr<FPresentationBarrier> PresentationBarrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateWeakLambda(this, [this]() {
		if (IsPermanent() == false)
		{
			--mLifeCount;
		}
		mReservedActions.Empty();

		// 핸들러 등록 해제
		USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
		checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));
		for (TScriptInterface<ISRPGCommandHandler>& TurnDefaultCommandHandler : mTurnDefaultCommandHandlers)
		{
			CommandRouterModel->UnregisterCommandHandler(TurnDefaultCommandHandler);
			CommandRouterModel->OnHandleCommand.RemoveDynamic(this, &USRPGTurnContext::OnHandleCommand);
		}

		checkf(mTurnPhase == ESRPGTurnPhase::TurnEnd, TEXT("턴 종료 절차 오류"));
		mTurnPhase = ESRPGTurnPhase::TurnInit;

		USRPGCombatModel* CombatModel = mParent.Get();
		checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));
		CombatModel->OnEndCurrentTurn(this, mTurnResult);
		}));
	OnEndTurnUI.Broadcast(PresentationBarrier, this, mTurnResult);
}

void USRPGTurnContext::EvaluateTurnStates(bool ForceAbort)
{
	EvaluateTurnEndState(ForceAbort);

	if (mReservedActions.Num() > mHeadActionIndex)
	{
		const bool ForceAbortAction = ForceAbort && mTurnPhase == ESRPGTurnPhase::TurnAbort;
		mReservedActions[mHeadActionIndex]->EvaluateActionEndState(ForceAbortAction);
	}
}

void USRPGTurnContext::OnEndCurrentAction(USRPGAction* Action, ESRPGActionResult ActionResult)
{
	// 액션 등록 해제
	++mHeadActionIndex;
	if (mHeadActionIndex > 5)
	{
		mReservedActions.RemoveAt(0, mHeadActionIndex);
		mHeadActionIndex = 0;
	}

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

void USRPGTurnContext::EvaluateTurnEndState(bool ForceAbort)
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

void USRPGTurnContext::EnqueueAction(USRPGAction* NewAction)
{
	const bool IsWaitingNewAction = mReservedActions.Num() <= mHeadActionIndex;

	NewAction->OnBeginActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGAction* Action) {
		OnBeginAnyActionUI.Broadcast(Barrier, this, Action);
		});
	NewAction->OnEndActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGAction* Action, ESRPGActionResult Result) {
		OnEndAnyActionUI.Broadcast(Barrier, this, Action, Result);
		});

	// 액션 등록
	mReservedActions.Add(NewAction);
	// 액션 초기화
	NewAction->InitAction(this, mOwner.Get());

	// 액션 대기 중 경우, 새로운 액션 즉시 진행
	if (IsWaitingNewAction == true)
	{
		DequeueAction();
	}
}

void USRPGTurnContext::DequeueAction()
{
	// 비어있는 경우는 그대로 대기
	if (mReservedActions.Num() <= mHeadActionIndex)
	{
		return;
	}

	checkf(mTurnPhase == ESRPGTurnPhase::TurnPlay, TEXT("액션 가능 상태에서만 다음 액션 진행 가능"));
	mTurnPhase = ESRPGTurnPhase::TurnPlay;

	// 액션 시작
	USRPGAction* CurAction = mReservedActions[mHeadActionIndex];
	CurAction->BeginAction();
}

void USRPGTurnContext::OnHandleCommand(ESRPGCommandResult Result)
{
	if (mTurnPhase == ESRPGTurnPhase::TurnPlay && mReservedActions.Num() > mHeadActionIndex)
	{
		mReservedActions[mHeadActionIndex]->TryEndAction();
	}
}

USRPGCombatModel* USRPGTurnContext::GetParent() const
{
	return mParent.Get();
}

UUnitModel* USRPGTurnContext::GetOwner() const
{
	return mOwner.Get();
}

int32 USRPGTurnContext::GetTurnId() const
{
	return mTurnId;
}

bool USRPGTurnContext::IsPermanent() const
{
	return mLifeCount == PERMENENT_TURN;
}

int32 USRPGTurnContext::GetLifeCount() const
{
	return mLifeCount;
}


