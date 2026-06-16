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
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnBeginAnyActionUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const FSRPGTurnContext& /*TurnContext*/, const FSRPGAction& /*Action*/)
DECLARE_MULTICAST_DELEGATE_FourParams(FOnEndAnyActionUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const FSRPGTurnContext& /*TurnContext*/, const FSRPGAction& /*Action*/, ESRPGActionResult /*Result*/)

class AUnit;
class USRPGCombatSubsystem;   // Clang(Android)은 friend 선언을 전방선언으로 인정하지 않음 → 정식 전방선언 필요
struct FSRPGAction;

/**
 * @brief  스킬 사용 시 임시 정보를 들고 있는 Context 객체
 */
struct FSRPGTurnContext : public TSharedFromThis<FSRPGTurnContext>
{
	friend class USRPGCombatSubsystem;
	friend struct FSRPGActionLock;

protected:
	FSRPGTurnContext() = default;

	/* 생명 주기 함수 */
protected:
	void InitTurn(USRPGCombatSubsystem* Parent, AUnit* Owner, int32 LifeCount);
	void BeginTurn();
	void TickTurn(float DeltaTime);
	void EndTurn();

	/* 액션 커맨드 처리 함수 */
protected:
	ESRPGActionCommandResult RouteCommand(TSharedPtr<const FSRPGActionCommand> Command);
	ESRPGActionCommandResult HandleActionCreationCommand(TSharedPtr<const FSRPGActionCommand> Command);
	ESRPGActionCommandResult HandleFallbackCommand(TSharedPtr<const FSRPGActionCommand> Command);

private:
	void EnqueueAction(TSharedPtr<FSRPGAction> NewAction);
	void DequeueAction();

public:
	void EvaluateTurnStates(bool ForceAbort = false);
	void OnEndCurrentAction(TSharedRef<FSRPGAction> Action, ESRPGActionResult ActionResult);

protected:
	void EvaluateTurnEndState(bool ForceAbort);

	/* 외부 API */
public:
	UWorld* GetWorld() const;
	USRPGCombatSubsystem* GetParent() const;
	AUnit* GetOwner() const;

	bool IsPermanent() const;
	int32 GetLifeCount() const;

public:
	FOnBeginTurnUI OnBeginTurnUI;
	FOnEndTurnUI OnEndTurnUI;
	FOnBeginAnyActionUI OnBeginAnyActionUI;
	FOnEndAnyActionUI OnEndAnyActionUI;

protected:
	TObjectPtr<USRPGCombatSubsystem> mParent;
	TObjectPtr<AUnit> mOwner;
	
	// @brief 현재 턴 상태
	ESRPGTurnPhase mTurnPhase = ESRPGTurnPhase::None;
	// @brief 턴 종료 결과
	ESRPGTurnResult mTurnResult = ESRPGTurnResult::Succeeded;

	TQueue<TSharedPtr<FSRPGAction>> mActions;

protected:
	static constexpr int32 PERMENENT_TURN = -1;
	int32 mLifeCount = 0;
};
