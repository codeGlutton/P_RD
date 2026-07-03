#include "SRPGFramework/SRPGMoveAction.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"

#include "Pawn/UnitModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Actor/TileMap/TileMapModel.h"

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

    // 1번 타일로 이동하는 것부터 시작 (0번은 현재 타일).
    // 해당 타일로 이동이 완료되면, 베리어가 끝나고 다시 다음 스텝으로 가는 걸 마지막 타일까지 반복.
    // 시뮬레이션모드에서는 베리어가 없으므로 즉각 다음 스텝으로 진행하므로 문제 없음
    StartStep(1);
}

void USRPGMoveAction::OnEndAction()
{
    Super::OnEndAction();

    /* 이동을 정상 완료한 경우, 사용한 이동 포인트를 이동 유닛에게 차감 통지한다 */

    if (mActionResult == ESRPGActionResult::Succeeded && mPathTileIndexes.Num() >= 2)
    {
        // 소모 이동 포인트 = 밟은 칸 수 (경로 칸 수 - 시작 타일)
        const int32 SpentPoint = mPathTileIndexes.Num() - 1;

        // 이동 유닛의 스킬 컴포넌트에 소모 이동 포인트 차감 통지
        if (USkillComponentModel* SkillComp = mInstigator->GetSkillComponentModel())
        {
            //SkillComp->UseMovePoint(SpentPoint);
        }
    }
}

void USRPGMoveAction::StartStep(int32 StepIndex)
{
    checkf(mPathTileIndexes.IsValidIndex(StepIndex) == true, TEXT("이동 경로 인덱스 오류"));
    mCurrentStepIndex = StepIndex;

    UTileMapModel* TileMap = GetTileMap();

    // 직전 타일에서 이번 타일을 바라볼때의 방향 계산
    // 직전->현재와 현재->다음 방향을 보간해서 자연스럽게 코너링 할 계획
    const ETileActorDirection Direction = UTileMapModel::TileDeltaToDirection(
        mPathTileIndexes[StepIndex - 1],
        mPathTileIndexes[StepIndex],
        mInstigator->GetTileTransform().mDirection);

    // 다음 타일로 이동 (모델의 논리적 위치 변경)
    // 점유는 즉시 하고, 도착 오버랩 통지는 CompleteStep가 함
    const FTileTransform NextTransform(mPathTileIndexes[StepIndex], Direction);
    TileMap->StartActorMovement(NextTransform, mInstigator.Get());

    // 이동 후 받을 베리어 생성
    // 액션이 먼저 파괴될 수 있으므로 WeakLambda로 보호.
    TSharedPtr<FPresentationBarrier> Barrier = FPresentationBarrier::Make(
        FOnFinishPresentation::CreateWeakLambda(this, [this]() {
            OnStepPresentationFinished();
            }));
    
    // OnStartMoveStep을 구독하고 있던 뷰가 이동 시작 (뷰의 물리적 위치 변경)
    mInstigator->OnStartMoveStep.Broadcast(NextTransform, TileMap->TileToWorldTransform(NextTransform), Barrier);
}

void USRPGMoveAction::CompleteStep()
{
    // 현재(도착) 타일의 오버랩 통지 — 함정/장판 등 타일 효과가 여기서 발동
    GetTileMap()->CompleteActorMovement(mInstigator.Get());
}

void USRPGMoveAction::OnStepPresentationFinished()
{
    // ActionPlay 상태가 아니면 중단됐다고 보고 바로 리턴
    if (mActionPhase != ESRPGActionPhase::ActionPlay)
    {
        return;
    }

    // 현재 타일 도착 처리 -> 함정/장판 등 오버랩 관련된 처리
    CompleteStep();

    // 1) 마지막 타일이면 이동 완료.
    if (mCurrentStepIndex >= mPathTileIndexes.Num() - 1)
    {
        MarkActionCompleted(ESRPGActionResult::Succeeded);
    }
    // 2) 마지막 타일이 아니면 다음 타일로 이동
    else
    {
        StartStep(mCurrentStepIndex + 1);
    }
}

UTileMapModel* USRPGMoveAction::GetTileMap() const
{
    // 전투 모델을 월드 서브시스템에서 바로 받아 타일 맵을 꺼낸다 (턴 컨텍스트 체인 의존 제거)
    USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
    checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

    UTileMapModel* TileMap = CombatModel->GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));
    return TileMap;
}
