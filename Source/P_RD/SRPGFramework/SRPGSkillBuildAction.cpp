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

FSRPGSkillBuildAction::FSRPGSkillBuildAction()
{
    mActionType = ESRPGActionType::BuildAction;
    mConsumesTurn = false;
}

void FSRPGSkillBuildAction::OnBeginAction()
{
    Super::OnBeginAction();
    SetSkill(mSelectedSkillIndex);

    // 플레이어 주사위 등록 대리자에 바인딩
}

void FSRPGSkillBuildAction::OnEndAction()
{
    mAimTileIndexes.Empty();
    mSelectedSkill = nullptr;
    mSelectedSkillIndex = INDEX_NONE;

    mEffectTileIndexes.Empty();
    mTargetIndex = FTileIndex::Invalid;
    // mCalculateDatas.Empty();

    USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
    checkf(CombatSubsystem != nullptr, TEXT("전투 서브시스템 nullptr"));
    ATileMap* TileMap = CombatSubsystem->GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

    /* 맵 클리어 */

    {
        TileMap->SetTileHighlight(mAimTileIndexes, ETileHighlightFlag::None);
    }

    /* 상태 변경되면서 외부에서 바인딩된 UI 변경 */

    SetBuildPhase(ESRPGSkillBuildPhase::None);

    // 플레이어 주사위 등록 대리자에 언바인딩

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
    case ESRPGActionCommandType::WorldTrace:
    case ESRPGActionCommandType::SkillSelect:
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
    case ESRPGActionCommandType::WorldTrace:
    {
        ApplyWorldTraceCommand(Command);
    }
    case ESRPGActionCommandType::SkillSelect:
    {
        TSharedPtr<const FSRPGSkillSelectCommand> SkillSelectCommand = StaticCastSharedPtr<const FSRPGSkillSelectCommand>(Command);
        OnChangeSkillBuildPhase = SkillSelectCommand->OnChangeSkillBuildPhase;
        SetSkill(SkillSelectCommand->mSkillIndex);
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
            case ESRPGSkillBuildPhase::AimSelection:
            {
                // 빌드 취소 처리
                EndAction();
                return;
            }
            case ESRPGSkillBuildPhase::Preview:
            {
                // 조준 대상 취소 처리
                SetSkill(mSelectedSkillIndex);
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
                    SetTargetTile(TargetTileIndex);
                    return;
                }
            }
            }
        }
    }
}

void FSRPGSkillBuildAction::SetSkill(int32 SkillIndex)
{
    mEffectTileIndexes.Empty();
    mTargetIndex = FTileIndex::Invalid;
    // mCalculateDatas.Empty();

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
        const int32 Range = mSelectedSkill->mAimDefaultRange;
        const EAimPattern Pattern = mSelectedSkill->mAimPattern;
        const bool CanAimObstacle = mSelectedSkill->mCanAimObstacle;
        const bool IsIndirect = mSelectedSkill->mIsIndirect;
        mAimTileIndexes = TileMap->GetAimableTiles(mInstigator->GetTileTransform().mIndex, Range, Pattern, CanAimObstacle, IsIndirect);
        TileMap->SetTileHighlight(mAimTileIndexes, ETileHighlightFlag::Aim);
    }

    /* 상태 변경되면서 외부에서 바인딩된 UI 변경 */

    SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);
}

void FSRPGSkillBuildAction::SetTargetTile(const FTileIndex& TileIndex)
{
    USkillComponent* SkillComp = mInstigator->GetSkillComponent();
    checkf(SkillComp != nullptr, TEXT("스킬 컴포넌트 nullptr"));

    USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
    checkf(CombatSubsystem != nullptr, TEXT("전투 서브시스템 nullptr"));
    ATileMap* TileMap = CombatSubsystem->GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

    /* 맵 변경 */

    {
        const int32 DiceSum = 1; // TODO : 플레이어의 주사위 스텟 확인

        const EEffectPattern Pattern = mSelectedSkill->mEffectPattern;
        const float EffectSize = mSelectedSkill->mEffectDefaultArea + DiceSum * mSelectedSkill->mEffectRatioArea;
        const bool IsInDirect = mSelectedSkill->mIsIndirect;
        mEffectTileIndexes = TileMap->GetEffectTiles(mInstigator->GetTileTransform().mIndex, TileIndex, Pattern, EffectSize, IsInDirect);
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

        // mCalculateDatas = SkillComp->CalculateDatas(mSelectedSkillIndex, AllEffectActors);
    }

    /* 상태 변경되면서 외부에서 바인딩된 UI 변경 */

    SetBuildPhase(ESRPGSkillBuildPhase::Preview);
}

void FSRPGSkillBuildAction::BuildSkill()
{
    USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
    checkf(CombatSubsystem != nullptr, TEXT("전투 서브시스템 nullptr"));

    auto SkillCastCommand = MakeShared<FSRPGSkillCastCommand>();
    // SkillCastCommand.mCalculateDatas = mCalculateDatas;

    CombatSubsystem->SummitCommand(SkillCastCommand);
}

void FSRPGSkillBuildAction::SetBuildPhase(ESRPGSkillBuildPhase BuildPhase)
{
    mSkillBuildPhase = BuildPhase;
    OnChangeSkillBuildPhase.Broadcast(*this, BuildPhase);
}


