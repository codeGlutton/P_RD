/*****************************************************************//**
 * @file   SRPGAction.h
 * @brief  전투 상황마다 활용되는 Context 객체 구현 헤더 
 * @author 모호재
 * @date   2026-04-28
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"

class AUnit;

/**
 * @brief  사용자 입력에 대한 SRPG 결과 객체
 */
struct FSRPGAction : public TSharedFromThis<FSRPGAction>
{
	friend struct FSRPGTurnContext;

public:
	virtual ~FSRPGAction() = default;

protected:
	void InitAction(TSharedRef<FSRPGTurnContext> Owner, AUnit* Instigator, TArray<AUnit*>& Targets);
	virtual void BeginAction();
	virtual void TickAction(float DeltaTime);
	virtual void EndAction();

protected:
	virtual bool IsBlockTurnProgression() const;

protected:
	void EvaluateActionEndState(bool ForceAbort = false);

public:
	UWorld* GetWorld() const;
	TWeakPtr<FSRPGTurnContext> GetOwner() const;
	AUnit* GetInstigator() const;

protected:
	TWeakPtr<FSRPGTurnContext> mOwner;
	TObjectPtr<AUnit> mInstigator;
	TArray<TObjectPtr<AUnit>> mTargets;

	// @brief 현재 액션 상태
	ESRPGActionPhase mPhase = ESRPGActionPhase::None;
	// @brief 액션 종료 결과
	ESRPGActionResult mResult = ESRPGActionResult::Succeeded;
};






