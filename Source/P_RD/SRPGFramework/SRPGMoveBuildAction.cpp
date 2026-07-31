#include "SRPGFramework/SRPGMoveBuildAction.h"

#include "Actor/ActorView.h"
#include "RDCollision.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Pawn/Player/PlayerUnitModel.h"
#include "Actor/TileMap/TileMapModel.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"

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

        if (mMoveBuildPhase != ESRPGMoveBuildPhase::None)
        {
            MarkActionCompleted(ESRPGActionResult::Cancelled);
            SetBuildPhase(ESRPGMoveBuildPhase::None);
            Result = ESRPGCommandResult::Handled;
            return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
        }

        UPlayerUnitModel* PlayerUnitModel = Cast<UPlayerUnitModel>(mInstigator);
        checkf(PlayerUnitModel != nullptr, TEXT("플레이어 유닛 모델 nullptr"));
        UAttributeSetComponentModel* AttributeSetComponentModel = PlayerUnitModel->GetAttributeComponentModel();
        checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 모델 nullptr"));
        mMovePoint = AttributeSetComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetMovementAttribute());

        const FSRPGMoveSelectCommand& MoveSelectCommand = Command.Get<FSRPGMoveSelectCommand>();
        // 커맨드 델리깃을 복사해서 페이즈가 바뀔 때 통지
        OnChangeMoveBuildPhase = MoveSelectCommand.OnChangeMoveBuildPhase;

        EnterMoveBuild();
        return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
    }
    case ESRPGCommandType::SkillSelect:
    {
        /* 이동 대상 선택 중 스킬을 누르면 이동 빌드를 취소한다(기획: 스킬↔이동 전환 시 이전 것 취소).
         * Ignored를 반환해, 라우터(USRPGActionCreationCommandHandler)가 이 SkillSelect로 스킬 빌드를 새로 생성하게 둔다. */

        MarkActionCompleted(ESRPGActionResult::Cancelled);
        SetBuildPhase(ESRPGMoveBuildPhase::None);
        return ESRPGCommandResult::Ignored;
    }
    case ESRPGCommandType::TurnEnd:
    {
        /* 이동 조준 중 엔드턴을 누르면 이동 빌드를 먼저 취소한다(스킬 빌드와 동일한 꼬임 방지).
         * Ignored를 반환해, 라우터가 이 TurnEnd로 TurnEndAction을 정상 생성하게 둔다. */

        MarkActionCompleted(ESRPGActionResult::Cancelled);
        SetBuildPhase(ESRPGMoveBuildPhase::None);
        return ESRPGCommandResult::Ignored;
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
    if (WorldTraceCommand.mIsLongPress == true)
    {
        return Result;
    }

    UTileMapModel* TileMap = GetTileMap();

    AActor* TargetActor = nullptr;
    FTileIndex TargetTileIndex = FTileIndex::Invalid;
    GetTileActorUnderCursor(GetWorld(), RDTraceChannels::TileOnlyTrace, WorldTraceCommand.mScreenPosition, OUT TargetActor, OUT TargetTileIndex);

    IActorView* ActorView = Cast<IActorView>(TargetActor);
    const bool IsContactedTileMap = ActorView != nullptr && ActorView->GetModel() == TileMap;
    // 타일맵을 아예 벗어난 탭(IsContactedTileMap=false)도 무효 타일과 똑같이 "타일 밖" 취소로 취급한다
    // (기획: 이동 대상 선택 중 타일 밖을 누르면 취소). TargetTileIndex가 유효하면 정상 처리로 간다.
    if (IsContactedTileMap == true || TargetTileIndex == FTileIndex::Invalid)
    {
        if (TargetTileIndex == FTileIndex::Invalid)
        {
            /* 한단계 취소작업 (무효 타일 또는 타일 밖 탭) */

            switch (mMoveBuildPhase)
            {
            case ESRPGMoveBuildPhase::Preview:
            {
                /* 프리뷰 단계에서 한단계 취소 시, 마지막 경유지 취소 */

                RemoveLastWaypoint();
                Result = ESRPGCommandResult::Handled;
                break;
            }
            case ESRPGMoveBuildPhase::DestSelection:
            {
                /* 목적지 선택 단계에서 한단계 취소 시, 빌드 자체 종료 */

                MarkActionCompleted(ESRPGActionResult::Cancelled);
                SetBuildPhase(ESRPGMoveBuildPhase::None);
                Result = ESRPGCommandResult::Handled;
                break;
            }
            }
        }
        else
        {
            /* 클릭 타일에 따라 확정 / 경유지 추가 / 취소 분기 */

            switch (mMoveBuildPhase)
            {
            case ESRPGMoveBuildPhase::Preview:
            {
                /* 마지막 경유지(도착 후보) 재클릭 시, 이동 발행 */

                if (GetLastWaypoint() == TargetTileIndex)
                {
                    BuildMove();
                    MarkActionCompleted(ESRPGActionResult::Succeeded);
                    SetBuildPhase(ESRPGMoveBuildPhase::Build);
                    Result = ESRPGCommandResult::Handled;
                    break;
                }

                // 중간 경유지 재클릭도 잘라내지 않고 아래에서 새 경유지로 추가 (왔다갔다 이동 허용)
                [[fallthrough]];
            }
            case ESRPGMoveBuildPhase::DestSelection:
            {
                /* 도달 가능한 칸 클릭 시, 경유지 추가 (마지막 경유지 위치 재클릭은 위에서 처리, 출발 타일 복귀 허용) */

                if (TargetTileIndex != GetLastWaypoint() && mReachableTileIndexes.Contains(TargetTileIndex) == true)
                {
                    AddWaypoint(TargetTileIndex);
                    Result = ESRPGCommandResult::Handled;
                    break;
                }

                /* 범위 밖 유효 타일 클릭 시, 마지막 경유지 취소 (경유지가 있는 프리뷰 단계만) */

                if (mMoveBuildPhase == ESRPGMoveBuildPhase::Preview)
                {
                    RemoveLastWaypoint();
                    Result = ESRPGCommandResult::Handled;
                }
                break;
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

    mPathSegments.Empty();

    UTileMapModel* TileMap = GetTileMap();
    TileMap->ClearTileHighlight(ETileHighlightFlag::Aim | ETileHighlightFlag::Effect | ETileHighlightFlag::Select);
    TileMap->ClearMovePath();

    SetBuildPhase(ESRPGMoveBuildPhase::None);
}

void USRPGMoveBuildAction::AddWaypoint(const FTileIndex& TileIndex)
{
    UE_LOG(LogTemp, Warning, TEXT("[화살표] AddWaypoint 진입 tile=(%d,%d) phase=%d"), TileIndex.mX, TileIndex.mY, (int32)mMoveBuildPhase);
    checkf(mMoveBuildPhase == ESRPGMoveBuildPhase::DestSelection || mMoveBuildPhase == ESRPGMoveBuildPhase::Preview, TEXT("이동 빌드 순서 오류"));

    UTileMapModel* TileMap = GetTileMap();

    /* 부분경로 계산 및 프리뷰 표시 — 빌드 시점에 확정한 경로를 실행까지 그대로 사용(심=라이브 보장) */

    {
        // 자기 자신은 점유 판정에서 무시해 출발 타일로 되돌아오는 경로도 허용
        mPathSegments.Add(TileMap->FindPath(GetLastWaypoint(), TileIndex, mInstigator.Get()));
        RefreshPathPreview();
        RefreshReachableTiles();
    }

    /* 예측 시스템 — 이동 시 '받는' 영향(경로/도착 타일의 함정·장판 데미지·상태이상) 예측 */

    {
        // 스킬의 CalculateSkillResult(가하는 데미지)의 역방향.
        // 받는 효과의 실제 적용은 실행 시 도착·경유 타일의 OnBeginTileOverlap이 담당.
        // 심 복제본에서 이 경로 이동을 실행해 출발/도착 스냅샷(MakeSnapshotData) 비교 또는
        // EventLog(mAttributeEffectEventLogs)로 스탯 델타를 산출해 프리뷰로 표시.
        // TODO : 이동 예측 계산 연결
    }

    /* 상태 변경되면서 외부에서 바인딩된 UI 변경 */

    SetBuildPhase(ESRPGMoveBuildPhase::Preview);
}

void USRPGMoveBuildAction::RemoveLastWaypoint()
{
    checkf(mMoveBuildPhase == ESRPGMoveBuildPhase::Preview, TEXT("이동 빌드 순서 오류"));

    /* 마지막 부분경로 제거 후 프리뷰, 도달 범위 갱신 */

    mPathSegments.Pop();
    RefreshPathPreview();
    RefreshReachableTiles();

    // 남은 경유지가 없으면 목적지 선택 단계로 복귀 (도달 범위 강조는 유지)
    SetBuildPhase(mPathSegments.IsEmpty() ? ESRPGMoveBuildPhase::DestSelection : ESRPGMoveBuildPhase::Preview);
}

void USRPGMoveBuildAction::BuildMove()
{
    checkf(mMoveBuildPhase == ESRPGMoveBuildPhase::Preview, TEXT("이동 빌드 순서 오류"));

    USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
    checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 서브시스템 모델 nullptr"));

    /* 부분경로들을 하나의 경로로 이어붙여 이동 액션 생성 명령 발행 */

    TArray<FTileIndex> PathTileIndexes;
    for (int32 SegmentIndex = 0; SegmentIndex < mPathSegments.Num(); ++SegmentIndex)
    {
        const TArray<FTileIndex>& Segment = mPathSegments[SegmentIndex];

        // 두 번째 부분경로부터는 첫 타일이 앞 부분경로의 끝 타일과 같으므로 제외
        for (int32 Index = (SegmentIndex == 0 ? 0 : 1); Index < Segment.Num(); ++Index)
        {
            PathTileIndexes.Add(Segment[Index]);
        }
    }

    TInstancedStruct<FSRPGCommand> MoveCastCommand;
    MoveCastCommand.InitializeAs<FSRPGMoveCommand>();
    MoveCastCommand.GetMutable<FSRPGMoveCommand>().mPathTileIndexes = PathTileIndexes;

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

    // 이동 거리 = 남은 이동 포인트 (경유지를 추가할수록 줄어듦), 기준 = 마지막 경유지
    // 자기 자신은 점유 판정에서 무시해 출발 타일 복귀 허용
    const int32 MoveRange = GetRemainMovePoint();
    mReachableTileIndexes = TileMap->GetReachableTiles(GetLastWaypoint(), MoveRange, mInstigator.Get());

    // 도달 범위를 조준 강조로 표시 (이동 범위 = 조준 범위로 표현)
    TileMap->SetTileHighlight(mReachableTileIndexes, ETileHighlightFlag::Aim);
}

FTileIndex USRPGMoveBuildAction::GetLastWaypoint() const
{
    // 부분경로가 없으면 유닛 현재 위치가 출발 기준
    if (mPathSegments.IsEmpty())
    {
        return mInstigator->GetTileTransform().mIndex;
    }
    return mPathSegments.Last().Last();
}

int32 USRPGMoveBuildAction::GetPlannedMoveCost() const
{
    int32 UsedMovePoint = 0;
    for (const TArray<FTileIndex>& Segment : mPathSegments)
    {
        UsedMovePoint += FMath::Max(Segment.Num() - 1, 0);
    }
    return UsedMovePoint;
}

int32 USRPGMoveBuildAction::GetRemainMovePoint() const
{
    return FMath::Max(mMovePoint - GetPlannedMoveCost(), 0);
}

void USRPGMoveBuildAction::RefreshPathPreview()
{
    UTileMapModel* TileMap = GetTileMap();

    UE_LOG(LogTemp, Warning, TEXT("[화살표] RefreshPathPreview 조각=%d"), mPathSegments.Num());

    // 부분경로가 없으면 프리뷰 해제
    if (mPathSegments.IsEmpty())
    {
        TileMap->ClearMovePath();
        return;
    }

    /* 부분경로들을 하나의 표시용 경로로 이어붙임 */

    TArray<FMovePathTile> PathTiles;
    for (int32 SegmentIndex = 0; SegmentIndex < mPathSegments.Num(); ++SegmentIndex)
    {
        const TArray<FTileIndex>& Segment = mPathSegments[SegmentIndex];

        // 두 번째 부분경로부터는 첫 타일이 앞 부분경로의 끝 타일과 같으므로 제외
        for (int32 Index = (SegmentIndex == 0 ? 0 : 1); Index < Segment.Num(); ++Index)
        {
            // 부분경로의 끝 타일 = 경유지 (단, 마지막 부분경로의 끝 = 도착 후보라 제외)
            const bool IsWaypoint = (Index == Segment.Num() - 1) && (SegmentIndex < mPathSegments.Num() - 1);
            PathTiles.Emplace(Segment[Index], IsWaypoint);
        }
    }

    TileMap->SetMovePath(PathTiles);
}
