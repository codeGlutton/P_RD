#include "SRPGFramework/SRPGSkillBuildAction.h"

#include "Actor/ActorView.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Pawn/UnitModel.h"
#include "Actor/TileMap/TileMapModel.h"

#include "DataAsset/SkillData/StaticSkillData.h"
// #include "Pawn/SkillComponent.h"
#include "SRPGFramework/SRPGSkillAction.h"

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
            ResetSkill();
            SetSkill(SkillSelectCommand.mSkillIndex);
        }
        return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
    }
    case ESRPGCommandType::DiceSelect:
    {
        /* 주사위 변경 시 타겟부터 재설정 */

        const FSRPGCDiceSelectCommand& DiceSelectCommand = Command.Get<FSRPGCDiceSelectCommand>();

        ResetTargetTile();
        ChangeDices(DiceSelectCommand.mDiceIndex);
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
    if (WorldTraceCommand.mIsLongPress == false)
    {
        USRPGTurnContext* TurnContext = mParent.Get();
        checkf(TurnContext != nullptr, TEXT("턴 객체 nullptr"));
        USRPGCombatModel* CombatModel = TurnContext->GetParent();
        checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

        UTileMapModel* TileMap = CombatModel->GetTileMap();
        checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"))

        AActor* TargetActor = nullptr;
        FTileIndex TargetTileIndex = FTileIndex::Invalid;
        GetTileActorUnderCursor(ECC_GameTraceChannel1 /* TODO : 임시 */, OUT TargetActor, OUT TargetTileIndex);

        IActorView* ActorView = Cast<IActorView>(TargetActor);
        if (ActorView->GetModel() == TileMap)
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
                    Result = ESRPGCommandResult::Handled;
                    break;
                }
                case ESRPGSkillBuildPhase::AimSelection:
                {
                    /* 조준 대상 설정 단계에서 한단계 취소 시, 빌드 자체 종료 */

                    EndAction(ESRPGActionResult::Cancelled);
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
                        EndAction(ESRPGActionResult::Succeeded);
                        Result = ESRPGCommandResult::Handled;
                        break;
                    }
                    [[fallthrough]];
                }
                case ESRPGSkillBuildPhase::AimSelection:
                {
                    /* 조준 가능한 칸 클릭 시, 프리뷰 단계까지 보여주기 */

                    if (mAimTileIndexes.Contains(TargetTileIndex) == true)
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

void USRPGSkillBuildAction::ChangeDices(int32 RequestedDiceIndex)
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::AimSelection, TEXT("스킬 빌드 순서 오류"));

    if (mSelectedDices.Contains(RequestedDiceIndex) == true)
    {
        // 이전 주사위 제거
        // mSelectedDiceSum -= GetDiceValue(RequestedDiceIndex);
        mSelectedDices.Remove(RequestedDiceIndex);
    }
    else if (mSelectedDices.Num() < mSelectedSkill->mDiceCount)
    {
        // 새로운 주사위 추가 할당
        // mSelectedDiceSum += GetDiceValue(RequestedDiceIndex);
        mSelectedDices.Add(RequestedDiceIndex);
    }

    ResetSkill();
    SetSkill(mSelectedSkillIndex);
}


void USRPGSkillBuildAction::SetSkill(int32 SkillIndex)
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::None, TEXT("스킬 빌드 순서 오류"));

    // USkillComponent* SkillComp = mInstigator->GetSkillComponent();
    // checkf(SkillComp != nullptr, TEXT("스킬 컴포넌트 nullptr"));

    USRPGTurnContext* TurnContext = mParent.Get();
    checkf(TurnContext != nullptr, TEXT("턴 객체 nullptr"));
    USRPGCombatModel* CombatModel = TurnContext->GetParent();
    checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

    UTileMapModel* TileMap = CombatModel->GetTileMap();
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

    /* 맵 변경 */

    {
        const float AimRange = mSelectedSkill->mAimDefaultRange + mSelectedDiceSum * mSelectedSkill->mAimRatioRange;
        const EAimPattern Pattern = mSelectedSkill->mAimPattern;
        const bool CanAimObstacle = mSelectedSkill->mCanAimObstacle;
        const bool IsIndirect = mSelectedSkill->mIsIndirect;
        mAimTileIndexes = TileMap->GetAimableTiles(mInstigator->GetTileTransform().mIndex, AimRange, Pattern, CanAimObstacle, IsIndirect);
        // TileMap->SetTileHighlight(mAimTileIndexes, ETileHighlightFlag::Aim);
    }

    /* 상태 변경되면서 외부에서 바인딩된 UI 변경 */

    SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);
}

void USRPGSkillBuildAction::ResetSkill()
{
    mAimTileIndexes.Empty();
    mSelectedSkill = nullptr;
    mSelectedSkillIndex = INDEX_NONE;

    mSelectedDices.Empty();
    mSelectedDiceSum = 0;

    mEffectTileIndexes.Empty();
    mTargetIndex = FTileIndex::Invalid;
    mCalculationResult.mEffectCommitResult.Empty();

    USRPGTurnContext* TurnContext = mParent.Get();
    checkf(TurnContext != nullptr, TEXT("턴 객체 nullptr"));
    USRPGCombatModel* CombatModel = TurnContext->GetParent();
    checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

    UTileMapModel* TileMap = CombatModel->GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"))

    // TileMap->ClearTileHighlight(ETileHighlightFlag::Aim | ETileHighlightFlag::Effect | ETileHighlightFlag::Select);
    SetBuildPhase(ESRPGSkillBuildPhase::None);
}

void USRPGSkillBuildAction::SetTargetTile(const FTileIndex& TileIndex)
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::AimSelection, TEXT("스킬 빌드 순서 오류"));

    // USkillComponent* SkillComp = mInstigator->GetSkillComponent();
    // checkf(SkillComp != nullptr, TEXT("스킬 컴포넌트 nullptr"));

    USRPGTurnContext* TurnContext = mParent.Get();
    checkf(TurnContext != nullptr, TEXT("턴 객체 nullptr"));
    USRPGCombatModel* CombatModel = TurnContext->GetParent();
    checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

    UTileMapModel* TileMap = CombatModel->GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"))

    /* 맵 변경 */

    {
        const EEffectPattern Pattern = mSelectedSkill->mEffectPattern;
        const int32 EffectRange = mSelectedSkill->mEffectDefaultArea + mSelectedDiceSum * mSelectedSkill->mEffectRatioArea;
        const bool IsInDirect = mSelectedSkill->mIsIndirect;
        mEffectTileIndexes = TileMap->GetEffectTiles(mInstigator->GetTileTransform().mIndex, TileIndex, Pattern, EffectRange, IsInDirect);
        // TileMap->SetTileHighlight(mEffectTileIndexes, ETileHighlightFlag::Effect);
    }

    /* 예측 시스템 */

    {
        TArray<TObjectPtr<UBoardActorModel>> AllEffectActors;
        for (const FTileIndex& EffectTileIndex : mEffectTileIndexes)
        {
            TArray<UBoardActorModel*> EffectActors = TileMap->GetActorsOnTile(EffectTileIndex);
            AllEffectActors.Append(EffectActors);
        }
        
        // 김준형
        // 파라미터 변경으로 비활성화 처리했습니다.
        //SkillComp->CalculateSkillResult(mSelectedSkillIndex, AllEffectActors, OUT mCalculationResult);
    }

    /* 상태 변경되면서 외부에서 바인딩된 UI 변경 */

    SetBuildPhase(ESRPGSkillBuildPhase::Preview);
}

void USRPGSkillBuildAction::ResetTargetTile()
{
    mEffectTileIndexes.Empty();
    mTargetIndex = FTileIndex::Invalid;
    mCalculationResult.mEffectCommitResult.Empty();

    USRPGTurnContext* TurnContext = mParent.Get();
    checkf(TurnContext != nullptr, TEXT("턴 객체 nullptr"));
    USRPGCombatModel* CombatModel = TurnContext->GetParent();
    checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

    UTileMapModel* TileMap = CombatModel->GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"))

    // TileMap->ClearTileHighlight(ETileHighlightFlag::Effect | ETileHighlightFlag::Select);
    SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);
}

void USRPGSkillBuildAction::BuildSkill()
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::Preview, TEXT("스킬 빌드 순서 오류"));

    USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
    checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 서브시스템 모델 nullptr"));

    TInstancedStruct<FSRPGCommand> SkillCastCommand;
    SkillCastCommand.InitializeAs<FSRPGSkillCastCommand>();
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mCalculationResult = mCalculationResult;

    CommandRouterModel->SummitCommand(SkillCastCommand);
}

void USRPGSkillBuildAction::SetBuildPhase(ESRPGSkillBuildPhase BuildPhase)
{
    mSkillBuildPhase = BuildPhase;
    OnChangeSkillBuildPhase.Broadcast(this, BuildPhase);
}


