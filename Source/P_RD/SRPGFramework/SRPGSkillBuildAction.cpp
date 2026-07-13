#include "SRPGFramework/SRPGSkillBuildAction.h"
#include "SRPGFramework/SRPGSkillAction.h"

#include "RDCollision.h"

#include "Component/SkillComponent/SkillComponentModel.h"
#include "Dice/DicePoolModel.h"
#include "DataAsset/SkillData/StaticSkillData.h"

#include "Actor/ActorView.h"
#include "Pawn/Player/PlayerUnitModel.h"
#include "Actor/TileMap/TileMapModel.h"

#include "Singleton/WorldSubsystem/SimulationSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

FSRPGSkillSelectCommand::FSRPGSkillSelectCommand()
{
    mCommandType = ESRPGCommandType::SkillSelect;
    mRequestedAction = USRPGSkillBuildAction::StaticClass();
}

FSRPGDiceSelectCommand::FSRPGDiceSelectCommand()
{
    mCommandType = ESRPGCommandType::DiceSelect;
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
    ResetDice();
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

            // 이전 스킬의 조준/효과 하이라이트를 먼저 지운다. ResetTargetTile은 데이터(mEffectTileIndexes)만
            // 비우고 화면 하이라이트는 안 지워서, 안 지우면 이전 스킬의 효과 범위가 남아 "취소가 안 된 것"처럼 보인다.
            ClearAllTileHighlights();
            ResetTargetTile();
            ResetDice();
            ResetSkill();
            /*
             * 조준 중 다른 스킬을 고르면 처음부터 다시 시작한다.
             * 페이즈도 None으로 돌려야 다음 SetSkill이 안전하다.
             */
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
        ResetDice();
        ResetSkill();
        MarkActionCompleted(ESRPGActionResult::Cancelled);
        SetBuildPhase(ESRPGSkillBuildPhase::None);
        return ESRPGCommandResult::Ignored;
    }
    case ESRPGCommandType::DiceSelect:
    {
        /* 주사위 변경 시 타겟부터 재설정 */

        const FSRPGDiceSelectCommand& DiceSelectCommand = Command.Get<FSRPGDiceSelectCommand>();

        /*
         * 주사위 변경은 스킬을 고른 뒤(조준/프리뷰 단계)에만 의미가 있다.
         * - 스킬 미선택(None)/시전 완료(Build) 상태의 주사위 클릭은 무시한다(억지로 진행하면
         *   선택 스킬이 없어 ChangeDices에서 nullptr 참조로 죽는다).
         * - 프리뷰에서 주사위를 바꾸면 주사위 합이 달라져 조준이 무효가 되므로, ChangeDices 전에
         *   조준 단계로 되돌린다. ChangeDices는 AimSelection 전제(checkf)라 안 되돌리면 어설션 크래시.
         *   (스킬 조준 -> 주사위 재선택 크래시 수정)
         */
        const bool IsSkillSelectedForDice = mSkillBuildPhase == ESRPGSkillBuildPhase::AimSelection
            || mSkillBuildPhase == ESRPGSkillBuildPhase::Preview;
        if (DiceSelectCommand.mDiceIndex != INDEX_NONE && IsSkillSelectedForDice == true)
        {
            ResetTargetTile();
            SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);
            ChangeDices(DiceSelectCommand.mDiceIndex);
            RefreshAimableTileHighlights();
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
    const bool IsContactedTileMap = ActorView != nullptr && ActorView->GetModel() == TileMap;
    const bool IsContactedBoard = IsContactedTileMap == true || TargetTileIndex != FTileIndex::Invalid;
    // 보드를 아예 벗어난 탭(IsContactedBoard=false)도 무효 타일과 똑같이 "타일 밖" 취소로 취급한다
    // (기획: 스킬 조준 중 타일 밖을 누르면 취소). TargetTileIndex가 유효하면 정상 처리로 간다.
    if (IsContactedBoard == true || TargetTileIndex == FTileIndex::Invalid)
    {
        if (TargetTileIndex == FTileIndex::Invalid)
        {
            /* 한단계 취소작업 (무효 타일 또는 보드 밖 탭) */

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
        else
        {
            /* 한단계 처리작업 */

            switch (mSkillBuildPhase)
            {
            case ESRPGSkillBuildPhase::Preview:
            {
                /* 확정 칸 클릭 시, 스킬 캐스팅 */

                if (mTargetIndex == TargetTileIndex)
                {
                    BuildSkill();
                    SetBuildPhase(ESRPGSkillBuildPhase::Build);
                    MarkActionCompleted(ESRPGActionResult::Succeeded);

                    Result = ESRPGCommandResult::Handled;
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
        mSelectedSkill = StaticSkillDataSoftObj.Get();
    }

    OnSelectSkill.Broadcast(mSelectedSkillIndex);
}

void USRPGSkillBuildAction::ChangeDices(int32 RequestedDiceIndex)
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::AimSelection, TEXT("스킬 빌드 순서 오류"));

    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));

    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

    if (DicePoolModel->IsSelectedDice(RequestedDiceIndex) == true)
    {
        // 이전 주사위 제거
        DicePoolModel->MarkDiceUnselected(RequestedDiceIndex);
    }
    else if (DicePoolModel->GetSelectedDiceNum() < mSelectedSkill->mRequiredDiceCount)
    {
        // 새로운 주사위 추가 할당
        DicePoolModel->MarkDiceSelected(RequestedDiceIndex);
    }
}

void USRPGSkillBuildAction::SetTargetTile(const FTileIndex& TargetIndex)
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::AimSelection, TEXT("스킬 빌드 순서 오류"));

    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));

    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

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
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mDiceSum = DicePoolModel->GetSelectedDiceSum();

    TArray<FSRPGTurnEventLog> TurnEventLogs = SimulationSubsystem->SimulateUntilNextAction(MoveTemp(SkillCastCommand));
    OnPostSimulateSkillAction.Broadcast(TurnEventLogs);
}

void USRPGSkillBuildAction::BuildSkill()
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::Preview, TEXT("스킬 빌드 순서 오류"));

    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));

    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

    // 확정
    DicePoolModel->MarkSelectedDiceAsUsed();

    USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
    checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 서브시스템 모델 nullptr"));

    TInstancedStruct<FSRPGCommand> SkillCastCommand;
    SkillCastCommand.InitializeAs<FSRPGSkillCastCommand>();
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mSkillIndex = mSelectedSkillIndex;
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mTargetIndex = mTargetIndex;
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mDiceSum = DicePoolModel->GetSelectedDiceSum();

    CommandRouterModel->SummitCommand(SkillCastCommand);
}

void USRPGSkillBuildAction::ResetSkill()
{
    mReachableTileIndexes.Empty();
    mSelectedSkill = nullptr;
    mSelectedSkillIndex = INDEX_NONE;

    OnSelectSkill.Broadcast(mSelectedSkillIndex);
}

void USRPGSkillBuildAction::ResetDice()
{
    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));

    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

    DicePoolModel->ResetSelected();
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

    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));

    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

    USkillComponentModel* SkillCompModel = mInstigator->GetSkillComponentModel();
    checkf(SkillCompModel != nullptr, TEXT("스킬 컴포넌트 모델 nullptr"));

    mReachableTileIndexes = SkillCompModel->GetAimableTiles(TileMap, mSelectedSkillIndex, DicePoolModel->GetSelectedDiceSum());
    TileMap->SetTileHighlight(mReachableTileIndexes, ETileHighlightFlag::Aim);
}

void USRPGSkillBuildAction::RefreshEffectTileHighlights()
{
    UTileMapModel* TileMap = GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));

    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

    USkillComponentModel* SkillCompModel = mInstigator->GetSkillComponentModel();
    checkf(SkillCompModel != nullptr, TEXT("스킬 컴포넌트 모델 nullptr"));

    mEffectTileIndexes = SkillCompModel->GetEffectTiles(TileMap, mSelectedSkillIndex, mTargetIndex, DicePoolModel->GetSelectedDiceSum());
    TileMap->SetTileHighlight(mEffectTileIndexes, ETileHighlightFlag::Effect);
}

bool USRPGSkillBuildAction::CanSelectTargetTile(const FTileIndex& Index) const
{
    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));

    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

    return mReachableTileIndexes.Contains(Index) == true && DicePoolModel->GetSelectedDiceNum() == mSelectedSkill->mRequiredDiceCount;
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

