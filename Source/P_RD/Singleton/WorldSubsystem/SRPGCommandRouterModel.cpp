#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"
#include "SRPGFramework/SRPGCommandHandler.h"
#include "SRPGFramework/SRPGCommand.h"

DEFINE_LOG_CATEGORY(LogSRPGCommandRouter)

bool USRPGCommandRouterModel::SummitCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
	checkf(Command.IsValid() == true, TEXT("유효하지 않은 커맨드"));
	checkf(Command.Get().GetCommandType() != ESRPGCommandType::None, TEXT("유효하지 않은 커맨드"));

	ESRPGCommandResult Result = ESRPGCommandResult::Ignored;

	/* 핸들러 우선 순위대로 커맨드 처리 요청 */

	for (const TScriptInterface<ISRPGCommandHandler>& Handler : mCommandHandlers)
	{
		Result = CombineSRPGCommandResult(Handler->HandleCommand(Command), Result);
		if (Result == ESRPGCommandResult::Handled)
		{
			UE_LOG(LogSRPGCommandRouter, Log, TEXT("특정 핸들러가 처리"));
			break;
		}
	}

	/* 커맨드 처리 실패 시 Fallback 처리 */

	if (Result != ESRPGCommandResult::Handled)
	{
		UE_LOG(LogSRPGCommandRouter, Log, TEXT("[%s] 커맨드 Fallback"), *EnumToString(Command.Get().GetCommandType()));
		Result = CombineSRPGCommandResult(HandleFallbackCommand(Command), Result);
	}

	/* 대리자 처리 */

	OnHandleCommand.Broadcast(Result);
	
	const bool IsCommandUsed = Result != ESRPGCommandResult::Ignored;
	return IsCommandUsed;
}

void USRPGCommandRouterModel::RegisterCommandHandler(TScriptInterface<ISRPGCommandHandler> Handler)
{
	mCommandHandlers.Add(Handler);
	mCommandHandlers.Sort([](const TScriptInterface<ISRPGCommandHandler>& Lhs, const TScriptInterface<ISRPGCommandHandler>& Rhs) {
		return Lhs->GetCommandPriority() > Rhs->GetCommandPriority();
		});
}

void USRPGCommandRouterModel::UnregisterCommandHandler(TScriptInterface<ISRPGCommandHandler> Handler)
{
	mCommandHandlers.Remove(Handler);
}

ESRPGCommandResult USRPGCommandRouterModel::HandleFallbackCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
	// TODO : 무언가... 하겠지?

	return ESRPGCommandResult::Ignored;
}