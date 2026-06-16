/*****************************************************************//**
 * @file   SRPGAction.h
 * @brief  사용자 입력에 따른 정해진 SRPG 행동 객체 구현 헤더
 * @author 모호재
 * @date   2026-04-28
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGFramework/SRPGCommandHandler.h"

struct FPresentationBarrier;
struct FSRPGAction;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBeginActionUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const FSRPGAction& /*Action*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnEndActionUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const FSRPGAction& /*Action*/, ESRPGActionResult /*Result*/);

class AUnit;
struct FSRPGTurnContext;
struct FSRPGCommand;

/**
 * @brief  사용자 입력에 따른 정해진 SRPG 행동 객체
 */
struct FSRPGAction : public TSharedFromThis<FSRPGAction>, public ISRPGCommandHandler
{
	friend struct FSRPGTurnContext;

protected:
	FSRPGAction() = default;
	virtual ~FSRPGAction()= default;

	/* 생명 주기 함수 */
protected:
	void InitAction(TSharedRef<FSRPGTurnContext> Parent, AUnit* Instigator);
	void BeginAction();
	void TickAction(float DeltaTime);
	void EndAction(ESRPGActionResult Result);

protected:
	virtual void OnBeginAction();
	virtual void OnTickAction(float DeltaTime);
	virtual void OnEndAction();

protected:
	void EvaluateActionEndState(bool ForceAbort = false);

	/* 액션 커맨드 처리 함수 */
protected:
	int8 GetCommandPriority() const override;
	ESRPGCommandResult HandleCommand(TSharedPtr<const FSRPGCommand> Command) override;

protected:
	void ReserveCommand(TSharedPtr<const FSRPGCommand> Command);

private:
	void FlushCommands();

	/* 헬퍼 함수 */
protected:
	/**
	 * 커서가 어떤 타일 액터를 가리키는지 알아오는 함수. 대상이 없는 경우 FTileIndex::Invalid를 반환
	 * @param Channel 검사할 트래이스 채널명
	 * @param Actor 측정된 액터
	 * @param TileIndex 부딧친 대상의 타일의 인덱스 값
	 */
	void GetTileActorUnderCursor(ECollisionChannel Channel, OUT AActor* Actor, OUT FTileIndex& TileIndex) const;

	/* 외부 API */
public:
	UWorld* GetWorld() const;
	TWeakPtr<FSRPGTurnContext> GetParent() const;
	AUnit* GetInstigator() const;
	ESRPGActionType GetActionType() const;
	bool ConsumesTurn() const;

protected:
	FOnBeginActionUI OnBeginActionUI;
	FOnEndActionUI OnEndActionUI;

protected:
	TWeakPtr<FSRPGTurnContext> mParent;
	TWeakObjectPtr<AUnit> mInstigator;

protected:
	// @brief 예약된 명령들
	TQueue<TSharedPtr<const FSRPGCommand>> mReservedCommands;

protected:
	// @brief 현재 액션 상태
	ESRPGActionPhase mActionPhase = ESRPGActionPhase::None;
	// @brief 액션 종료 결과
	ESRPGActionResult mActionResult = ESRPGActionResult::Succeeded;
	// @brief 액션 타입
	ESRPGActionType mActionType = ESRPGActionType::InPlayAction;
	// @brief 해당 액션의 턴 소모 여부
	bool mConsumesTurn = false;
};

