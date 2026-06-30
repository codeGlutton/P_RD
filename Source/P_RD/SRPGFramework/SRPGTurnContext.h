/*****************************************************************//**
 * @file	SRPGTurnContext.h
 * @brief	턴 런타임 정보를 담은 Context 객체 구현 헤더
 * @author	모호재
 * @date	2026-04-28
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGFramework/SRPGCommandHandler.h"
#include "SRPGTurnContext.generated.h"

struct FPresentationBarrier;

class USRPGCombatModel;
class USRPGTurnContext;
class USRPGAction;
struct FSRPGCommand;

class UUnitModel;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBeginTurnUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const USRPGTurnContext* /*TurnContext*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnEndTurnUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const USRPGTurnContext* /*TurnContext*/, ESRPGTurnResult /*Result*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnBeginAnyActionUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const USRPGTurnContext* /*TurnContext*/, const USRPGAction* /*Action*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnEndAnyActionUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const USRPGTurnContext* /*TurnContext*/, const USRPGAction* /*Action*/, ESRPGActionResult /*Result*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnShowDicePanelAtTurnStartUI, const USRPGTurnContext* /*TurnContext*/);

UCLASS()
class USRPGActionCreationCommandHandler : public UObject, public ISRPGCommandHandler
{
	GENERATED_BODY()

	friend class USRPGTurnContext;

protected:
	int8 GetCommandPriority() const override;
	ESRPGCommandResult HandleCommand(const TInstancedStruct<FSRPGCommand>& Command) override;

protected:
	TWeakObjectPtr<USRPGTurnContext> GetParent() const;

protected:
	UPROPERTY(Category = Parent, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "Parent"))
	TWeakObjectPtr<USRPGTurnContext> mParent;
};

UCLASS()
class USRPGDetailInfoPopupCommandHandler : public UObject, public ISRPGCommandHandler
{
	GENERATED_BODY()

	friend class USRPGTurnContext;

protected:
	int8 GetCommandPriority() const override;
	ESRPGCommandResult HandleCommand(const TInstancedStruct<FSRPGCommand>& Command) override;
};

/** @brief 스킬 사용 시 임시 정보를 들고 있는 Context 객체 */
UCLASS()
class USRPGTurnContext : public UObject
{
	GENERATED_BODY()

	friend class USRPGCombatModel;

	/* 생명 주기 함수 */
protected:
	void InitTurn(USRPGCombatModel* Parent, UUnitModel* Owner, int32 TurnId, int32 LifeCount);
	void BeginTurn();
	void TickTurn(float DeltaTime);
	void EndTurn();

public:
	void EvaluateTurnStates(bool ForceAbort = false);
	void OnEndCurrentAction(USRPGAction* Action, ESRPGActionResult ActionResult);

protected:
	void EvaluateTurnEndState(bool ForceAbort);

	/* 액션 처리 함수 */
protected:
	void EnqueueAction(USRPGAction* NewAction);
	void DequeueAction();

protected:
	UFUNCTION()
	void OnHandleCommand(ESRPGCommandResult Result);

	/* 외부 API */
public:
	USRPGCombatModel* GetParent() const;
	UUnitModel* GetOwner() const;

	int32 GetTurnId() const;

	bool IsPermanent() const;
	int32 GetLifeCount() const;

	/* 시뮬 함수 */
protected:
	void ForcedClearActions();
	void ForcedAdvanceUntilNextAction(TInstancedStruct<FSRPGCommand> NextCommand);

public:
	FOnBeginTurnUI OnBeginTurnUI;
	FOnEndTurnUI OnEndTurnUI;
	FOnBeginAnyActionUI OnBeginAnyActionUI;
	FOnEndAnyActionUI OnEndAnyActionUI;
	FOnShowDicePanelAtTurnStartUI OnShowDicePanelAtTurnStartUI;

protected:
	UPROPERTY(Category = Parent, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "Parent"))
	TWeakObjectPtr<USRPGCombatModel> mParent;
	UPROPERTY(Category = Owner, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "Owner"))
	TWeakObjectPtr<UUnitModel> mOwner;
	
protected:
	// @brief 턴 ID
	UPROPERTY(Category = Turn, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TurnId"))
	int32 mTurnId = INDEX_NONE;
	// @brief 현재 턴 상태
	UPROPERTY(Category = Turn, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TurnPhase"))
	ESRPGTurnPhase mTurnPhase = ESRPGTurnPhase::None;
	// @brief 턴 종료 결과
	UPROPERTY(Category = Turn, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TurnResult"))
	ESRPGTurnResult mTurnResult = ESRPGTurnResult::Succeeded;

protected:
	static constexpr int32 PERMENENT_TURN = -1;

	// @brief 남은 턴 수명
	UPROPERTY(Category = Turn, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "LifeCount"))
	int32 mLifeCount = 0;

protected:
	// @brief 예약된 액션들
	UPROPERTY(Category = Action, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "ReservedActions"))
	TArray<TObjectPtr<USRPGAction>> mReservedActions;
	// @brief 액션 큐 처리용 헤드 인덱스
	UPROPERTY(Category = Action, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "HeadActionIndex"))
	int32 mHeadActionIndex = 0;

protected:
	// @brief 기본 핸들러들
	UPROPERTY(Category = Handler, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TurnDefaultCommandHandlers"))
	TArray<TScriptInterface<ISRPGCommandHandler>> mTurnDefaultCommandHandlers;

protected:
	// @brief 액션 종료 시, 중단해야될 필요가 있는지
	bool mShouldTerminateAfterAction = false;
};
