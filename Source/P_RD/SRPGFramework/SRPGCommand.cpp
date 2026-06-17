#include "SRPGFramework/SRPGCommand.h"

ESRPGCommandType FSRPGCommand::GetCommandType() const
{
	return mCommandType;
}

TSharedPtr<FSRPGAction> FSRPGCommand::CreateAction() const
{
	return nullptr;
}

FSRPGWorldTraceCommand::FSRPGWorldTraceCommand()
{
	mCommandType = ESRPGCommandType::WorldTrace;
}