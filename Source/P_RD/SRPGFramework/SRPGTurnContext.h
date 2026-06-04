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

protected:
	FSRPGTurnContext() = default;

public:
	/**
	 * 즉시 액션 생성 함수
	 * @tparam ActionType 액션 종류
	 */
	template<typename ActionType>
	TSharedPtr<ActionType> MakeAction() const
	{
		TSharedPtr<ActionType> NewAction = TSharedPtr<ActionType>(new ActionType(), [](ActionType* Action) {
			delete Action;
			});
		NewAction->InitAction(AsShared(), mOwner);

		return NewAction;
	}

	/**
	 * 액션 빌더 생성 함수. 스킬과 이동과 같은 사용자 지정 Action은 실행 전 순차적인 작업이 요구되기에 빌더 절차 필수
	 * @tparam BuilderType 빌더 종류
	 */
	template<typename BuilderType>
	TSharedPtr<BuilderType> MakeActionBuilder()
	{
		// 예를 들어 공격 스킬 및 이동 스킬 등의 Action은 
		// 모든 상황이 종료되고 ActionSelect를 도달해야 프리뷰 UI를 정확히 표기할 수 있다.
		checkf(mPhase == ESRPGTurnPhase::ActionSelect, TEXT("액션 선택 대기 중에만 빌드 가능"));
		mPhase = ESRPGTurnPhase::ActionBuild;

		TSharedPtr<BuilderType> mActionBuilder = TSharedPtr<BuilderType>(new BuilderType(), [](BuilderType* Builder) {
			delete Builder;
			});
		mActionBuilder->InitBuilder(AsShared(), mOwner);

		return mActionBuilder;
	}

	template<typename ActionType>
	TSharedPtr<ActionType> BuildAction(TSharedPtr<FSRPGActionBuilder> Builder)
	{
		TSharedPtr<ActionType> NewAction = StaticCastSharedPtr<ActionType>(BuildAction(Builder));

		return NewAction;
	}
	TSharedPtr<FSRPGAction> BuildAction(TSharedPtr<FSRPGActionBuilder> Builder);
	void UnbuildAction(TSharedPtr<FSRPGActionBuilder> Builder);

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
