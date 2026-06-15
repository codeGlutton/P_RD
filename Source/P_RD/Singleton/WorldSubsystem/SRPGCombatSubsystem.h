/*****************************************************************//**
 * @file   SRPGCombatSubsystem.h
 * @brief  SRPG 턴제 전투를 제어하기 위한 서브 시스템 구현 헤더
 * @author 모호재
 * @date   2026-05-06
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "Tool/CircularList.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGFramework/SRPGTurnContext.h"

#include "SRPGCombatSubsystem.generated.h"

// SRPG Combat 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogSRPGCombat, Log, All)

struct FPresentationBarrier;
struct FSRPGAction;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBeginCombatUI, TSharedPtr<FPresentationBarrier> /*Barrier*/)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEndCombatUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, ESRPGCombatResult /*Result*/)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBeginAnyTurnUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const FSRPGTurnContext& /*TurnContext*/)
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnEndAnyTurnUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const FSRPGTurnContext& /*TurnContext*/, ESRPGTurnResult /*Result*/)
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnBeginAnyTurnActionUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const FSRPGTurnContext& /*TurnContext*/, const FSRPGAction& /*Action*/)
DECLARE_MULTICAST_DELEGATE_FourParams(FOnEndAnyTurnActionUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const FSRPGTurnContext& /*TurnContext*/, const FSRPGAction& /*Action*/, ESRPGActionResult /*Result*/)

struct FSRPGActionCommand;

class ITileActor;
class AUnit;
class ATileMap;

class UStaticCombatRoomSpawnData;
struct FEnemyUnitPlacementData;
struct FObstaclePlacementData;

USTRUCT()
struct FSRPGTurnUnregisterRequest
{
	GENERATED_BODY()

public:
	TSharedPtr<FSRPGTurnContext> mTargetTurnContext;
	TObjectPtr<const AUnit> mTargetOwner;
};

/**
 * @brief  SRPG 턴제 전투를 제어하기 위한 서브 시스템
 */
UCLASS()
class P_RD_API USRPGCombatSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

	/* UTickableWorldSubsystem 상속 */
public:
	void Tick(float DeltaTime) override;
	TStatId GetStatId() const override;

	/* 생명 주기 함수 */
public:
	void InitCombat(const UStaticCombatRoomSpawnData* RoomSpawnData, AUnit* PlayerUnit);
	void BeginCombat();
	void EndCombat();

public:
	/**
	 * 현재 전투 상태를 측정하여, 전투가 종료되어야 할지. 턴이 종료되어야 할지 내부적으로 기록해두는 함수.
	 * ex) 현재 턴의 Owner가 죽은 경우, 턴이 종료되도록 유도
	 * ex) 현재 턴의 Player가 죽은 경우, 턴과 전투가 순차적으로 종료되도록 유도
	 */
	void EvaluateCombatStates();
	/**
	 * 현재 턴이 종료되었을 시, 전투를 유지할지 처리하는 함수
	 * @param TurnContext 종료된 현재 턴 객체
	 * @param TurnResult 턴 종료 결과 타입
	 */
	void OnEndCurrentTurn(TSharedRef<FSRPGTurnContext> TurnContext, ESRPGTurnResult TurnResult);

protected:
	void EvaluateCombatEndState();

	/* 등록 및 해제 함수 */
public:
	/**
	 * 존재하는 유닛의 새로운 턴 등록 함수. 새로운 턴은 항상 라운드 최후방에 등록
	 * @param Owner 유닛 객체
	 * @param LifeCount 턴의 생명 주기. 해당 턴에 대해서 남은 실행 횟수를 의미
	 * @return 생성된 턴 컨텍스트
	 */
	TWeakPtr<FSRPGTurnContext> RegisterTurn(AUnit* Owner, int32 LifeCount = FSRPGTurnContext::PERMENENT_TURN);

protected:
	/**
	 * 턴을 삭재하는 함수. 해당 턴이 현재 진행 중이면, Pending. 아니라면 즉시 제거.
	 * @param TurnContext 삭제할 TurnContext
	 */
	void UnregisterTurn(TSharedRef<FSRPGTurnContext> TurnContext);
	/**
	 * Owner가 동일한 모든 턴을 삭재하는 함수. 해당 턴이 현재 진행 중이면, Pending. 아니라면 즉시 제거.
	 * @param Owner 삭제한 Owner
	 */
	void UnregisterTurns(const AUnit* Owner);

	bool UnregisterTurnImmediately(TSharedRef<FSRPGTurnContext> TurnContext);
	int32 UnregisterTurnsImmediately(const AUnit* Owner);
	
	/**
	 * 미뤄두었던 턴 삭제를 처리하는 함수
	 */
	void FlushPendingTurnRequests();

public:
	/**
	 * 새로운 적을 등록하는 함수. 해당 적 턴은 항상 최후방에 등록
	 * @param EnemyPlacementData 적 배치 데이터
	 */
	void RegisterEnemyUnit(const FEnemyUnitPlacementData& EnemyPlacementData);
	/**
	 * 새로운 장애물을 등록하는 함수
	 * @param ObstaclePlacementDatas 장애물 배치 데이터
	 */
	void RegisterObstacle(const FObstaclePlacementData& ObstaclePlacementDatas);

protected:
	void SpawnTileMap();
	void RegisterPlayerUnit(AUnit* PlayerUnit, const FTileTransform& Transform);
	void RegisterEnemyUnits(const TArray<FEnemyUnitPlacementData>& EnemyPlacementDatas);
	void RegisterObstacles(const TArray<FObstaclePlacementData>& ObstaclePlacementDatas);

protected:
	void AdvanceTurn(bool IsInitialRound = false);
	void NotifyRoundStartIfNeeded();
	void NotifyRoundEndIfNeeded();

	/* 커맨드 함수 */
public:
	/**
	 * 유저 입력 처리
	 * @param Command 처리할 명령 객체
	 * @return 명령 처리 여부
	 */
	bool SummitCommand(TSharedPtr<const FSRPGActionCommand> Command);

public:
	TWeakPtr<FSRPGTurnContext> GetCurrentTurnContext();
	TWeakPtr<FSRPGTurnContext> GetTurnContext(const AUnit* Owner);
	TArray<TWeakPtr<FSRPGTurnContext>> GetTurnContexts(const AUnit* Owner);
	ATileMap* GetTileMap();
	TArray<TObjectPtr<AUnit>>& GetUnits();

public:
	/**
	 * @brief 전투 시작 시 뜰 UI
	 * @details
	 * Broadcast시 전달될 Barrier 스마트 포인터 카운팅이 0이 되기 전까지, 로직은 일시정지
	 */
	FOnBeginCombatUI OnBeginCombatUI;
	/**
	 * @brief 전투 종료 시 뜰 UI
	 * @details
	 * Broadcast시 전달될 Barrier 스마트 포인터 카운팅이 0이 되기 전까지, 로직은 일시정지
	 */
	FOnEndCombatUI OnEndCombatUI;
	/**
	 * @brief 턴 시작 시 뜰 UI
	 * @details
	 * Broadcast시 전달될 Barrier 스마트 포인터 카운팅이 0이 되기 전까지, 로직은 일시정지
	 */
	FOnBeginAnyTurnUI OnBeginAnyTurnUI;
	/**
	 * @brief 턴 종료 시 뜰 UI
	 * @details
	 * Broadcast시 전달될 Barrier 스마트 포인터 카운팅이 0이 되기 전까지, 로직은 일시정지
	 */
	FOnEndAnyTurnUI OnEndAnyTurnUI;
	/**
	 * @brief 액션 시작 시 뜰 UI
	 * @details
	 * Broadcast시 전달될 Barrier 스마트 포인터 카운팅이 0이 되기 전까지, 로직은 일시정지
	 */
	FOnBeginAnyTurnActionUI OnBeginAnyTurnActionUI;
	/**
	 * @brief 액션 종료 시 뜰 UI
	 * @details
	 * Broadcast시 전달될 Barrier 스마트 포인터 카운팅이 0이 되기 전까지, 로직은 일시정지
	 */
	FOnEndAnyTurnActionUI OnEndAnyTurnActionUI;

protected:
	// @brief 현재 전투 방 상태
	ESRPGCombatRoomPhase mCombatPhase = ESRPGCombatRoomPhase::None;
	// @brief 전투 방 결과
	ESRPGCombatResult mCombatResult;

	// @brief 등록된 턴 객체들
	TCircularDoubleLinkedList<TSharedPtr<FSRPGTurnContext>> mTurnContexts;
	TCircularDoubleLinkedList<TSharedPtr<FSRPGTurnContext>>::TCircularDoubleLinkedListNode* mCurTurnContextNode = nullptr;
	
	// @brief 미뤄진 턴 제거 요청
	TQueue<FSRPGTurnUnregisterRequest> mPendingTurnRequests;

protected:
	// @brief 배치된 타일맵
	TObjectPtr<ATileMap> mTileMap;

	// @brief 등록된 Player 유닛
	TObjectPtr<AUnit> mPlayerUnit;
	// @brief 모든 등록 유닛 (플레이어 포함)
	TArray<TObjectPtr<AUnit>> mUnits;
	// @brief 모든 등록 장애물
	TArray<TScriptInterface<ITileActor>> mObstacles;
};
