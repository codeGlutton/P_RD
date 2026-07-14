#include "SRPGFramework/SRPGCommand.h"

ESRPGCommandType FSRPGCommand::GetCommandType() const
{
	return mCommandType;
}

FSRPGBuildConfirmCommand::FSRPGBuildConfirmCommand()
{
	mCommandType = ESRPGCommandType::BuildConfirm;
}

FSRPGBuildCancelCommand::FSRPGBuildCancelCommand()
{
	mCommandType = ESRPGCommandType::BuildCancel;
}

FSRPGWorldTraceCommand::FSRPGWorldTraceCommand()
{
	mCommandType = ESRPGCommandType::WorldTrace;
}

