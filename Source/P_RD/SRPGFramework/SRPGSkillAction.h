/*****************************************************************//**
 * @file   SRPGSkillAction.h
 * @brief  스킬에 대한 SRPG 행동 객체 구현 헤더
 * @author 모호재
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGAction.h"
#include "SRPGFramework/SRPGCommand.h"
#include "FunctionLibrary/CombatCalculator/CombatResult.h"
#include "SRPGSkillAction.generated.h"

struct FSRPGSkillAction;

USTRUCT()
struct FSRPGSkillCastCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGSkillCastCommand();

public:
	FSkillCommitResult mCalculationResult;
};

/**
 * @brief  사용자 입력에 따른 정해진 SRPG 행동 객체
 */
UCLASS()
class USRPGSkillAction : public USRPGAction
{
	GENERATED_BODY()

protected:
	USRPGSkillAction();

protected:
	void OnBeginAction() override;
	void OnTickAction(float DeltaTime) override;
	void OnEndAction() override;

protected:
	// TArray<FSRPGCalculatedData> mCalculateDatas;
};

