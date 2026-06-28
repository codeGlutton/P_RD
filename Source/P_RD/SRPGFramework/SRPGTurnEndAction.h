/*****************************************************************//**
 * @file   SRPGTurnEndAction.h
 * @brief  종료 명령 대한 SRPG 행동 객체 구현 헤더
 * @author 모호재
 * @date   2026-06-28
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGAction.h"
#include "SRPGFramework/SRPGCommand.h"
#include "SRPGTurnEndAction.generated.h"

/**
 * @brief  종료 명령 객체
 */
USTRUCT(BlueprintType)
struct FSRPGTurnEndCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGTurnEndCommand();
};

/**
 * @brief  종료 명령 대한 SRPG 행동 객체
 */
UCLASS()
class USRPGTurnEndAction : public USRPGAction
{
	GENERATED_BODY()

protected:
	USRPGTurnEndAction();

protected:
	void OnBeginAction() override;
};

