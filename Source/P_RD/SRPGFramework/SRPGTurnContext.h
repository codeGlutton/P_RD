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
struct FSRPGTurnContext;

/**
 * @brief  턴 진행을 막는 RAII 방식의 Lock 객체
 */
struct FSRPGActionLock
{
public:
	FSRPGActionLock(TSharedPtr<FSRPGTurnContext> TurnContext);
	~FSRPGActionLock();

private:
	TWeakPtr<FSRPGTurnContext> mTurnContext;
};

/**
 * @brief  스킬 사용 시 임시 정보를 들고 있는 Context 객체
 */
struct FSRPGTurnContext : public TSharedFromThis<FSRPGTurnContext>
{
	friend class USRPGCombatSubsystem;
	friend struct FSRPGActionLock;

protected:
	FSRPGTurnContext() = default;

public:
	/**
	 * 액션 빌더 생성 함수
	 * @tparam BuilderType 빌더 종류
	 */
	template<typename BuilderType>
	TUniquePtr<BuilderType> StartActionBuild()
	{
		static_assert(TIsDerivedFrom<BuilderType, FSRPGActionBuilder>::Value, TEXT("BuilderType 는 반드시 FSRPGActionBuilder 파생 객체"));

		TUniquePtr<BuilderType> ActionBuilder = MakeUnique<BuilderType>();
		ActionBuilder->InitBuilder(AsShared(), mOwner);

		return ActionBuilder;
	}

	void CompleteActionBuild(TUniquePtr<FSRPGActionBuilder>&& Builder);
	void CancelActionBuild(TUniquePtr<FSRPGActionBuilder>&& Builder);

protected:
	/**
	 * 액션을 액션 큐에 삽입. 여기서 ActionSelect 상태의 경우, 액션 큐를 진행해도 무방하다고 판단하고 즉시 실행
	 * @param NewAction 새로운 액션
	 */
	void PushAction(TSharedPtr<FSRPGAction> NewAction);

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
