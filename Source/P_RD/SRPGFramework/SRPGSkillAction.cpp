#include "SRPGFramework/SRPGSkillAction.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"

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

