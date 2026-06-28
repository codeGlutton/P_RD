#include "SRPGFramework/SRPGSkillAction.h"

#include "Component/SkillComponent/SkillComponentModel.h"

#include "Pawn/UnitModel.h"

// #include "Pawn/SkillComponent.h"

FSRPGSkillCastCommand::FSRPGSkillCastCommand()
{
    mCommandType = ESRPGCommandType::SkillCast;
    mRequestedAction = USRPGSkillAction::StaticClass();
}

USRPGSkillAction::USRPGSkillAction()
{
    mActionType = ESRPGActionType::InPlayAction;
    mConsumesTurn = false;
}

void USRPGSkillAction::OnBeginAction()
{
    Super::OnBeginAction();
    
    // TODO : 스킬 처리 로직
}

void USRPGSkillAction::OnTickAction(float DeltaTime)
{
    Super::OnTickAction(DeltaTime);
}

void USRPGSkillAction::OnEndAction()
{
    Super::OnEndAction();
}

ESRPGCommandResult USRPGSkillAction::HandleCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
    ESRPGCommandResult Result = Super::HandleCommand(Command);
    if (Result == ESRPGCommandResult::Handled)
    {
        return Result;
    }

    switch (Command.Get().GetCommandType())
    {
    case ESRPGCommandType::SkillCast:
    {
        /* 스킬 실행 */

        const FSRPGSkillCastCommand& SkillCastCommand = Command.Get<FSRPGSkillCastCommand>();

        USkillComponentModel* SkillCompModel = mInstigator->GetSkillComponentModel();
        checkf(SkillCompModel != nullptr, TEXT("스킬 컴포넌트 모델 nullptr"));

        SkillCompModel->ActivateSkill(SkillCastCommand.mSkillIndex, SkillCastCommand.mEffectTileIndexes, SkillCastCommand.mDicePoint);

        return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
    }
    }

    return ESRPGCommandResult::Ignored;
}

