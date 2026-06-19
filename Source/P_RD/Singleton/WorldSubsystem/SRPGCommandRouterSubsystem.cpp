#include "Singleton/WorldSubsystem/SRPGCommandRouterSubsystem.h"
#include "SRPGFramework/SRPGCommandHandler.h"
#include "SRPGFramework/SRPGCommand.h"

DEFINE_LOG_CATEGORY(LogSRPGCommandRouter)

bool USRPGCommandRouterSubsystem::SummitCommand(TSharedPtr<const FSRPGCommand> Command)
{
	checkf(Command != nullptr, TEXT("유효하지 않은 커맨드"));
	checkf(Command->GetCommandType() != ESRPGCommandType::None, TEXT("유효하지 않은 커맨드"));

	ESRPGCommandResult Result = ESRPGCommandResult::Ignored;

	/* 핸들러 우선 순위대로 커맨드 처리 요청 */

	for (const TSharedPtr<ISRPGCommandHandler>& Handler : mCommandHandlers)
	{
		Result = CombineSRPGCommandResult(Handler->HandleCommand(Command), Result);
		if (Result == ESRPGCommandResult::Handled)
		{
			UE_LOG(LogSRPGCommandRouter, Log, TEXT("[%s] 특정 핸들러가 처리"), *EnumToString(Command->GetCommandType()));
			break;
		}
	}

	/* 커맨드 처리 실패 시 Fallback 처리 */

	if (Result != ESRPGCommandResult::Handled)
	{
		UE_LOG(LogSRPGCommandRouter, Log, TEXT("[%s] 커맨드 Fallback"), *EnumToString(Command->GetCommandType()));
		Result = CombineSRPGCommandResult(HandleFallbackCommand(Command), Result);
	}

	const bool IsCommandUsed = Result != ESRPGCommandResult::Ignored;
	return IsCommandUsed;
}

void USRPGCommandRouterSubsystem::RegisterCommandHandler(TSharedPtr<ISRPGCommandHandler> Handler)
{
	mCommandHandlers.Add(Handler);
	mCommandHandlers.Sort([](const TSharedPtr<ISRPGCommandHandler>& Lhs, const TSharedPtr<ISRPGCommandHandler>& Rhs) {
		return Lhs->GetCommandPriority() > Rhs->GetCommandPriority();
		});
}

void USRPGCommandRouterSubsystem::UnregisterCommandHandler(TSharedPtr<ISRPGCommandHandler> Handler)
{
	mCommandHandlers.Remove(Handler);
}

ESRPGCommandResult USRPGCommandRouterSubsystem::HandleFallbackCommand(TSharedPtr<const FSRPGCommand> Command)
{
	// TODO : 무언가... 하겠지?

	return ESRPGCommandResult::Ignored;
}