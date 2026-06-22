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
#include "SRPGAction.generated.h"

struct FPresentationBarrier;

class UUnitModel;

class USRPGTurnContext;
class USRPGAction;
struct FSRPGCommand;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBeginActionUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const USRPGAction* /*Action*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnEndActionUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const USRPGAction* /*Action*/, ESRPGActionResult /*Result*/);

/**
 * @brief  사용자 입력에 따른 정해진 SRPG 행동 객체
 */
UCLASS(abstract)
class USRPGAction : public UObject, public ISRPGCommandHandler
{
	GENERATED_BODY()

	friend class USRPGTurnContext;
	friend class USRPGActionCreationCommandHandler;

	/* 생명 주기 함수 */
protected:
	void InitAction(USRPGTurnContext* Parent, UUnitModel* Instigator);
	void BeginAction();
	void TickAction(float DeltaTime);
	void EndAction();

protected:
	/**
	 * 액션 내부에서 종료됨을 알리는 함수
	 * @param Result 결과
	 */
	void MarkActionCompleted(ESRPGActionResult Result);
	void TryEndAction();

protected:
	virtual void OnBeginAction();
	virtual void OnTickAction(float DeltaTime);
	virtual void OnEndAction();

protected:
	void EvaluateActionEndState(bool ForceAbort = false);

	/* 액션 커맨드 처리 함수 */
protected:
	int8 GetCommandPriority() const override;
	ESRPGCommandResult HandleCommand(const TInstancedStruct<FSRPGCommand>& Command) override;

protected:
	void ReserveInitializeCommand(TInstancedStruct<FSRPGCommand> Command);

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
	TWeakObjectPtr<USRPGTurnContext> GetParent() const;
	UUnitModel* GetInstigator() const;
	ESRPGActionType GetActionType() const;
	bool ConsumesTurn() const;

protected:
	FOnBeginActionUI OnBeginActionUI;
	FOnEndActionUI OnEndActionUI;

protected:
	UPROPERTY(Category = Parent, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "Parent"))
	TWeakObjectPtr<USRPGTurnContext> mParent;
	UPROPERTY(Category = Instigator, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "Instigator"))
	TWeakObjectPtr<UUnitModel> mInstigator;

protected:
	// @brief 예약된 초기화 명령
	UPROPERTY(Category = Command, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "InitializeCommand"))
	TInstancedStruct<FSRPGCommand> mInitializeCommand;

protected:
	// @brief 현재 액션 상태
	UPROPERTY(Category = Action, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "ActionPhase"))
	ESRPGActionPhase mActionPhase = ESRPGActionPhase::None;
	// @brief 액션 종료 결과
	UPROPERTY(Category = Action, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "ActionResult"))
	ESRPGActionResult mActionResult = ESRPGActionResult::Succeeded;
	// @brief 액션 타입
	UPROPERTY(Category = Action, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "ActionType"))
	ESRPGActionType mActionType = ESRPGActionType::InPlayAction;
	// @brief 해당 액션의 턴 소모 여부
	UPROPERTY(Category = Action, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "ConsumesTurn"))
	bool mConsumesTurn = false;
};

