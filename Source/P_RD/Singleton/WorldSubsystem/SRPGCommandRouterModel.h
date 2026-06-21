/*****************************************************************//**
 * @file   SRPGCommandRouterModel.h
 * @brief  SRPG 유저 명령을 전달하는 서브시스템 모델 정의 헤더
 * @author 모호재
 * @date   2026-06-19
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectModel.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGCommandRouterModel.generated.h"

// SRPG Command 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogSRPGCommandRouter, Log, All)

struct FSRPGCommand;
class ISRPGCommandHandler;

/**
 * @brief  SRPG 유저 명령을 전달하는 서브시스템 모델
 */
UCLASS()
class P_RD_API USRPGCommandRouterModel : public UObjectModel
{
	GENERATED_BODY()

public:
	/**
	 * 특정 명령을 핸들러에게 제출하는 함수
	 * @param Command 처리할 명령 객체
	 * @return 명령 처리 여부
	 */
	bool SummitCommand(const TInstancedStruct<FSRPGCommand>& Command);

public:
	void RegisterCommandHandler(TScriptInterface<ISRPGCommandHandler> Handler);
	void UnregisterCommandHandler(TScriptInterface<ISRPGCommandHandler> Handler);

protected:
	ESRPGCommandResult HandleFallbackCommand(const TInstancedStruct<FSRPGCommand>& Command);

protected:
	// @brief 등록된 명령 처리자들
	UPROPERTY(Category = Handler, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CommandHandlers"))
	TArray<TScriptInterface<ISRPGCommandHandler>> mCommandHandlers;
};
