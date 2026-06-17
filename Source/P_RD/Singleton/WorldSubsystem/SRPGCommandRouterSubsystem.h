/*****************************************************************//**
 * @file   SRPGCommandRouterSubsystem.h
 * @brief  SRPG 유저 명령을 전달하는 서브시스템 정의 헤더
 * @author 모호재
 * @date   2026-06-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGCommandRouterSubsystem.generated.h"

// User Command 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogSRPGCommandRouter, Log, All)

struct FSRPGCommand;
struct ISRPGCommandHandler;

/**
 * @brief  SRPG 유저 명령을 전달하는 서브시스템
 */
UCLASS()
class P_RD_API USRPGCommandRouterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	/* 커맨드 함수 */
public:
	/**
	 * 유저 명령 처리
	 * @param Command 처리할 명령 객체
	 * @return 명령 처리 여부
	 */
	bool SummitCommand(TSharedPtr<const FSRPGCommand> Command);

public:
	void RegisterCommandHandler(TSharedPtr<ISRPGCommandHandler> Handler);
	void UnregisterCommandHandler(TSharedPtr<ISRPGCommandHandler> Handler);

protected:
	ESRPGCommandResult HandleFallbackCommand(TSharedPtr<const FSRPGCommand> Command);

protected:
	// @brief 명령 처리자들
	TArray<TSharedPtr<ISRPGCommandHandler>> mCommandHandlers;
};
