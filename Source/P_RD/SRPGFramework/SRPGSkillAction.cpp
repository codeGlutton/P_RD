#include "SRPGFramework/SRPGSkillAction.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"

#include "Pawn/Unit.h"

#include "Pawn/SkillComponent.h"

FSRPGSkillCastCommand::FSRPGSkillCastCommand()
{
    mCommandType = ESRPGCommandType::SkillCast;
}

FSRPGSkillAction::FSRPGSkillAction()
{
    mActionType = ESRPGActionType::InPlayAction;
    mConsumesTurn = false;
}

void FSRPGSkillAction::OnBeginAction()
{
    Super::OnBeginAction();
    
    // TODO : 스킬 처리 로직
}

void FSRPGSkillAction::OnTickAction(float DeltaTime)
{
    Super::OnTickAction(DeltaTime);
}

void FSRPGSkillAction::OnEndAction()
{
    Super::OnEndAction();
}

