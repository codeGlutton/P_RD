#include "SRPGFramework/SRPGAction.h"
#include "SRPGFramework/SRPGTurnContext.h"
#include "SRPGFramework/SRPGCommand.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterSubsystem.h"
#include "Singleton/WorldSubsystem/PresentationSyncSubsystem.h"

#include "Pawn/Unit.h"
#include "Actor/TileMap/TileMap.h"

#include "FunctionLibrary/GASTargetFunctionLibrary.h"

void FSRPGAction::InitAction(TSharedRef<FSRPGTurnContext> Parent, AUnit* Instigator)
{
	checkf(Instigator != nullptr, TEXT("유발자 유닛 nullptr"));

	checkf(mActionPhase == ESRPGActionPhase::None, TEXT("중복 초기화"));
	mActionPhase = ESRPGActionPhase::ActionInit;

	mParent = Parent;
	mInstigator = Instigator;
}

void FSRPGAction::BeginAction()
{
	checkf(mActionPhase == ESRPGActionPhase::ActionInit, TEXT("이미 액션 진행 중에 재실행 오류"));
	mActionPhase = ESRPGActionPhase::ActionStart;

	// 시작 연출 시작
	UPresentationSyncSubsystem* PresentationSyncSubsystem = GetWorld()->GetSubsystem<UPresentationSyncSubsystem>();
	checkf(PresentationSyncSubsystem != nullptr, TEXT("연출 동기화 서브시스템 nullptr"));
	auto PresentationBarrier = PresentationSyncSubsystem->MakePresentationBarrier(FOnFinishPresentation::CreateSPLambda(AsShared(), [this]() {
		checkf(mActionPhase == ESRPGActionPhase::ActionStart, TEXT("액션 실행 절차 오류"));
		mActionPhase = ESRPGActionPhase::ActionPlay;

		// 핸들러 등록
		USRPGCommandRouterSubsystem* CommandRouterSubsystem = GetWorld()->GetSubsystem<USRPGCommandRouterSubsystem>();
		checkf(CommandRouterSubsystem != nullptr, TEXT("명령 라우터 서브시스템 nullptr"));
		CommandRouterSubsystem->RegisterCommandHandler(AsShared());
		// 예약된 초기화 커맨드 시작
		FlushCommands();
		
		// 액션 시작 로직
		OnBeginAction();
		}));
	OnBeginActionUI.Broadcast(PresentationBarrier, *this);
}

void FSRPGAction::TickAction(float DeltaTime)
{
	OnTickAction(DeltaTime);
}

void FSRPGAction::EndAction(ESRPGActionResult Result)
{
	if (mActionPhase == ESRPGActionPhase::ActionPlay)
	{
		/* 정상 종료 시, 원하는 결과로 반영 */

		mActionPhase = ESRPGActionPhase::ActionAbort;
		mActionResult = Result;
	}

	checkf(mActionPhase == ESRPGActionPhase::ActionAbort, TEXT("액션 종료 절차 오류"));
	mActionPhase = ESRPGActionPhase::ActionEnd;

	// 액션 종료 로직
	OnEndAction();

	// 핸들러 등록 해제
	USRPGCommandRouterSubsystem* CommandRouterSubsystem = GetWorld()->GetSubsystem<USRPGCommandRouterSubsystem>();
	checkf(CommandRouterSubsystem != nullptr, TEXT("명령 라우터 서브시스템 nullptr"));
	CommandRouterSubsystem->UnregisterCommandHandler(AsShared());

	TSharedPtr<FSRPGTurnContext> TurnContext = mParent.Pin();
	checkf(TurnContext != nullptr, TEXT("턴 객체 nullptr"));
	USRPGCombatModel* CombatModel = TurnContext->GetParent();
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	// 전투 상태 평가
	CombatModel->EvaluateCombatStates();

	// 종료 연출 시작
	UPresentationSyncSubsystem* PresentationSyncSubsystem = GetWorld()->GetSubsystem<UPresentationSyncSubsystem>();
	checkf(PresentationSyncSubsystem != nullptr, TEXT("연출 동기화 서브시스템 nullptr"));
	auto PresentationBarrier = PresentationSyncSubsystem->MakePresentationBarrier(FOnFinishPresentation::CreateSPLambda(AsShared(), [this]() {
		TSharedPtr<FSRPGTurnContext> TurnContext = mParent.Pin();
		checkf(TurnContext != nullptr, TEXT("이미 제거된 턴에서 Action 종료 명령 오류"));
		TurnContext->OnEndCurrentAction(AsShared(), mActionResult);
		}));
	OnEndActionUI.Broadcast(PresentationBarrier, *this, mActionResult);
}

void FSRPGAction::OnBeginAction()
{
}

void FSRPGAction::OnTickAction(float DeltaTime)
{
}

void FSRPGAction::OnEndAction()
{
}

void FSRPGAction::EvaluateActionEndState(bool ForceAbort)
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

int8 FSRPGAction::GetCommandPriority() const
{
	return 0;
}

ESRPGCommandResult FSRPGAction::HandleCommand(TSharedPtr<const FSRPGCommand> Command)
{
	return ESRPGCommandResult::Ignored;
}

void FSRPGAction::ReserveCommand(TSharedPtr<const FSRPGCommand> Command)
{
	mReservedCommands.Dequeue(Command);
}

void FSRPGAction::FlushCommands()
{
	while (mActionPhase == ESRPGActionPhase::ActionPlay && mReservedCommands.IsEmpty() == false)
	{
		TSharedPtr<const FSRPGCommand> Command;
		mReservedCommands.Dequeue(Command);
		HandleCommand(Command);
	}
}

void FSRPGAction::GetTileActorUnderCursor(ECollisionChannel Channel, OUT AActor* Actor, OUT FTileIndex& TileIndex) const
{
	Actor = nullptr;
	TileIndex = FTileIndex::Invalid;

	/* 마우스 포인트 지점 아래로 Raycast 검사 */

	TSharedPtr<FSRPGTurnContext> TurnContext = mParent.Pin();
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
			if (Actor == CombatModel->GetTileMap())
			{
				TileIndex = CombatModel->GetTileMap()->WorldToTileIndex(HitResult.ImpactPoint);
			}
			else
			{
				ITileActor* TileActor = Cast<ITileActor>(HitResult.GetActor());
				if (TileActor != nullptr)
				{
					TileIndex = TileActor->GetTileTransform().mIndex;
				}
			}
		}
	}
}

UWorld* FSRPGAction::GetWorld() const
{
	TSharedPtr<FSRPGTurnContext> TurnContext = GetParent().Pin();
	if (TurnContext != nullptr)
	{
		return TurnContext->GetWorld();
	}
	return nullptr;
}

TWeakPtr<FSRPGTurnContext> FSRPGAction::GetParent() const
{
	return mParent;
}

AUnit* FSRPGAction::GetInstigator() const
{
	return mInstigator.Get();
}

ESRPGActionType FSRPGAction::GetActionType() const
{
	return mActionType;
}

bool FSRPGAction::ConsumesTurn() const
{
	return mConsumesTurn;
}


