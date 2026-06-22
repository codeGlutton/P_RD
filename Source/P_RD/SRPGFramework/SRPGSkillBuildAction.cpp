#include "SRPGFramework/SRPGSkillBuildAction.h"
#include "SRPGFramework/SRPGSkillAction.h"

#include "RDCollision.h"

// #include "Pawn/SkillComponent.h"
#include "Dice/DicePoolModel.h"
#include "DataAsset/SkillData/StaticSkillData.h"

#include "Actor/ActorView.h"
#include "Pawn/Player/PlayerUnitModel.h"
#include "Actor/TileMap/TileMapModel.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

FSRPGSkillSelectCommand::FSRPGSkillSelectCommand()
{
    mCommandType = ESRPGCommandType::SkillSelect;
    mRequestedAction = USRPGSkillBuildAction::StaticClass();
}

FSRPGCDiceSelectCommand::FSRPGCDiceSelectCommand()
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

        if (SkillSelectCommand.mSkillIndex != mSelectedSkillIndex)
        {
            ResetTargetTile();
            ResetDice();
            ResetSkill();
            SetSkill(SkillSelectCommand.mSkillIndex);
            RefreshAimableTileHighlights();
            SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);
        }
        return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
    }
    case ESRPGCommandType::DiceSelect:
    {
        /* 주사위 변경 시 타겟부터 재설정 */

        const FSRPGCDiceSelectCommand& DiceSelectCommand = Command.Get<FSRPGCDiceSelectCommand>();

        if (DiceSelectCommand.mDiceIndex != INDEX_NONE)
        {
            ResetTargetTile();
            ChangeDices(DiceSelectCommand.mDiceIndex);
            RefreshAimableTileHighlights();
            SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);
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
    GetTileActorUnderCursor(RDTraceChannels::TileOnlyTrace, OUT TargetActor, OUT TargetTileIndex);
    
    IActorView* ActorView = Cast<IActorView>(TargetActor);
    const bool IsContactedTileMap = ActorView != nullptr && ActorView->GetModel() == TileMap;
    if (IsContactedTileMap == true)
    {
        if (TargetTileIndex == FTileIndex::Invalid)
        {
            /* 한단계 취소작업 */

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

                SetBuildPhase(ESRPGSkillBuildPhase::None);
                MarkActionCompleted(ESRPGActionResult::Cancelled);

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

                const bool CanAim = mReachableTileIndexes.Contains(TargetTileIndex) == true && mSelectedDices.Num() == mSelectedSkill->mDiceCount;
                if (CanAim == true)
                {
                    ResetTargetTile();
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

    // USkillComponent* SkillComp = mInstigator->GetSkillComponent();
    // checkf(SkillComp != nullptr, TEXT("스킬 컴포넌트 nullptr"));

    UTileMapModel* TileMap = GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"))

    /* 스킬 등록 */

    {
        TSoftObjectPtr<UStaticSkillData> StaticSkillDataSoftObj = nullptr;
        // SkillComp->GetSkillData(SkillIndex, OUT StaticSkillDataSoftObj);
        if (StaticSkillDataSoftObj == nullptr)
        {
            UE_LOG(LogSRPGCombat, Warning, TEXT("스킬 시전 시 비정상적 스킬 선택"));
            return;
        }

        mSelectedSkillIndex = SkillIndex;
        mSelectedSkill = StaticSkillDataSoftObj.Get();
    }
}

void USRPGSkillBuildAction::ChangeDices(int32 RequestedDiceIndex)
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::AimSelection, TEXT("스킬 빌드 순서 오류"));

    if (mSelectedDices.Contains(RequestedDiceIndex) == true)
    {
        // 이전 주사위 제거
        mSelectedDices.Remove(RequestedDiceIndex);
    }
    else if (mSelectedDices.Num() < mSelectedSkill->mDiceCount)
    {
        // 새로운 주사위 추가 할당
        mSelectedDices.Add(RequestedDiceIndex);
    }

    mSelectedDiceSum = 0;
    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    if (PlayerUnit != nullptr)
    {
        if (UDicePoolModel* DicePool = PlayerUnit->GetDicePool())
        {
            for (int32 DiceIndex : mSelectedDices)
            {
                mSelectedDiceSum += DicePool->GetRolledDiceValue(DiceIndex);
            }
        }
    }
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

    // USkillComponent* SkillComp = mInstigator->GetSkillComponent();
    // checkf(SkillComp != nullptr, TEXT("스킬 컴포넌트 nullptr"));

    // 김준형
    // 파라미터 변경으로 비활성화 처리했습니다.
    //SkillComp->CalculateSkillResult(mSelectedSkillIndex, AllEffectActors, OUT mCalculationResult);
}

void USRPGSkillBuildAction::BuildSkill()
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::Preview, TEXT("스킬 빌드 순서 오류"));

    USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
    checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 서브시스템 모델 nullptr"));

    TInstancedStruct<FSRPGCommand> SkillCastCommand;
    SkillCastCommand.InitializeAs<FSRPGSkillCastCommand>();
    // SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mCalculationResult = mCalculationResult;

    CommandRouterModel->SummitCommand(SkillCastCommand);
}

void USRPGSkillBuildAction::ResetSkill()
{
    mReachableTileIndexes.Empty();
    mSelectedSkill = nullptr;
    mSelectedSkillIndex = INDEX_NONE;
}

void USRPGSkillBuildAction::ResetDice()
{
    mSelectedDices.Empty();
    mSelectedDiceSum = 0;
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

    const float AimRange = mSelectedSkill->mAimDefaultRange + mSelectedDiceSum * mSelectedSkill->mAimRatioRange;
    const EAimPattern Pattern = mSelectedSkill->mAimPattern;
    const bool CanAimObstacle = mSelectedSkill->mCanAimObstacle;
    const bool IsIndirect = mSelectedSkill->mIsIndirect;
    mReachableTileIndexes = TileMap->GetAimableTiles(mInstigator->GetTileTransform().mIndex, AimRange, Pattern, CanAimObstacle, IsIndirect);
    TileMap->SetTileHighlight(mReachableTileIndexes, ETileHighlightFlag::Aim);
}

void USRPGSkillBuildAction::RefreshEffectTileHighlights()
{
    UTileMapModel* TileMap = GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

    const EEffectPattern Pattern = mSelectedSkill->mEffectPattern;
    const int32 EffectRange = mSelectedSkill->mEffectDefaultArea + mSelectedDiceSum * mSelectedSkill->mEffectRatioArea;
    const bool IsInDirect = mSelectedSkill->mIsIndirect;
    mEffectTileIndexes = TileMap->GetEffectTiles(mInstigator->GetTileTransform().mIndex, mTargetIndex, Pattern, EffectRange, IsInDirect);
    TileMap->SetTileHighlight(mEffectTileIndexes, ETileHighlightFlag::Effect);
}

void USRPGSkillBuildAction::SetBuildPhase(ESRPGSkillBuildPhase BuildPhase)
{
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


