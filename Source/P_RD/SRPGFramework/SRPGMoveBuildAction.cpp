#include "SRPGFramework/SRPGMoveBuildAction.h"

#include "Actor/ActorView.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Pawn/UnitModel.h"
#include "Actor/TileMap/TileMapModel.h"

#include "SRPGFramework/SRPGMoveAction.h"

FSRPGMoveSelectCommand::FSRPGMoveSelectCommand()
{
    mCommandType = ESRPGCommandType::MoveSelect;
    mRequestedAction = USRPGMoveBuildAction::StaticClass();
}

USRPGMoveBuildAction::USRPGMoveBuildAction()
{
    mActionType = ESRPGActionType::BuildAction;
    mConsumesTurn = false;
}

void USRPGMoveBuildAction::OnBeginAction()
{
    Super::OnBeginAction();
}

void USRPGMoveBuildAction::OnEndAction()
{
    ResetMoveBuild();
    Super::OnEndAction();
}

ESRPGCommandResult USRPGMoveBuildAction::HandleCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
    ESRPGCommandResult Result = Super::HandleCommand(Command);
    if (Result == ESRPGCommandResult::Handled)
    {
        return Result;
    }

    switch (Command.Get().GetCommandType())
    {
    case ESRPGCommandType::MoveSelect:
    {
        /* 이동 빌드 진입 시 외부에서 넘어온 이동 포인트를 받아 도달 범위 계산 */

        const FSRPGMoveSelectCommand& MoveSelectCommand = Command.Get<FSRPGMoveSelectCommand>();
        mMovePoint = MoveSelectCommand.mMovePoint;

        if (mMoveBuildPhase == ESRPGMoveBuildPhase::None)
        {
            EnterMoveBuild();
        }
        return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
    }
    case ESRPGCommandType::WorldTrace:
    {
        /* 월드 공간 터치 시 선택 위치에 따라서 결정 */

        return CombineSRPGCommandResult(HandleWorldTraceCommand(Command), Result);
    }
    }

    return ESRPGCommandResult::Ignored;
}

ESRPGCommandResult USRPGMoveBuildAction::HandleWorldTraceCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
    ESRPGCommandResult Result = ESRPGCommandResult::Ignored;

    const FSRPGWorldTraceCommand& WorldTraceCommand = Command.Get<FSRPGWorldTraceCommand>();
    if (WorldTraceCommand.mIsLongPress == false)
    {
        UTileMapModel* TileMap = GetTileMap();

        AActor* TargetActor = nullptr;
        FTileIndex TargetTileIndex = FTileIndex::Invalid;
        GetTileActorUnderCursor(ECC_GameTraceChannel1 /* TODO : 임시 */, OUT TargetActor, OUT TargetTileIndex);

        IActorView* ActorView = Cast<IActorView>(TargetActor);
        if (ActorView != nullptr && ActorView->GetModel() == TileMap)
        {
            if (TargetTileIndex == FTileIndex::Invalid)
            {
                /* 한단계 취소작업 */

                switch (mMoveBuildPhase)
                {
                case ESRPGMoveBuildPhase::Preview:
                {
                    /* 프리뷰 단계에서 한단계 취소 시, 목적지 취소 처리 */

                    ResetTargetTile();
                    Result = ESRPGCommandResult::Handled;
                    break;
                }
                case ESRPGMoveBuildPhase::DestSelection:
                {
                    /* 목적지 선택 단계에서 한단계 취소 시, 빌드 자체 종료 */

                    MarkActionCompleted(ESRPGActionResult::Cancelled);
                    Result = ESRPGCommandResult::Handled;
                    break;
                }
                }
            }
            else
            {
                /* 한단계 처리작업 */

                switch (mMoveBuildPhase)
                {
                case ESRPGMoveBuildPhase::Preview:
                {
                    /* 확정 칸 클릭 시, 이동 발행 */

                    if (mTargetIndex == TargetTileIndex)
                    {
                        BuildMove();
                        MarkActionCompleted(ESRPGActionResult::Succeeded);
                        Result = ESRPGCommandResult::Handled;
                        break;
                    }
                    [[fallthrough]];
                }
                case ESRPGMoveBuildPhase::DestSelection:
                {
                    /* 도달 가능한 칸 클릭 시, 프리뷰 단계까지 보여주기 */

                    if (mReachableTileIndexes.Contains(TargetTileIndex) == true)
                    {
                        ResetTargetTile();
                        SetTargetTile(TargetTileIndex);
                        Result = ESRPGCommandResult::Handled;
                        break;
                    }
                    break;
                }
                }
            }
        }
    }

    return Result;
}

void USRPGMoveBuildAction::EnterMoveBuild()
{
    checkf(mMoveBuildPhase == ESRPGMoveBuildPhase::None, TEXT("이동 빌드 순서 오류"));

    // 이동 스킬은 '이동'으로 고정이라 별도 조회 없이, 외부에서 받은 이동 포인트로 도달 범위를 강조한다

    /* 도달 범위 계산 및 강조 */

    RefreshReachableTiles();

    /* 상태 변경되면서 외부에서 바인딩된 UI 변경 */

    SetBuildPhase(ESRPGMoveBuildPhase::DestSelection);
}

void USRPGMoveBuildAction::ResetMoveBuild()
{
    mReachableTileIndexes.Empty();

    mMovePoint = 0;

    mPathTileIndexes.Empty();
    mTargetIndex = FTileIndex::Invalid;

    UTileMapModel* TileMap = GetTileMap();
    TileMap->ClearTileHighlight(ETileHighlightFlag::Aim | ETileHighlightFlag::Effect | ETileHighlightFlag::Select);
    TileMap->ClearMovePath();

    SetBuildPhase(ESRPGMoveBuildPhase::None);
}

void USRPGMoveBuildAction::SetTargetTile(const FTileIndex& TileIndex)
{
    checkf(mMoveBuildPhase == ESRPGMoveBuildPhase::DestSelection, TEXT("이동 빌드 순서 오류"));

    UTileMapModel* TileMap = GetTileMap();

    /* 경로 계산 및 프리뷰 표시 — 빌드 시점에 확정한 경로를 실행까지 그대로 사용(심=라이브 보장) */

    {
        const FTileIndex Origin = mInstigator->GetTileTransform().mIndex;
        mTargetIndex = TileIndex;
        mPathTileIndexes = TileMap->FindPath(Origin, TileIndex);
        TileMap->SetMovePath(Origin, TileIndex);
    }

    /* 예측 시스템 — 이동 시 '받는' 영향(경로/도착 타일의 함정·장판 데미지·상태이상) 예측 */

    {
        // 스킬의 CalculateSkillResult(가하는 데미지)의 역방향이다.
        // 받는 효과의 실제 적용은 실행 시 도착·경유 타일의 OnBeginTileOverlap이 담당한다(현재 스텁).
        // 시뮬레이션이 완성되면, 심 복제본에서 이 경로 이동을 실행해 출발/도착 스냅샷(MakeSnapshotData)을
        // 비교하거나 EventLog(mAttributeEffectEventLogs)로 스탯 델타를 산출해 프리뷰로 보여준다.
        // TODO : 시뮬레이션·속성·타일효과 시스템 연결 후 구현 (상대 API가 전부 스텁/null이라 현재 호출 불가)
    }

    /* 상태 변경되면서 외부에서 바인딩된 UI 변경 */

    SetBuildPhase(ESRPGMoveBuildPhase::Preview);
}

void USRPGMoveBuildAction::ResetTargetTile()
{
    mPathTileIndexes.Empty();
    mTargetIndex = FTileIndex::Invalid;

    // 경로 프리뷰만 해제 (도달 범위 강조는 유지)
    GetTileMap()->ClearMovePath();

    SetBuildPhase(ESRPGMoveBuildPhase::DestSelection);
}

void USRPGMoveBuildAction::BuildMove()
{
    checkf(mMoveBuildPhase == ESRPGMoveBuildPhase::Preview, TEXT("이동 빌드 순서 오류"));

    USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
    checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 서브시스템 모델 nullptr"));

    /* 확정된 경로를 실어 이동 액션 생성 명령 발행 */

    TInstancedStruct<FSRPGCommand> MoveCastCommand;
    MoveCastCommand.InitializeAs<FSRPGMoveCommand>();
    MoveCastCommand.GetMutable<FSRPGMoveCommand>().mPathTileIndexes = mPathTileIndexes;

    CommandRouterModel->SummitCommand(MoveCastCommand);
}

void USRPGMoveBuildAction::SetBuildPhase(ESRPGMoveBuildPhase BuildPhase)
{
    mMoveBuildPhase = BuildPhase;
    OnChangeMoveBuildPhase.Broadcast(this, BuildPhase);
}

UTileMapModel* USRPGMoveBuildAction::GetTileMap() const
{
    // 전투 모델을 월드 서브시스템에서 바로 받아 타일 맵을 꺼낸다 (턴 컨텍스트 체인 의존 제거)
    USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
    checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

    UTileMapModel* TileMap = CombatModel->GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));
    return TileMap;
}

void USRPGMoveBuildAction::RefreshReachableTiles()
{
    UTileMapModel* TileMap = GetTileMap();

    // 이동 거리 = 외부에서 넘어온 이동 포인트 (사거리 계산은 빌드 밖에서 수행)
    const int32 MoveRange = mMovePoint;
    mReachableTileIndexes = TileMap->GetReachableTiles(mInstigator->GetTileTransform().mIndex, MoveRange);

    // 도달 범위를 조준 강조로 표시 (이동 범위 = 조준 범위로 표현)
    TileMap->SetTileHighlight(mReachableTileIndexes, ETileHighlightFlag::Aim);
}
