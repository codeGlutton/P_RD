#include "SRPGFramework/SRPGCommand.h"

ESRPGCommandType FSRPGCommand::GetCommandType() const
{
	return mCommandType;
}

FSRPGWorldTraceCommand::FSRPGWorldTraceCommand()
{
	mCommandType = ESRPGCommandType::WorldTrace;
}

