#include "SRPGFramework/SRPGAction.h"
#include "SRPGFramework/SRPGTurnContext.h"
#include "SRPGFramework/SRPGCommand.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Singleton/WorldSubsystem/PresentationBarrier.h"

#include "ObjectView.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Pawn/UnitModel.h"
#include "Actor/TileMap/TileMap.h"

#include "Simulation/Logger/EventLogger.h"

void USRPGAction::InitAction(USRPGTurnContext* Parent, UUnitModel* Instigator)
{
	checkf(Instigator != nullptr, TEXT("유발자 유닛 nullptr"));

	checkf(mActionPhase == ESRPGActionPhase::None, TEXT("중복 초기화"));
	mActionPhase = ESRPGActionPhase::ActionInit;

	mParent = Parent;
	mInstigator = Instigator;
}

void USRPGAction::BeginAction()
{
	checkf(mActionPhase == ESRPGActionPhase::ActionInit, TEXT("이미 액션 진행 중에 재실행 오류"));
	mActionPhase = ESRPGActionPhase::ActionStart;

	UE_LOG(LogSRPGCombat, Log, TEXT("액션 시작"));

	// 시작 연출 시작
	TSharedPtr<FPresentationBarrier> PresentationBarrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateWeakLambda(this, [this]() {
		checkf(mActionPhase == ESRPGActionPhase::ActionStart, TEXT("액션 실행 절차 오류"));
		mActionPhase = ESRPGActionPhase::ActionPlay;
		
		// 핸들러 등록
		USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
		checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));
		CommandRouterModel->RegisterCommandHandler(this);

		// 로그 작성
		GetWorldEventLogger(this)->BeginActionLog(mInstigator->GetTileTransform().mIndex);

		// 예약된 초기화 커맨드 시작
		if (mInitializeCommand.IsValid() == true)
		{
			HandleCommand(mInitializeCommand);
			mInitializeCommand.Reset();
		}
		
		// 액션 시작 로직
		OnBeginAction();
		}));
	OnBeginActionUI.Broadcast(PresentationBarrier, this);
}

void USRPGAction::TickAction(float DeltaTime)
{
	if (mActionPhase == ESRPGActionPhase::ActionPlay)
	{
		OnTickAction(DeltaTime);
	}
}

void USRPGAction::EndAction()
{
	checkf(mActionPhase == ESRPGActionPhase::ActionAbort, TEXT("액션 종료 절차 오류"));
	mActionPhase = ESRPGActionPhase::ActionEnd;

	UE_LOG(LogSRPGCombat, Log, TEXT("액션 종료"));

	// 액션 종료 로직
	OnEndAction();

	// 로그 작성
	GetWorldEventLogger(this)->EndActionLog();

	// 핸들러 등록 해제
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));
	CommandRouterModel->UnregisterCommandHandler(this);

	// 전투 상태 평가
	USRPGTurnContext* TurnContext = mParent.Get();
	checkf(TurnContext != nullptr, TEXT("턴 객체 nullptr"));
	USRPGCombatModel* CombatModel = TurnContext->GetParent();
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));
	CombatModel->EvaluateCombatStates();

	// 종료 연출 시작
	TSharedPtr<FPresentationBarrier> PresentationBarrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateWeakLambda(this, [this]() {
		USRPGTurnContext* TurnContext = mParent.Get();
		checkf(TurnContext != nullptr, TEXT("이미 제거된 턴에서 Action 종료 명령 오류"));
		TurnContext->OnEndCurrentAction(this, mActionResult);
		}));
	OnEndActionUI.Broadcast(PresentationBarrier, this, mActionResult);
}

void USRPGAction::TryBeginAction()
{
	if (mActionPhase == ESRPGActionPhase::ActionInit)
	{
		BeginAction();
	}
}

void USRPGAction::MarkActionCompleted(ESRPGActionResult Result)
{
	if (mActionPhase == ESRPGActionPhase::ActionPlay)
	{
		/* 정상 종료 시, 원하는 결과로 반영 */

		mActionPhase = ESRPGActionPhase::ActionAbort;
		mActionResult = Result;
	}
}

void USRPGAction::TryEndAction()
{
	if (mActionPhase == ESRPGActionPhase::ActionAbort)
	{
		EndAction();
	}
}

void USRPGAction::OnBeginAction()
{
}

void USRPGAction::OnTickAction(float DeltaTime)
{
}

void USRPGAction::OnEndAction()
{
}

void USRPGAction::EvaluateActionEndState(bool ForceAbort)
{
	if (mActionPhase != ESRPGActionPhase::ActionPlay)
	{
		return;
	}

	/* 외부에서 강제 종료되는가? */

	if (ForceAbort == true)
	{
		mActionResult = ESRPGActionResult::Cancelled;
		mActionPhase = ESRPGActionPhase::ActionAbort;
		return;
	}
}

int8 USRPGAction::GetCommandPriority() const
{
	return 0;
}

ESRPGCommandResult USRPGAction::HandleCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
	return ESRPGCommandResult::Ignored;
}

void USRPGAction::ReserveInitializeCommand(TInstancedStruct<FSRPGCommand> Command)
{
	mInitializeCommand = MoveTemp(Command);
}

void USRPGAction::GetTileActorUnderCursor(ECollisionChannel Channel, OUT AActor*& Actor, OUT FTileIndex& TileIndex) const
{
	Actor = nullptr;
	TileIndex = FTileIndex::Invalid;

	/* 마우스 포인트 지점 아래로 Raycast 검사 */

	USRPGTurnContext* TurnContext = mParent.Get();
	checkf(TurnContext != nullptr, TEXT("턴 객체 nullptr"));
	USRPGCombatModel* CombatModel = TurnContext->GetParent();
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController != nullptr)
	{
		FHitResult HitResult;
		if (PlayerController->GetHitResultUnderCursor(Channel, false, HitResult) == true)
		{
			Actor = HitResult.GetActor();
			if (Actor == CombatModel->GetTileMap()->GetView<AActor>())
			{
				TileIndex = CombatModel->GetTileMap()->WorldToTileIndex(HitResult.ImpactPoint);
			}
			else
			{
				IObjectView* ObjectView = Cast<IObjectView>(HitResult.GetActor());
				if (ObjectView != nullptr)
				{
					UBoardActorModel* BoardActorModel = ObjectView->GetModel<UBoardActorModel>();
					TileIndex = BoardActorModel->GetTileTransform().mIndex;
				}
			}
		}
	}
}

TWeakObjectPtr<USRPGTurnContext> USRPGAction::GetParent() const
{
	return mParent;
}

UUnitModel* USRPGAction::GetInstigator() const
{
	return mInstigator.Get();
}

ESRPGActionType USRPGAction::GetActionType() const
{
	return mActionType;
}

bool USRPGAction::ConsumesTurn() const
{
	return mConsumesTurn;
}


