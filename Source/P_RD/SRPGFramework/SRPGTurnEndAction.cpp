#include "SRPGFramework/SRPGTurnEndAction.h"

FSRPGTurnEndCommand::FSRPGTurnEndCommand()
{
    mCommandType = ESRPGCommandType::TurnEnd;
    mRequestedAction = USRPGTurnEndAction::StaticClass();
}

USRPGTurnEndAction::USRPGTurnEndAction()
{
    mActionType = ESRPGActionType::InPlayAction;
    mConsumesTurn = true;
}

void USRPGTurnEndAction::OnBeginAction()
{
    Super::OnBeginAction();

    MarkActionCompleted(ESRPGActionResult::Succeeded);
}
