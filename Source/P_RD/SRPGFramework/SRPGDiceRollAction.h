/*****************************************************************//**
 * @file   SRPGDiceRollAction.h
 * @brief  주사위 굴리기 SRPG 행동 객체 구현 헤더
 * @author 모호재
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGAction.h"
#include "SRPGFramework/SRPGCommand.h"
#include "SRPGDiceRollAction.generated.h"

USTRUCT()
struct FSRPGDicePrepareCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGDicePrepareCommand();
};

USTRUCT()
struct FSRPGDiceRollCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGDiceRollCommand();
};

/**
 * @brief  주사위 굴리기 SRPG 행동 객체
 */
UCLASS()
class USRPGDiceRollAction : public USRPGAction
{
	GENERATED_BODY()

protected:
	USRPGDiceRollAction();

protected:
	ESRPGCommandResult HandleCommand(const TInstancedStruct<FSRPGCommand>& Command) override;
};

