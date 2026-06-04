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
struct FSRPGTurnContext;
struct FSRPGAction;

/**
 * @brief  SRPG 행동을 제작하기 위해 요구되는 절차 처리 일회성 객체
 * @details 
 * 예를 들어 스킬 실행 Action은 스킬 선택, 시전 영역 선택, 최종 프리뷰 확인 후 
 * 선택 과정이 지나야 로직이 최종적으로 결정된다. 이 일련의 제작 과정을 도와주는 객체
 * 해당 객체의 생성 시에는 추가적인 Action 실행이 일시적으로 중단된다.
 */
struct FSRPGActionBuilder
{
	friend struct FSRPGTurnContext;
	using ActionType = FSRPGAction;

protected:
	FSRPGActionBuilder() = default;
	virtual ~FSRPGActionBuilder() = default;

protected:
	void InitBuilder(TSharedRef<FSRPGTurnContext> Owner, AUnit* Instigator);

	virtual TSharedPtr<FSRPGAction> BuildAction() = 0;
	virtual void UnbuildAction() = 0;

protected:
	TWeakPtr<FSRPGTurnContext> mOwner;
	TObjectPtr<AUnit> mInstigator;

protected:
	bool mIsExpired = false;
};

/**
 * @brief  사용자 입력에 따른 정해진 SRPG 행동 객체
 */
struct FSRPGAction : public TSharedFromThis<FSRPGAction>
{
	friend struct FSRPGTurnContext;
	friend struct FSRPGActionBuilder;

protected:
	FSRPGAction() = default;
	virtual ~FSRPGAction() = default;

protected:
	void InitAction(TSharedRef<FSRPGTurnContext> Owner, AUnit* Instigator);
	virtual void BeginAction();
	virtual void TickAction(float DeltaTime);
	virtual void EndAction();

protected:
	/**
	 * 액션 종료와 함께 턴 종료를 강제하는 액션인지 여부
	 * @return 턴 종료를 강제하는 액션 여부
	 */
	virtual bool IsTurnEndingAction() const;

protected:
	void EvaluateActionEndState(bool ForceAbort = false);

public:
	UWorld* GetWorld() const;
	TWeakPtr<FSRPGTurnContext> GetOwner() const;
	AUnit* GetInstigator() const;

protected:
	TWeakPtr<FSRPGTurnContext> mOwner;
	TObjectPtr<AUnit> mInstigator;

	// @brief 현재 액션 상태
	ESRPGActionPhase mPhase = ESRPGActionPhase::None;
	// @brief 액션 종료 결과
	ESRPGActionResult mResult = ESRPGActionResult::Succeeded;
};






