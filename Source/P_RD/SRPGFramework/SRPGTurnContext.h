/*****************************************************************//**
 * @file   SRPGTurnContext.h
 * @brief  턴 런타임 정보를 담은 Context 객체 구현 헤더 
 * @author 모호재
 * @date   2026-04-28
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGFramework/SRPGAction.h"

struct FSRPGTurnContext;
struct FPresentationBarrier;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBeginTurnUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const FSRPGTurnContext& /*TurnContext*/)
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnEndTurnUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const FSRPGTurnContext& /*TurnContext*/, ESRPGTurnResult /*Result*/)

class AUnit;
struct FSRPGAction;

/**
 * @brief  스킬 사용 시 임시 정보를 들고 있는 Context 객체
 */
struct FSRPGTurnContext : public TSharedFromThis<FSRPGTurnContext>
{
	friend class USRPGCombatSubsystem;

public:
	/**
	 * 액션 추가 함수
	 * @tparam ActionType 액션 종류
	 * @tparam Args 액션 생성자 인자 타입들
	 * @param ...args 액션 생성자 인자 값들
	 */
	template<typename ActionType, typename... Args>
	void PushAction(AUnit* Instigator, TArray<AUnit*>& Targets, Args&&... args)
	{
		if (mPhase == ESRPGTurnPhase::TurnEnd)
		{
			return;
		}

		TSharedPtr<FSRPGAction> NewAction = MakeShared<ActionType>(MoveTemp<Args>(args)...);
		NewAction->InitAction(AsShared(), Instigator, Targets);

		// 자신의 턴에 진행중인 액션이 존재하지 않을 경우, 새로운 액션 즉시 진행
		const bool NeedToPlayAction = mActions.IsEmpty() == true;
		mActions.Enqueue(NewAction);
		if (NeedToPlayAction == true)
		{
			StartNextAction();
		}
	}

protected:
	void InitTurn(AUnit* Owner, int32 LifeCount);
	void BeginTurn();
	void TickTurn(float DeltaTime);
	void EndTurn();

protected:
	void StartNextAction();

public:
	void EvaluateTurnStates(bool ForceAbort = false);
	void OnEndCurrentAction(TSharedRef<FSRPGAction> Action, ESRPGActionResult ActionResult);

protected:
	void EvaluateTurnEndState(bool ForceAbort);

public:
	UWorld* GetWorld() const;
	AUnit* GetOwner() const;

	bool IsPermanent() const;
	int32 GetLifeCount() const;

public:
	FOnBeginTurnUI OnBeginTurnUI;
	FOnEndTurnUI OnEndTurnUI;

protected:
	TObjectPtr<AUnit> mOwner;
	
	// @brief 현재 턴 상태
	ESRPGTurnPhase mPhase = ESRPGTurnPhase::None;
	// @brief 턴 종료 결과
	ESRPGTurnResult mResult = ESRPGTurnResult::Succeeded;

	TQueue<TSharedPtr<FSRPGAction>> mActions;

protected:
	static constexpr int32 PERMENENT_TURN = -1;
	int32 mLifeCount = 0;
};
