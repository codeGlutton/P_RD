/*****************************************************************//**
 * @file   SRPGCommandHandler.h
 * @brief  SRPG 명령을 처리할 수 있는 객체 인터페이스 정의 헤더
 * @author 모호재
 * @date   2026-06-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGCommandHandler.generated.h"

struct FSRPGCommand;

UINTERFACE(MinimalAPI)
class USRPGCommandHandler : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief  SRPG 명령을 처리할 수 있는 객체 인터페이스
 */
class P_RD_API ISRPGCommandHandler
{
	GENERATED_BODY()

	friend class USRPGCommandRouterModel;

protected:
	/**
	 * 명령 처리 우선순위를 반환하는 함수
	 * @return 명령 처리 우선순위 값
	 */
	virtual int8 GetCommandPriority() const = 0;
	/**
	 * 명령을 처리하는 함수
	 * @param Command 명령 객체
	 * @return 명령 처리 결과
	 */
	virtual ESRPGCommandResult HandleCommand(const TInstancedStruct<FSRPGCommand>& Command) = 0;

public:
	static constexpr int8 HIGHEST_PRIORITY = INT8_MAX;
	static constexpr int8 LOWEST_PRIORITY = INT8_MIN;
};
