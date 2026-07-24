#include "SRPGFramework/SRPGSkillBuildAction.h"
#include "SRPGFramework/SRPGSkillAction.h"

#include "RDCollision.h"

#include "Component/SkillComponent/SkillComponentModel.h"
#include "DataAsset/SkillData/StaticSkillData.h"

#include "Actor/ActorView.h"
#include "Pawn/UnitModel.h"
#include "Actor/TileMap/TileMapModel.h"

#include "Singleton/WorldSubsystem/SimulationSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

FSRPGSkillSelectCommand::FSRPGSkillSelectCommand()
{
    mCommandType = ESRPGCommandType::SkillSelect;
    mRequestedAction = USRPGSkillBuildAction::StaticClass();
}

USRPGSkillBuildAction::USRPGSkillBuildAction()
{
    mActionType = ESRPGActionType::BuildAction;
    mConsumesTurn = false;
}

void USRPGSkillBuildAction::OnBeginAction()
{
    Super::OnBeginAction();
}

void USRPGSkillBuildAction::OnEndAction()
{
    ClearAllTileHighlights();
    ResetTargetTile();
    ResetSkill();

    Super::OnEndAction();
}

ESRPGCommandResult USRPGSkillBuildAction::HandleCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
    ESRPGCommandResult Result = Super::HandleCommand(Command);
    if (Result == ESRPGCommandResult::Handled)
    {
        return Result;
    }

    switch (Command.Get().GetCommandType())
    {
    case ESRPGCommandType::SkillSelect:
    {
        /* 새롭게 스킬 선택 시 제거 */

        const FSRPGSkillSelectCommand& SkillSelectCommand = Command.Get<FSRPGSkillSelectCommand>();

        OnSelectSkill = SkillSelectCommand.OnSelectSkill;
        OnChangeSkillBuildPhase = SkillSelectCommand.OnChangeSkillBuildPhase;
        OnPostSimulateSkillAction = SkillSelectCommand.OnPostSimulateSkillAction;
        OnCancelSimulateSkillAction = SkillSelectCommand.OnCancelSimulateSkillAction;
        if (SkillSelectCommand.mSkillIndex != mSelectedSkillIndex)
        {
            /* 다르면 변경 */

            ClearAllTileHighlights();
            ResetTargetTile();
            ResetSkill();
            SetBuildPhase(ESRPGSkillBuildPhase::None);

            SetSkill(SkillSelectCommand.mSkillIndex);
            RefreshAimableTileHighlights();
            SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);
        }
        else
        {
            /* 같으면 취소 */

            MarkActionCompleted(ESRPGActionResult::Cancelled);
            SetBuildPhase(ESRPGSkillBuildPhase::None);
        }
        return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
    }
    case ESRPGCommandType::MoveSelect:
    case ESRPGCommandType::TurnEnd:
    {
        /* 다른 동작 요구 시 취소 */

        ClearAllTileHighlights();
        ResetTargetTile();
        ResetSkill();
        MarkActionCompleted(ESRPGActionResult::Cancelled);
        SetBuildPhase(ESRPGSkillBuildPhase::None);
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

ESRPGCommandResult USRPGSkillBuildAction::HandleWorldTraceCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
    ESRPGCommandResult Result = ESRPGCommandResult::Ignored;

    const FSRPGWorldTraceCommand& WorldTraceCommand = Command.Get<FSRPGWorldTraceCommand>();
    if (WorldTraceCommand.mIsLongPress == true)
    {
        return Result;
    }

    UTileMapModel* TileMap = GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

    AActor* TargetActor = nullptr;
    FTileIndex TargetTileIndex = FTileIndex::Invalid;
    GetTileActorUnderCursor(GetWorld(), RDTraceChannels::TileOnlyTrace, WorldTraceCommand.mScreenPosition, OUT TargetActor, OUT TargetTileIndex);

    IActorView* ActorView = Cast<IActorView>(TargetActor);
    const bool IsContactedBoard = TargetTileIndex != FTileIndex::Invalid;
    if (IsContactedBoard == true)
    {
        /* 한단계 처리작업 (보드 안 탭) */

        switch (mSkillBuildPhase)
        {
        case ESRPGSkillBuildPhase::Preview:
        {
            /* 확정 칸 클릭 시, 스킬 캐스팅 */
            if (CanConfirmTargetTile(TargetTileIndex) == true)
            {
                if (CanBuildSkill() == true)
                {
                    BuildSkill();
                    SetBuildPhase(ESRPGSkillBuildPhase::Build);
                    MarkActionCompleted(ESRPGActionResult::Succeeded);

                    Result = ESRPGCommandResult::Handled;
                }
                break;
            }
            [[fallthrough]];
        }
        case ESRPGSkillBuildPhase::AimSelection:
        {
            /* 조준 가능한 칸 클릭 시, 프리뷰 단계까지 보여주기 */

            if (CanSelectTargetTile(TargetTileIndex) == true)
            {
                ResetTargetTile();
                SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);
                SetTargetTile(TargetTileIndex);
                RefreshEffectTileHighlights();
                SetBuildPhase(ESRPGSkillBuildPhase::Preview);

                Result = ESRPGCommandResult::Handled;
                break;
            }
            break;
        }
        }
    }
    else
    {
        /* 한단계 취소작업 (보드 밖 탭) */

        switch (mSkillBuildPhase)
        {
        case ESRPGSkillBuildPhase::Preview:
        {
            /* 프리뷰 단계에서 한단계 취소 시, 조준 대상 취소 처리 */

            ResetTargetTile();
            RefreshAimableTileHighlights();
            SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);

            Result = ESRPGCommandResult::Handled;
            break;
        }
        case ESRPGSkillBuildPhase::AimSelection:
        {
            /* 조준 대상 설정 단계에서 한단계 취소 시, 빌드 자체 종료 */

            MarkActionCompleted(ESRPGActionResult::Cancelled);
            SetBuildPhase(ESRPGSkillBuildPhase::None);

            Result = ESRPGCommandResult::Handled;
            break;
        }
        }
    }
    return Result;
}

void USRPGSkillBuildAction::SetSkill(int32 SkillIndex)
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::None, TEXT("스킬 빌드 순서 오류"));

    USkillComponentModel* SkillCompModel = mInstigator->GetSkillComponentModel();
    checkf(SkillCompModel != nullptr, TEXT("스킬 컴포넌트 모델 nullptr"));

    UTileMapModel* TileMap = GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"))

    /* 스킬 등록 */

    {
        TSoftObjectPtr<UStaticSkillData> StaticSkillDataSoftObj = nullptr;
        StaticSkillDataSoftObj = SkillCompModel->GetSkill(SkillIndex)->mData;
        if (StaticSkillDataSoftObj == nullptr)
        {
            UE_LOG(LogSRPGCombat, Warning, TEXT("스킬 시전 시 비정상적 스킬 선택"));
            return;
        }

        mSelectedSkillIndex = SkillIndex;
    }

    OnSelectSkill.Broadcast(mSelectedSkillIndex);
}

void USRPGSkillBuildAction::SetTargetTile(const FTileIndex& TargetIndex)
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::AimSelection, TEXT("스킬 빌드 순서 오류"));

    mTargetIndex = TargetIndex;

    UTileMapModel* TileMap = GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

    TArray<TObjectPtr<UBoardActorModel>> AllEffectActors;
    for (const FTileIndex& EffectTileIndex : mEffectTileIndexes)
    {
        TArray<UBoardActorModel*> EffectActors = TileMap->GetActorsOnTile(EffectTileIndex);
        AllEffectActors.Append(EffectActors);
    }

    USimulationSubsystem* SimulationSubsystem = GetWorld()->GetSubsystem<USimulationSubsystem>();
    checkf(SimulationSubsystem != nullptr, TEXT("시뮬레이션 서브시스템 모델 nullptr"));

    TInstancedStruct<FSRPGCommand> SkillCastCommand;
    SkillCastCommand.InitializeAs<FSRPGSkillCastCommand>();
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mSkillIndex = mSelectedSkillIndex;
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mTargetIndex = mTargetIndex;

    TArray<FSRPGTurnEventLog> TurnEventLogs = SimulationSubsystem->SimulateUntilNextAction(MoveTemp(SkillCastCommand));
    OnPostSimulateSkillAction.Broadcast(TurnEventLogs);
}

void USRPGSkillBuildAction::BuildSkill()
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::Preview, TEXT("스킬 빌드 순서 오류"));

    USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
    checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 서브시스템 모델 nullptr"));

    TInstancedStruct<FSRPGCommand> SkillCastCommand;
    SkillCastCommand.InitializeAs<FSRPGSkillCastCommand>();
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mSkillIndex = mSelectedSkillIndex;
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mTargetIndex = mTargetIndex;

    CommandRouterModel->SummitCommand(SkillCastCommand);
}

void USRPGSkillBuildAction::ResetSkill()
{
    mReachableTileIndexes.Empty();
    mSelectedSkillIndex = INDEX_NONE;

    OnSelectSkill.Broadcast(mSelectedSkillIndex);
}

void USRPGSkillBuildAction::ResetTargetTile()
{
    mEffectTileIndexes.Empty();
    mTargetIndex = FTileIndex::Invalid;
}

void USRPGSkillBuildAction::ClearAllTileHighlights()
{
    UTileMapModel* TileMap = GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

    TileMap->ClearTileHighlight(ETileHighlightFlag::All);
}

void USRPGSkillBuildAction::RefreshAimableTileHighlights()
{
    UTileMapModel* TileMap = GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

    USkillComponentModel* SkillCompModel = mInstigator->GetSkillComponentModel();
    checkf(SkillCompModel != nullptr, TEXT("스킬 컴포넌트 모델 nullptr"));

    mReachableTileIndexes = SkillCompModel->GetAimableTiles(TileMap, mSelectedSkillIndex);
    TileMap->SetTileHighlight(mReachableTileIndexes, ETileHighlightFlag::Aim);
}

void USRPGSkillBuildAction::RefreshEffectTileHighlights()
{
    UTileMapModel* TileMap = GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

    USkillComponentModel* SkillCompModel = mInstigator->GetSkillComponentModel();
    checkf(SkillCompModel != nullptr, TEXT("스킬 컴포넌트 모델 nullptr"));

    mEffectTileIndexes = SkillCompModel->GetEffectTiles(TileMap, mSelectedSkillIndex, mTargetIndex);
    TileMap->SetTileHighlight(mEffectTileIndexes, ETileHighlightFlag::Effect);
}

bool USRPGSkillBuildAction::CanSelectTargetTile(const FTileIndex& Index) const
{
    return mReachableTileIndexes.Contains(Index) == true;
}

bool USRPGSkillBuildAction::CanConfirmTargetTile(const FTileIndex& Index) const
{
    return mTargetIndex != FTileIndex::Invalid && mTargetIndex == Index;
}

bool USRPGSkillBuildAction::CanBuildSkill() const
{
    USkillComponentModel* SkillCompModel = mInstigator->GetSkillComponentModel();
    checkf(SkillCompModel != nullptr, TEXT("스킬 컴포넌트 모델 nullptr"));

    return SkillCompModel->CanActiveSkill(mSelectedSkillIndex);
}

void USRPGSkillBuildAction::SetBuildPhase(ESRPGSkillBuildPhase BuildPhase)
{
	if (mSkillBuildPhase == BuildPhase)
	{
		return;
	}

    if (BuildPhase != ESRPGSkillBuildPhase::Build && mSkillBuildPhase == ESRPGSkillBuildPhase::Preview)
    {
        OnCancelSimulateSkillAction.Broadcast();
    }

    mSkillBuildPhase = BuildPhase;
    OnChangeSkillBuildPhase.Broadcast(this, BuildPhase);
}

UTileMapModel* USRPGSkillBuildAction::GetTileMap() const
{
    USRPGTurnContext* TurnContext = mParent.Get();
    if (TurnContext != nullptr)
    {
        USRPGCombatModel* CombatModel = TurnContext->GetParent();
        if (CombatModel != nullptr)
        {
            UTileMapModel* TileMap = CombatModel->GetTileMap();
            return TileMap;
        }
    }
    return nullptr;
}

