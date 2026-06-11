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
#include "SRPGFramework/SRPGActionLock.h"

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
	friend struct FSRPGActionLock;

protected:
	FSRPGTurnContext() = default;

public:
	/**
	 * 액션 빌드 함수
	 * @tparam DraftType 초안 종류
	 * @return 액션을 만들기 위한 초안 구성 객체
	 */
	template<typename DraftType>
	TUniquePtr<DraftType> StartActionBuild()
	{
		static_assert(TIsDerivedFrom<DraftType, FSRPGActionDraft>::Value, TEXT("BuilderType 는 반드시 FSRPGActionBuilder 파생 객체"));

		TUniquePtr<DraftType> ActionDraft = MakeUnique<DraftType>();
		ActionDraft->InitDraft(AsShared(), mOwner);

		return ActionDraft;
	}

	void CompleteActionBuild(TUniquePtr<FSRPGActionDraft>&& Draft);
	void CancelActionBuild(TUniquePtr<FSRPGActionDraft>&& Draft);

protected:
	void InitTurn(AUnit* Owner, int32 LifeCount);
	void BeginTurn();
	void TickTurn(float DeltaTime);
	void EndTurn();

protected:
	void EnqueueAction(TSharedPtr<FSRPGAction> NewAction);
	void DequeueAction();

public:
	void EvaluateTurnStates(bool ForceAbort = false);
	void OnEndCurrentAction(TSharedRef<FSRPGAction> Action, ESRPGActionResult ActionResult);

protected:
	void EvaluateTurnEndState(bool ForceAbort);

protected:
	void NotifyTurnStartIfNeeded();
	void NotifyTurnEndIfNeeded();

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
