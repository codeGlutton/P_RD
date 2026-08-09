#include "SRPGFramework/SRPGMoveAction.h"

#include "Pawn/UnitModel.h"
#include "Component/BoardMovementComponent/BoardMovementComponentModel.h"

FSRPGMoveCommand::FSRPGMoveCommand()
{
    mCommandType = ESRPGCommandType::MoveCast;
    mRequestedAction = USRPGMoveAction::StaticClass();
}

USRPGMoveAction::USRPGMoveAction()
{
    mActionType = ESRPGActionType::InPlayAction;
    mConsumesTurn = false;
}

ESRPGCommandResult USRPGMoveAction::HandleCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
    ESRPGCommandResult Result = Super::HandleCommand(Command);
    if (Result == ESRPGCommandResult::Handled)
    {
        return Result;
    }

    /* 생성 시 예약된 이동 명령에서 확정 경로를 수신 (빌드 액션이 실어 보낸 경로) */
    if (Command.Get().GetCommandType() == ESRPGCommandType::MoveCast)
    {
        mPathTileIndexes = Command.Get<FSRPGMoveCommand>().mPathTileIndexes;
        return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
    }

    return ESRPGCommandResult::Ignored;
}

void USRPGMoveAction::OnBeginAction()
{
    Super::OnBeginAction();

    // 경로가 없거나 시작 타일만 있으면 이동 없이 즉시 종료
    if (mPathTileIndexes.Num() < 2)
    {
        MarkActionCompleted(ESRPGActionResult::Succeeded);
        return;
    }

    // 실제 이동은 컴포넌트 모델이 처리, 액션은 완료를 기다렸다가 턴 시스템에 보고
    // 액션이 먼저 파괴될 수 있으므로 WeakLambda로 보호.
    UBoardMovementComponentModel* MovementCompModel = mInstigator->GetBoardMovementComponentModel();
    checkf(MovementCompModel != nullptr, TEXT("이동 컴포넌트 모델 nullptr"));

    const bool Started = MovementCompModel->MoveAlongPath(
        mPathTileIndexes,
        FOnBoardMoveFinished::CreateWeakLambda(this, [this]() {
            MarkActionCompleted(ESRPGActionResult::Succeeded);
            }));
    checkf(Started == true, TEXT("이동 시작 실패 (이미 이동 중이거나 경로 오류)"));
}

void USRPGMoveAction::OnEndAction()
{
    Super::OnEndAction();

    // 이동이 진행 중인 채로 액션이 끝나면(중단) 컴포넌트 모델도 정지
    UBoardMovementComponentModel* MovementCompModel = mInstigator.IsValid() ? mInstigator->GetBoardMovementComponentModel() : nullptr;
    if (MovementCompModel != nullptr && MovementCompModel->IsMoving() == true)
    {
        MovementCompModel->CancelMove();
    }
}

