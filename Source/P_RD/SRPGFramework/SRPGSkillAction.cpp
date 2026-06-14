#include "SRPGFramework/SRPGSkillAction.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"

#include "Pawn/Unit.h"

#include "Pawn/SkillComponent.h"

FSRPGSkillCastCommand::FSRPGSkillCastCommand()
{
    mActionCommandType = ESRPGActionCommandType::SkillCast;
}

FSRPGSkillAction::FSRPGSkillAction()
{
    mActionType = ESRPGActionType::InPlayAction;
    mConsumesTurn = false;
}

void FSRPGSkillAction::OnBeginAction()
{
    Super::OnBeginAction();
}

void FSRPGSkillAction::OnTickAction(float DeltaTime)
{
    Super::OnTickAction(DeltaTime);
}

void FSRPGSkillAction::OnEndAction()
{
    Super::OnEndAction();
}

