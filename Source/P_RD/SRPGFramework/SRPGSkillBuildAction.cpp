#include "SRPGFramework/SRPGSkillBuildAction.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"

#include "Pawn/Unit.h"
#include "Actor/TileMap/TileMap.h"

#include "DataAsset/SkillData/StaticSkillData.h"
#include "Pawn/SkillComponent.h"
#include "SRPGFramework/SRPGSkillAction.h"

FSRPGSkillSelectCommand::FSRPGSkillSelectCommand()
{
    mActionCommandType = ESRPGActionCommandType::SkillSelect;
}

FSRPGCDiceSelectCommand::FSRPGCDiceSelectCommand()
{
    mActionCommandType = ESRPGActionCommandType::DiceSelect;
}

FSRPGSkillBuildAction::FSRPGSkillBuildAction()
{
    mActionType = ESRPGActionType::BuildAction;
    mConsumesTurn = false;
}

void FSRPGSkillBuildAction::OnBeginAction()
{
    Super::OnBeginAction();
}

void FSRPGSkillBuildAction::OnEndAction()
{
    ResetSkill();
    Super::OnEndAction();
}

ESRPGActionCommandResult FSRPGSkillBuildAction::CanHandleCommand(TSharedPtr<const FSRPGActionCommand> Command) const
{
    if (Super::CanHandleCommand(Command) == ESRPGActionCommandResult::Handle)
    {
        return ESRPGActionCommandResult::Handle;
    }

    switch (Command->GetActionCommandType())
    {
    case ESRPGActionCommandType::SkillSelect:
    case ESRPGActionCommandType::DiceSelect:
    {
        return ESRPGActionCommandResult::Handle;
    }
    case ESRPGActionCommandType::WorldTrace:
    {
        TSharedPtr<const FSRPGWorldTraceCommand> WorldTraceCommand = StaticCastSharedPtr<const FSRPGWorldTraceCommand>(Command);
        return WorldTraceCommand->mIsLongPress == true ? ESRPGActionCommandResult::Unhandle : ESRPGActionCommandResult::Handle;
    }
    }

    return ESRPGActionCommandResult::Unhandle;
}

void FSRPGSkillBuildAction::ApplyCommand(TSharedPtr<const FSRPGActionCommand> Command)
{
    Super::ApplyCommand(Command);

    switch (Command->GetActionCommandType())
    {
    case ESRPGActionCommandType::SkillSelect:
    {
        TSharedPtr<const FSRPGSkillSelectCommand> SkillSelectCommand = StaticCastSharedPtr<const FSRPGSkillSelectCommand>(Command);
        OnChangeSkillBuildPhase = SkillSelectCommand->OnChangeSkillBuildPhase;

        ResetSkill();
        SetSkill(SkillSelectCommand->mSkillIndex);
        break;
    }
    case ESRPGActionCommandType::DiceSelect:
    {
        TSharedPtr<const FSRPGCDiceSelectCommand> DiceSelectCommand = StaticCastSharedPtr<const FSRPGCDiceSelectCommand>(Command);
        
        ResetTargetTile();
        ChangeDices(DiceSelectCommand->mDiceIndex);
        break;
    }
    case ESRPGActionCommandType::WorldTrace:
    {
        ApplyWorldTraceCommand(Command);
        break;
    }
    }
}

void FSRPGSkillBuildAction::ApplyWorldTraceCommand(TSharedPtr<const FSRPGActionCommand> Command)
{
    USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
    checkf(CombatSubsystem != nullptr, TEXT("전투 서브시스템 nullptr"));
    ATileMap* TileMap = CombatSubsystem->GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"))

    AActor* TargetActor = nullptr;
    FTileIndex TargetTileIndex = FTileIndex::Invalid;
    GetTileActorUnderCursor(ECC_GameTraceChannel1 /* TODO : 임시 */, OUT TargetActor, OUT TargetTileIndex);

    if (TargetActor == TileMap)
    {
        if (TargetTileIndex == FTileIndex::Invalid)
        {
            /* 한단계 취소작업 */

            switch (mSkillBuildPhase)
            {
            case ESRPGSkillBuildPhase::Preview:
            {
                // 조준 대상 취소 처리
                ResetTargetTile();
                return;
            }
            case ESRPGSkillBuildPhase::AimSelection:
            {
                // 빌드 취소 처리
                EndAction();
                return;
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
                // 타겟 지정 확정 칸 클릭 시
                if (mTargetIndex == TargetTileIndex)
                {
                    BuildSkill();
                    EndAction();
                    return;
                }
            }
            case ESRPGSkillBuildPhase::AimSelection:
            {
                // 조준 가능한 칸 클릭 시
                if (mAimTileIndexes.Contains(TargetTileIndex) == true)
                {
                    ResetTargetTile();
                    SetTargetTile(TargetTileIndex);
                    return;
                }
            }
            }
        }
    }
}

void FSRPGSkillBuildAction::ChangeDices(int32 RequestedDiceIndex)
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


void FSRPGSkillBuildAction::SetSkill(int32 SkillIndex)
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::None, TEXT("스킬 빌드 순서 오류"));

    USkillComponent* SkillComp = mInstigator->GetSkillComponent();
    checkf(SkillComp != nullptr, TEXT("스킬 컴포넌트 nullptr"));

    USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
    checkf(CombatSubsystem != nullptr, TEXT("전투 서브시스템 nullptr"));
    ATileMap* TileMap = CombatSubsystem->GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));


    /* 스킬 등록 */

    {
        TSoftObjectPtr<UStaticSkillData> StaticSkillDataSoftObj = nullptr;
        SkillComp->GetSkillData(SkillIndex, OUT StaticSkillDataSoftObj);
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
        TileMap->SetTileHighlight(mAimTileIndexes, ETileHighlightFlag::Aim);
    }

    /* 상태 변경되면서 외부에서 바인딩된 UI 변경 */

    SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);
}

void FSRPGSkillBuildAction::ResetSkill()
{
    mAimTileIndexes.Empty();
    mSelectedSkill = nullptr;
    mSelectedSkillIndex = INDEX_NONE;

    mSelectedDices.Empty();
    mSelectedDiceSum = 0;

    mEffectTileIndexes.Empty();
    mTargetIndex = FTileIndex::Invalid;
    mCalculationResult.mEffectCommitResult.Empty();

    USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
    checkf(CombatSubsystem != nullptr, TEXT("전투 서브시스템 nullptr"));
    ATileMap* TileMap = CombatSubsystem->GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

    TileMap->ClearTileHighlight(ETileHighlightFlag::Aim | ETileHighlightFlag::Effect | ETileHighlightFlag::Select);
    SetBuildPhase(ESRPGSkillBuildPhase::None);
}

void FSRPGSkillBuildAction::SetTargetTile(const FTileIndex& TileIndex)
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::AimSelection, TEXT("스킬 빌드 순서 오류"));

    USkillComponent* SkillComp = mInstigator->GetSkillComponent();
    checkf(SkillComp != nullptr, TEXT("스킬 컴포넌트 nullptr"));

    USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
    checkf(CombatSubsystem != nullptr, TEXT("전투 서브시스템 nullptr"));
    ATileMap* TileMap = CombatSubsystem->GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

    /* 맵 변경 */

    {
        const EEffectPattern Pattern = mSelectedSkill->mEffectPattern;
        const int32 EffectRange = mSelectedSkill->mEffectDefaultArea + mSelectedDiceSum * mSelectedSkill->mEffectRatioArea;
        const bool IsInDirect = mSelectedSkill->mIsIndirect;
        mEffectTileIndexes = TileMap->GetEffectTiles(mInstigator->GetTileTransform().mIndex, TileIndex, Pattern, EffectRange, IsInDirect);
        TileMap->SetTileHighlight(mEffectTileIndexes, ETileHighlightFlag::Effect);
    }

    /* 예측 시스템 */

    {
        TArray<TScriptInterface<ITileActor>> AllEffectActors;
        for (const FTileIndex& EffectTileIndex : mEffectTileIndexes)
        {
            TArray<TScriptInterface<ITileActor>> EffectActors = TileMap->GetActorsOnTile(EffectTileIndex);
            AllEffectActors.Append(EffectActors);
        }

        SkillComp->CalculateSkillResult(mSelectedSkillIndex, AllEffectActors, OUT mCalculationResult);
    }

    /* 상태 변경되면서 외부에서 바인딩된 UI 변경 */

    SetBuildPhase(ESRPGSkillBuildPhase::Preview);
}

void FSRPGSkillBuildAction::ResetTargetTile()
{
    mEffectTileIndexes.Empty();
    mTargetIndex = FTileIndex::Invalid;
    mCalculationResult.mEffectCommitResult.Empty();

    USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
    checkf(CombatSubsystem != nullptr, TEXT("전투 서브시스템 nullptr"));
    ATileMap* TileMap = CombatSubsystem->GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

    TileMap->ClearTileHighlight(ETileHighlightFlag::Effect | ETileHighlightFlag::Select);
    SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);
}

void FSRPGSkillBuildAction::BuildSkill()
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::Preview, TEXT("스킬 빌드 순서 오류"));

    USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
    checkf(CombatSubsystem != nullptr, TEXT("전투 서브시스템 nullptr"));

    TSharedPtr<FSRPGSkillCastCommand> SkillCastCommand = MakeShared<FSRPGSkillCastCommand>();
    SkillCastCommand->mCalculationResult = mCalculationResult;

    CombatSubsystem->SummitCommand(SkillCastCommand);
}

void FSRPGSkillBuildAction::SetBuildPhase(ESRPGSkillBuildPhase BuildPhase)
{
    mSkillBuildPhase = BuildPhase;
    OnChangeSkillBuildPhase.Broadcast(*this, BuildPhase);
}


