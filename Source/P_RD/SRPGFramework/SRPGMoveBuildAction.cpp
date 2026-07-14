#include "SRPGFramework/SRPGMoveBuildAction.h"

#include "Actor/ActorView.h"
#include "RDCollision.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Pawn/Player/PlayerUnitModel.h"
#include "Actor/TileMap/TileMapModel.h"

#include "Component/SkillComponent/SkillComponentModel.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_GetMove.h"
#include "Dice/DicePoolModel.h"

#include "SRPGFramework/SRPGMoveAction.h"
#include "SRPGFramework/SRPGSkillBuildAction.h"

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
        /* 이동 스킬 선택 시 주사위가 비어 있는 상태로 빌드에 진입한다. */

        if (mMoveBuildPhase != ESRPGMoveBuildPhase::None)
        {
            MarkActionCompleted(ESRPGActionResult::Cancelled);
            SetBuildPhase(ESRPGMoveBuildPhase::None);
            Result = ESRPGCommandResult::Handled;
            return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
        }

        const FSRPGMoveSelectCommand& MoveSelectCommand = Command.Get<FSRPGMoveSelectCommand>();
        mMovementSkillIndex = MoveSelectCommand.mSkillIndex;
        mMovePoint = 0;
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
    case ESRPGCommandType::DiceSelect:
    {
        const FSRPGDiceSelectCommand& DiceSelectCommand = Command.Get<FSRPGDiceSelectCommand>();
        const bool IsMoveBuildActive = mMoveBuildPhase == ESRPGMoveBuildPhase::DestSelection
            || mMoveBuildPhase == ESRPGMoveBuildPhase::Preview;
        if (DiceSelectCommand.mDiceIndex != INDEX_NONE && IsMoveBuildActive)
        {
            // 주사위가 바뀌면 기존 목적지/경로는 더 이상 유효하지 않다.
            if (mMoveBuildPhase == ESRPGMoveBuildPhase::Preview)
            {
                ResetTargetTile();
            }
            ChangeDices(DiceSelectCommand.mDiceIndex);
            mMovePoint = CalculateMovePoint();
            RefreshReachableTiles();
            // 같은 페이즈라도 거리 표시값이 바뀌었으므로 UI에 다시 알린다.
            SetBuildPhase(ESRPGMoveBuildPhase::DestSelection);
        }
        return ESRPGCommandResult::Handled;
    }
    case ESRPGCommandType::BuildConfirm:
    {
        if (mMoveBuildPhase == ESRPGMoveBuildPhase::Preview && BuildMove())
        {
            SetBuildPhase(ESRPGMoveBuildPhase::Build);
            MarkActionCompleted(ESRPGActionResult::Succeeded);
        }
        return ESRPGCommandResult::Handled;
    }
    case ESRPGCommandType::BuildCancel:
    {
        if (mMoveBuildPhase == ESRPGMoveBuildPhase::DestSelection
            || mMoveBuildPhase == ESRPGMoveBuildPhase::Preview)
        {
            MarkActionCompleted(ESRPGActionResult::Cancelled);
            SetBuildPhase(ESRPGMoveBuildPhase::None);
        }
        return ESRPGCommandResult::Handled;
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
                /* 프리뷰 단계에서 한단계 취소 시, 목적지 취소 처리 */

                ResetTargetTile();
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
            /* 한단계 처리작업 */

            switch (mMoveBuildPhase)
            {
            case ESRPGMoveBuildPhase::Preview:
            {
                // 같은 칸을 다시 눌러도 실행하지 않는다. 명시적 BuildConfirm만 이동을 확정한다.
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
    if (UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get()))
    {
        if (UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel())
        {
            DicePoolModel->ResetSelected();
        }
    }

    mReachableTileIndexes.Empty();

    mMovePoint = 0;
    mMovementSkillIndex = INDEX_NONE;

    mPathTileIndexes.Empty();
    mTargetIndex = FTileIndex::Invalid;

    UTileMapModel* TileMap = GetTileMap();
    TileMap->ClearTileHighlight(ETileHighlightFlag::Aim | ETileHighlightFlag::Effect | ETileHighlightFlag::Select);
    TileMap->ClearMovePath();

    SetBuildPhase(ESRPGMoveBuildPhase::None);
}

void USRPGMoveBuildAction::ChangeDices(int32 RequestedDiceIndex)
{
    checkf(mMoveBuildPhase == ESRPGMoveBuildPhase::DestSelection, TEXT("이동 빌드 순서 오류"));

    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));

    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

    if (DicePoolModel->IsSelectedDice(RequestedDiceIndex))
    {
        DicePoolModel->MarkDiceUnselected(RequestedDiceIndex);
    }
    else
    {
        // 이동은 요구 개수 상한이 없다. 굴려졌고 미사용인 주사위라면 여러 개를 계속 추가할 수 있다.
        DicePoolModel->MarkDiceSelected(RequestedDiceIndex);
    }
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
        // 스킬의 CalculateSkillResult(가하는 데미지)의 역방향.
        // 받는 효과의 실제 적용은 실행 시 도착·경유 타일의 OnBeginTileOverlap이 담당.
        // 심 복제본에서 이 경로 이동을 실행해 출발/도착 스냅샷(MakeSnapshotData) 비교 또는
        // EventLog(mAttributeEffectEventLogs)로 스탯 델타를 산출해 프리뷰로 표시.
        // TODO : 이동 예측 계산 연결
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

bool USRPGMoveBuildAction::BuildMove()
{
    checkf(mMoveBuildPhase == ESRPGMoveBuildPhase::Preview, TEXT("이동 빌드 순서 오류"));

    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));
    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

    if (DicePoolModel->GetSelectedDiceNum() < 1 || mPathTileIndexes.Num() < 2)
    {
        return false;
    }

    USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
    checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 서브시스템 모델 nullptr"));

    /* 확정된 경로를 실어 이동 액션 생성 명령 발행 */

    TInstancedStruct<FSRPGCommand> MoveCastCommand;
    MoveCastCommand.InitializeAs<FSRPGMoveCommand>();
    MoveCastCommand.GetMutable<FSRPGMoveCommand>().mPathTileIndexes = mPathTileIndexes;
    MoveCastCommand.GetMutable<FSRPGMoveCommand>().mConsumeMovementAttribute = false;

    // 실행 액션 등록이 성공했을 때만 선택 주사위를 이번 턴 사용됨으로 잠근다.
    if (CommandRouterModel->SummitCommand(MoveCastCommand) == false)
    {
        return false;
    }

    DicePoolModel->MarkSelectedDiceAsUsed();
    return true;
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

    TileMap->ClearTileHighlight(ETileHighlightFlag::Aim);

    // 이동 거리 = 선택 주사위 합에 이동 스킬 효과와 현재 이동 보정을 적용한 값.
    const int32 MoveRange = mMovePoint;
    mReachableTileIndexes = TileMap->GetReachableTiles(mInstigator->GetTileTransform().mIndex, MoveRange);

    // 도달 범위를 조준 강조로 표시 (이동 범위 = 조준 범위로 표현)
    TileMap->SetTileHighlight(mReachableTileIndexes, ETileHighlightFlag::Aim);
}

int32 USRPGMoveBuildAction::CalculateMovePoint() const
{
    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 모델 nullptr"));

    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트 nullptr"));
    if (DicePoolModel->GetSelectedDiceNum() < 1)
    {
        return 0;
    }

    USkillComponentModel* SkillComponentModel = PlayerUnit->GetSkillComponentModel();
    checkf(SkillComponentModel != nullptr, TEXT("스킬 컴포넌트 nullptr"));
    const FSkillEntry* MovementSkillEntry = SkillComponentModel->GetSkill(mMovementSkillIndex);
    const UStaticSkillData* MovementSkill = MovementSkillEntry != nullptr ? MovementSkillEntry->mData.Get() : nullptr;
    if (MovementSkill == nullptr)
    {
        UE_LOG(LogSRPGCombat, Warning, TEXT("이동 스킬 데이터를 찾지 못해 주사위 합을 이동 거리로 사용합니다."));
        return DicePoolModel->GetSelectedDiceSum();
    }

    IBoardCombatTarget* CombatTarget = Cast<IBoardCombatTarget>(PlayerUnit);
    checkf(CombatTarget != nullptr, TEXT("이동 유닛이 전투 타겟 인터페이스를 구현하지 않음"));
    const float DiceSum = StaticCast<float>(DicePoolModel->GetSelectedDiceSum());
    for (const FSkillMotionLayer& MotionLayer : MovementSkill->mSkillMotionLayers)
    {
        for (const TInstancedStruct<FSkillEffectLayer>& EffectLayer : MotionLayer.mSkillEffectLayers)
        {
            if (const FSkillEffectLayer_GetMove* MoveEffect = EffectLayer.GetPtr<FSkillEffectLayer_GetMove>())
            {
                return MoveEffect->CalculateMoveGain(CombatTarget, DiceSum);
            }
        }
    }

    UE_LOG(LogSRPGCombat, Warning, TEXT("이동 스킬에 GetMove 효과가 없어 주사위 합을 이동 거리로 사용합니다."));
    return DicePoolModel->GetSelectedDiceSum();
}
