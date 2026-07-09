#include "SRPGFramework/SRPGTurnContext.h"
#include "SRPGFramework/SRPGAction.h"
#include "SRPGFramework/SRPGCommand.h"

#include "RDCollision.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "Simulation/Logger/EventLogger.h"

#include "SRPGFramework/SRPGDiceRollAction.h"
#include "SRPGFramework/SRPGEnemyTurnPlanner.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Actor/BoardActor/BoardSelectionTarget.h"
#include "Pawn/UnitModel.h"
#include "Pawn/Enemy/EnemyUnitModel.h"
#include "FunctionLibrary/RandomStreamFunctionLibrary.h"

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

TWeakObjectPtr<USRPGTurnContext> USRPGActionCreationCommandHandler::GetParent() const
{
	return mParent;
}

int8 USRPGDetailInfoPopupCommandHandler::GetCommandPriority() const
{
	return ISRPGCommandHandler::HIGHEST_PRIORITY;
}

ESRPGCommandResult USRPGDetailInfoPopupCommandHandler::HandleCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
	if (Command.Get().GetCommandType() == ESRPGCommandType::WorldTrace)
	{
		/* 월드 공간 터치 시 선택 위치에 따라서 결정 */

		const FSRPGWorldTraceCommand& WorldTraceCommand = Command.Get<FSRPGWorldTraceCommand>();
		if (WorldTraceCommand.mIsLongPress == true)
		{
			AActor* HitActor = nullptr;
			FTileIndex TileIndex = FTileIndex::Invalid;
			GetTileActorUnderCursor(GetWorld(), RDTraceChannels::TileAnyTrace, WorldTraceCommand.mScreenPosition, OUT HitActor, OUT TileIndex);

			IBoardSelectionTarget* SelectionTarget = Cast<IBoardSelectionTarget>(HitActor);
			if (SelectionTarget != nullptr && SelectionTarget->IsSelectable() == true)
			{
				WorldTraceCommand.OnShowTargetDetailPanelUI.Broadcast(SelectionTarget);
			}
			return ESRPGCommandResult::Handled;
		}
	}

	return ESRPGCommandResult::Ignored;
}

TWeakObjectPtr<USRPGTurnContext> USRPGDetailInfoPopupCommandHandler::GetParent() const
{
	return mParent;
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

	USRPGDetailInfoPopupCommandHandler* DetailInfoPopupCommandHandler = NewObject<USRPGDetailInfoPopupCommandHandler>(this);
	DetailInfoPopupCommandHandler->mParent = this;
	mTurnDefaultCommandHandlers.Add(DetailInfoPopupCommandHandler);
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

		USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
		checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));
		UPassiveComponentModel* PassiveComponentModel = mOwner->GetPassiveComponentModel();
		checkf(PassiveComponentModel != nullptr, TEXT("패시브 컴포넌트 nullptr"));
		USRPGCombatModel* CombatModel = mParent.Get();
		checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

		// 핸들러 등록
		{
			CommandRouterModel->OnHandleCommand.AddDynamic(this, &USRPGTurnContext::OnHandleCommand);
			for (TScriptInterface<ISRPGCommandHandler>& TurnDefaultCommandHandler : mTurnDefaultCommandHandlers)
			{
				CommandRouterModel->RegisterCommandHandler(TurnDefaultCommandHandler);
			}
		}

		// 로그 작성
		GetWorldEventLogger(this)->BeginTurnLog(mOwner->GetModelId(), mOwner->GetClass());

		// 유닛 턴 시작 단계
		mOwner->OnBeginTurn();

		// 전투 상태 평가
		CombatModel->EvaluateCombatStates();

		// 턴 종료 여부 체크
		if (mTurnPhase == ESRPGTurnPhase::TurnAbort)
		{
			EndTurn();
			return;
		}

		if (mOwner->IsPlayerUnitModel() == true)
		{
			/* 플레이어의 경우 주사위 굴리기 액션 추가 */

			TInstancedStruct<FSRPGCommand> DicePrepareCommand;
			DicePrepareCommand.InitializeAs<FSRPGDicePrepareCommand>();
			DicePrepareCommand.GetMutable<FSRPGDicePrepareCommand>().OnShowDicePanelUI.AddWeakLambda(this, [this]() {
				OnShowDicePanelAtTurnStartUI.Broadcast(this);
				});

			CommandRouterModel->SummitCommand(DicePrepareCommand);
		}
		else
		{
			/* AI의 경우 움직임 판단 로직 시작 */

			UEnemyUnitModel* Enemy = Cast<UEnemyUnitModel>(mOwner.Get());
			UUnitModel* Player = CombatModel->GetPlayerUnit();
			UTileMapModel* TileMap = CombatModel->GetTileMap();

			// PlanTurn에서 Command 리스트를 리턴하면 순서대로 라우터에 전달
			// @note 스킬 랜덤 선택은 시뮬/라이브 동일 결과를 위해 룸의 이벤트 스트림 사용
			for (TInstancedStruct<FSRPGCommand>& Command : USRPGEnemyTurnPlanner::PlanTurn(Enemy, Player, TileMap, URandomStreamFunctionLibrary::GetEventStream(this)))
			{
				CommandRouterModel->SummitCommand(Command);
			}
		}
		}));
	OnBeginTurnUI.Broadcast(PresentationBarrier, this);
}

void USRPGTurnContext::TickTurn(float DeltaTime)
{
	if (mTurnPhase == ESRPGTurnPhase::TurnPlay && mReservedActions.Num() > mHeadActionIndex)
	{
		mReservedActions[mHeadActionIndex]->TryBeginAction();
		mReservedActions[mHeadActionIndex]->TickAction(DeltaTime);
		mReservedActions[mHeadActionIndex]->TryEndAction();
	}
}

void USRPGTurnContext::EndTurn()
{
	checkf(mTurnPhase == ESRPGTurnPhase::TurnAbort, TEXT("턴 종료 절차 오류"));
	mTurnPhase = ESRPGTurnPhase::TurnEnd;

	UPassiveComponentModel* PassiveComponentModel = mOwner->GetPassiveComponentModel();
	checkf(PassiveComponentModel != nullptr, TEXT("패시브 컴포넌트 nullptr"));

	UE_LOG(LogSRPGCombat, Log, TEXT("턴 종료"));

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
		// 큐를 비우면 소비 인덱스도 함께 되돌려야 한다. 이 컨텍스트는 다음 턴에 재사용되므로, 인덱스가 남아 있으면
		// 새로 등록된 액션이 mReservedActions.Num() > mHeadActionIndex 조건을 통과하지 못해 영원히 시작되지 않는다.
		mHeadActionIndex = 0;

		// 핸들러 등록 해제
		USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
		checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));
		for (TScriptInterface<ISRPGCommandHandler>& TurnDefaultCommandHandler : mTurnDefaultCommandHandlers)
		{
			CommandRouterModel->UnregisterCommandHandler(TurnDefaultCommandHandler);
		}
		CommandRouterModel->OnHandleCommand.RemoveDynamic(this, &USRPGTurnContext::OnHandleCommand);

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

	// 강제 중단
	if (mShouldTerminateAfterAction == true)
	{
		mShouldTerminateAfterAction = false;
		return;
	}

	// 턴 종료 여부 체크
	if (mTurnPhase == ESRPGTurnPhase::TurnAbort)
	{
		EndTurn();
		return;
	}

	// 예약된 액션 꺼내기
	DequeueAction();

	// 다음 액션 실행
	if (mTurnPhase == ESRPGTurnPhase::TurnPlay && mReservedActions.Num() > mHeadActionIndex)
	{
		mReservedActions[mHeadActionIndex]->TryBeginAction();
	}
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
	const bool IsPlayingInPlayAction = mReservedActions.Num() > mHeadActionIndex && mReservedActions[mHeadActionIndex]->GetActionType() == ESRPGActionType::InPlayAction;
	if (IsPlayingInPlayAction == true && NewAction->GetActionType() == ESRPGActionType::BuildAction)
	{
		return;
	}

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
}

void USRPGTurnContext::DequeueAction()
{
	// 비어있는 경우는 그대로 대기
	if (mReservedActions.Num() <= mHeadActionIndex)
	{
		return;
	}

	// 액션 등록 해제
	++mHeadActionIndex;
	if (mHeadActionIndex > 5)
	{
		mReservedActions.RemoveAt(0, mHeadActionIndex);
		mHeadActionIndex = 0;
	}
}

void USRPGTurnContext::OnHandleCommand(ESRPGCommandResult Result)
{
	if (mTurnPhase == ESRPGTurnPhase::TurnPlay && mReservedActions.Num() > mHeadActionIndex)
	{
		mReservedActions[mHeadActionIndex]->TryBeginAction();
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

void USRPGTurnContext::ForcedClearActions()
{
	if (mTurnPhase == ESRPGTurnPhase::TurnPlay && mReservedActions.Num() > mHeadActionIndex)
	{
		const int32 NewHeadActionIndex = mHeadActionIndex + 1;
		while (mReservedActions.Num() != NewHeadActionIndex)
		{
			// 현재 액션이 마지막 액션이 되도록 예약된 액션 전부 제거
			mReservedActions.Pop();
		}

		// 로그 작성
		GetWorldEventLogger(this)->BeginTurnLog(mOwner->GetModelId(), mOwner->GetClass());
		GetWorldEventLogger(this)->BeginActionLog(mOwner->GetTileTransform().mIndex);

		// 현재 액션 종료 요청
		mReservedActions[mHeadActionIndex]->MarkActionCompleted(ESRPGActionResult::Cancelled);
		mReservedActions[mHeadActionIndex]->TryEndAction();
	}
	else
	{
		mReservedActions.Empty();
		mHeadActionIndex = 0;
	}
}

void USRPGTurnContext::ForcedAdvanceUntilNextAction(TInstancedStruct<FSRPGCommand> NextCommand)
{
	mShouldTerminateAfterAction = true;

	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 서브시스템 모델 nullptr"));

	CommandRouterModel->SummitCommand(NextCommand);
}
