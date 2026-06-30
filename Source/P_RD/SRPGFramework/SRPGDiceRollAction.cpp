#include "SRPGFramework/SRPGDiceRollAction.h"
#include "FunctionLibrary/RandomStreamFunctionLibrary.h"

#include "Pawn/Player/PlayerUnitModel.h"

#include "Dice/DicePoolModel.h"

FSRPGDicePrepareCommand::FSRPGDicePrepareCommand()
{
    mCommandType = ESRPGCommandType::DicePrepare;
    mRequestedAction = USRPGDiceRollAction::StaticClass();
}

FSRPGDiceRollCommand::FSRPGDiceRollCommand()
{
    mCommandType = ESRPGCommandType::DiceRoll;
}

USRPGDiceRollAction::USRPGDiceRollAction()
{
    mActionType = ESRPGActionType::InPlayAction;
    mConsumesTurn = false;
}

ESRPGCommandResult USRPGDiceRollAction::HandleCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
    ESRPGCommandResult Result = Super::HandleCommand(Command);
    if (Result == ESRPGCommandResult::Handled)
    {
        return Result;
    }

    switch (Command.Get().GetCommandType())
    {
    case ESRPGCommandType::DicePrepare:
    {
        /* 주사위 팝업 보여주기 */

        UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
        checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));
        UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
        checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트 모델 nullptr"));
        
        DicePoolModel->ResetUsed();

        const FSRPGDicePrepareCommand& DicePrepareCommand = Command.Get<FSRPGDicePrepareCommand>();
        DicePrepareCommand.OnShowDicePanelUI.Broadcast();

        return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
    }
    case ESRPGCommandType::DiceRoll:
    {
        /* 주사위 굴리기 */

        UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
        checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));
        UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
        checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트 모델 nullptr"));

        DicePoolModel->RollAll(URandomStreamFunctionLibrary::GetEventStream(this));
        MarkActionCompleted(ESRPGActionResult::Succeeded);

        return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
    }
    }

    return ESRPGCommandResult::Ignored;
}

