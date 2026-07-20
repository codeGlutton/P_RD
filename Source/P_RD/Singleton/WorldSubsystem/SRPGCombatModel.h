/*****************************************************************//**
 * @file   SRPGCombatModel.h
 * @brief  SRPG 턴제 전투를 제어하기 위한 서브 시스템의 모델 구현 헤더
 * @author 모호재
 * @date   2026-06-16
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectModel.h"

#include "Tool/CircularList.h"
#include "SRPGFramework/SRPGEnemyIntent.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGFramework/SRPGTurnContext.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"

#include "SRPGCombatModel.generated.h"

 // SRPG Combat 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogSRPGCombat, Log, All)

struct FPresentationBarrier;

class USRPGTurnContext;
class USRPGAction;

class UBoardActorModel;
class IBoardCombatTarget;
class UUnitModel;
class UEnemyUnitModel;
class UTileMapModel;

class UStaticCombatRoomSpawnData;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnRegisterUnitUI, UUnitModel* /*Unit*/)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUnregisterUnitUI, UUnitModel* /*Unit*/)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRegisterObstacleUI, UBoardActorModel* /*Actor*/)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUnregisterObstacleUI, UBoardActorModel* /*Actor*/)

DECLARE_MULTICAST_DELEGATE_OneParam(FOnShowDicePanelAnyTurnUI, const USRPGTurnContext* /*TurnContext*/);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBeginCombatUI, TSharedPtr<FPresentationBarrier> /*Barrier*/)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEndCombatUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, ESRPGCombatResult /*Result*/)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBeginAnyTurnUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const USRPGTurnContext* /*TurnContext*/)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBeginAnyRoundUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, int32 /*RoundCount*/)
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnEndAnyTurnUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const USRPGTurnContext* /*TurnContext*/, ESRPGTurnResult /*Result*/)
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnBeginAnyTurnActionUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const USRPGTurnContext* /*TurnContext*/, const USRPGAction* /*Action*/)
DECLARE_MULTICAST_DELEGATE_FourParams(FOnEndAnyTurnActionUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const USRPGTurnContext* /*TurnContext*/, const USRPGAction* /*Action*/, ESRPGActionResult /*Result*/)

DECLARE_MULTICAST_DELEGATE_OneParam(FOnZoomInToActorsUI, const TArray<FTileIndex>& /*ActorTileIndexes*/);
DECLARE_MULTICAST_DELEGATE(FOnZoomOutFromActorsUI);
DECLARE_MULTICAST_DELEGATE(FOnEnemyIntentsChangedUI);

USTRUCT(BlueprintType)
struct FSRPGTurnUnregisterRequest
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = Request, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TargetTurnContext"))
	TObjectPtr<USRPGTurnContext> mTargetTurnContext;
	UPROPERTY(Category = Request, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TargetOwner"))
	TObjectPtr<UUnitModel> mTargetOwner;
};

/**
 * @brief  SRPG 턴제 전투를 제어하기 위한 서브 시스템 모델
 */
UCLASS()
class P_RD_API USRPGCombatModel : public UObjectModel, public FTickableGameObject
{
	GENERATED_BODY()

	/* UObjectModel 상속 */
public:
	void Serialize(FArchive& Ar) override;

	/* FTickableGameObject 상속 */
public:
	void Tick(float DeltaTime) override;
	bool IsTickable() const override;
	TStatId GetStatId() const override;

	/* 생명 주기 함수 */
public:
	void InitCombat(UStaticCombatRoomSpawnData* RoomSpawnData, UUnitModel* PlayerUnit, const FTransform& RoomStartTransform);
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
	void OnEndCurrentTurn(USRPGTurnContext* TurnContext, ESRPGTurnResult TurnResult);

protected:
	void ClearDeadActors();
	void EvaluateCombatEndState();

	/* 등록 및 해제 함수 */
public:
	/**
	 * 존재하는 유닛의 새로운 턴 등록 함수. 새로운 턴은 항상 라운드 최후방에 등록
	 * @param Owner 유닛 객체
	 * @param LifeCount 턴의 생명 주기. 해당 턴에 대해서 남은 실행 횟수를 의미
	 * @return 생성된 턴 컨텍스트
	 */
	USRPGTurnContext* RegisterTurn(UUnitModel* Owner, int32 LifeCount = USRPGTurnContext::PERMENENT_TURN);

protected:
	/**
	 * 턴을 삭재하는 함수. 해당 턴이 현재 진행 중이면, Pending. 아니라면 즉시 제거.
	 * @param TurnContext 삭제할 TurnContext
	 */
	void UnregisterTurn(USRPGTurnContext* TurnContext);
	/**
	 * Owner가 동일한 모든 턴을 삭재하는 함수. 해당 턴이 현재 진행 중이면, Pending. 아니라면 즉시 제거.
	 * @param Owner 삭제한 Owner
	 */
	void UnregisterTurns(UUnitModel* Owner);

	bool UnregisterTurnImmediately(USRPGTurnContext* TurnContext);
	int32 UnregisterTurnsImmediately(UUnitModel* Owner);
	
	/**
	 * 미뤄두었던 턴 삭제를 처리하는 함수
	 */
	void FlushPendingTurnRequests();

public:
	/**
	 * 새로운 적을 등록하는 함수. 해당 적 턴은 항상 최후방에 등록
	 * @param EnemyPlacementData 적 배치 데이터
	 */
	UEnemyUnitModel* RegisterEnemyUnit(FEnemyUnitPlacementData& EnemyPlacementData);
	/**
	 * 새로운 장애물을 등록하는 함수
	 * @param ObstaclePlacementDatas 장애물 배치 데이터
	 */
	void RegisterObstacle(FObstaclePlacementData& ObstaclePlacementDatas);

public:
	void UnregisterUnit(UUnitModel* Unit);
	void UnregisterObstacle(UBoardActorModel* Obstcle);

protected:
	void SpawnTileMap(const FTransform& RoomStartTransform);
	void RegisterPlayerUnit(UUnitModel* PlayerUnit, const FTileTransform& Transform);
	void RegisterEnemyUnits(TArray<FEnemyUnitPlacementData>& EnemyPlacementDatas);
	void RegisterUnit(UUnitModel* Unit, const FTileTransform& Transform);
	void RegisterObstacles(TArray<FObstaclePlacementData>& ObstaclePlacementDatas);
	void SpawnRoundReinforcements();
	UEnemyUnitModel* SpawnOneReinforcement();
	UEnemyUnitModel* SpawnOneReinforcementAt(const FTileTransform& SpawnTransform);
	void SpawnActionReinforcementIfNeeded();
	void QueueReinforcements(int32 DesiredCount);
	void CommitPendingReinforcements();
	bool FindReinforcementSpawnTile(FTileTransform& OutTransform) const;
	int32 GetReinforcementEnemyCapForRound() const;
	void ApplyHordeEnemyStats(UEnemyUnitModel* EnemyUnit) const;
	bool HasFutureReinforcements() const;

protected:
	/**
	 * 플레이어 행동 뒤 살아 있는 적 전원이 한 번씩 연속 행동한다.
	 * bCompletedPlayerTurn은 방금 끝난 쪽을 명시해 원형 리스트의 단순 다음 노드 대신
	 * 플레이어 -> 대기 적 전체 -> 플레이어 순서로 전환하기 위해 사용한다.
	 */
	void AdvanceTurn(bool IsInitialRound = false, bool bCompletedPlayerTurn = false);
	void NotifyRoundStartIfNeeded(TSharedPtr<FPresentationBarrier> RoundPresentationBarrier);
	void NotifyRoundEnd();
	void RebuildEnemyResponseOrder();
	int32 PopNextEnemyResponseTurnId();
	int32 FindPlayerTurnId() const;

	/* 액션 함수 */
public:
	/**
	 * 유저 입력 처리
	 * @param Action 추가할 액션 객체
	 * @return 액션 등록 여부
	 */
	bool PushAction(USRPGAction* Action);

public:
	USRPGTurnContext* GetCurrentTurnContext() const;
	USRPGTurnContext* GetTurnContext(const UUnitModel* Owner) const;
	TArray<TObjectPtr<USRPGTurnContext>> GetTurnContexts(const UUnitModel* Owner) const;
	TArray<TObjectPtr<USRPGTurnContext>> GetOrderedTurnContexts() const;
	UTileMapModel* GetTileMap() const;

	UUnitModel* GetPlayerUnit() const;
	const TArray<TObjectPtr<UUnitModel>>& GetUnits() const;
	const TArray<TObjectPtr<UBoardActorModel>>& GetObstacles() const;

	int32 GetRoundCount() const;
	const TArray<FTileTransform>& GetPendingReinforcementTransforms() const { return mPendingReinforcementTransforms; }
	int32 GetCurrentReinforcementEnemyCap() const { return GetReinforcementEnemyCapForRound(); }
	int32 GetWarriorCombo() const { return mWarriorCombo; }
	int32 GetWarriorMomentum() const { return mWarriorMomentum; }
	ESRPGWarriorAftermathStance GetWarriorAftermathStance() const { return mWarriorAftermathStance; }
	FText GetWarriorAftermathStanceLabel() const;

	/* 공개 적 의도 */
public:
	const TArray<FSRPGEnemyIntent>& GetEnemyIntents() const;
	/** @brief 플레이어의 실제 이동/스킬 종료 뒤 모든 생존 적의 최신 경로와 공격을 다시 공개한다. */
	void ReplanEnemyIntentsAfterPlayerAction(const USRPGAction* Action, ESRPGActionResult ActionResult);
	void MarkEnemyIntentExecuting(UUnitModel* Enemy, int32 TurnId = INDEX_NONE);
	void ReportPlayerDisplacement(
		UUnitModel* Target,
		const FTileIndex& From,
		const FTileIndex& To,
		int32 DiceValue,
		ESRPGPlayerDisplacementType DisplacementType = ESRPGPlayerDisplacementType::Push);
	void ReportPlayerDisplacementCollision(
		UUnitModel* Target,
		UBoardActorModel* Blocker,
		ESRPGPlayerDisplacementType DisplacementType);
	/** @brief 다리 걸기로 대상 적의 이번 대응 이동 예산을 즉시 줄이고 새 계획을 공개한다. */
	void ReportPlayerStagger(UUnitModel* Target, int32 DiceValue);
	void ReportFixedIntentPathDisrupted(UUnitModel* Enemy, const FText& Reason);
	void ResolveFixedIntentCollision(UUnitModel* Enemy, UBoardActorModel* Blocker);
	void ResolveFixedIntentAttack(UUnitModel* Enemy, const TArray<IBoardCombatTarget*>& ResolvedTargets);
	/** @brief 적 고유 기술이 명중 대상을 강제 이동시킨 결과를 공격자의 공개 의도에 기록한다. */
	void ReportEnemySkillDisplacement(
		UUnitModel* Enemy,
		UUnitModel* Target,
		const FTileIndex& From,
		const FTileIndex& To,
		const FText& SkillName,
		bool bApplyImpactDamage);
	/** @brief 적 고유 기술로 밀리던 대상이 다른 유닛/장애물에 부딪힌 실제 충돌을 해결한다. */
	void ReportEnemySkillCollision(
		UUnitModel* Enemy,
		UUnitModel* Target,
		UBoardActorModel* Blocker,
		const FText& SkillName);
	void RefreshEnemyIntentHighlights();

protected:
	void PrepareEnemyIntents();
	bool BuildEnemyIntentForTurn(USRPGTurnContext* TurnContext, int32 ExecutionOrder, FSRPGEnemyIntent& OutIntent);
	bool RebuildEnemyIntentPlan(FSRPGEnemyIntent& Intent, bool bPreserveSkill);
	void CompleteEnemyIntent(UUnitModel* Enemy);
	FSRPGEnemyIntent* FindEnemyIntent(UUnitModel* Enemy);
	const FSRPGEnemyIntent* FindEnemyIntent(const UUnitModel* Enemy) const;
	void AppendEnemyIntentResult(FSRPGEnemyIntent& Intent, ESRPGEnemyIntentResult Result, const FText& Message);
	void BroadcastEnemyIntentChanged();
	void ActivateWarriorAftermath(const USRPGAction* Action);
	bool TryResolveWarriorAftermath(FSRPGEnemyIntent& Intent);
	void AddWarriorCombo(int32 Amount);

	/* 시뮬 함수 */
public:
	void ForcedAdvanceUntilNextAction(TInstancedStruct<FSRPGCommand> NextCommand, bool NeedEndCurrentAction);
	void ForcedAdvanceUntilNextPlayerTurn(bool NeedEndCurrentAction);

public:
	/**
	 * @brief 유닛 등록 시 알림 대리자
	 */
	FOnRegisterUnitUI OnRegisterUnitUI;
	/**
	 * @brief 유닛 해제 시 알림 대리자
	 */
	FOnUnregisterUnitUI OnUnregisterUnitUI;
	/**
	 * @brief 장애물 등록 시 알림 대리자
	 */
	FOnRegisterObstacleUI OnRegisterObstacleUI;
	/**
	 * @brief 장애물 해제 시 알림 대리자
	 */
	FOnUnregisterObstacleUI OnUnregisterObstacleUI;

public:
	/**
	 * @brief 주사위 굴리기 패널 실행 타이밍 대리자
	 */
	FOnShowDicePanelAnyTurnUI OnShowDicePanelAnyTurnUI;

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
	 * @brief 라운드 시작 시 뜰 UI (라운드당 1회)
	 * @details
	 * OnBeginAnyTurnUI와 동일한 배리어 규약. 방송 지점은 프레임워크 소유자가 AdvanceTurn 라운드 시작부에 연결한다(NotifyRoundStartIfNeeded TODO).
	 */
	FOnBeginAnyRoundUI OnBeginAnyRoundUI;
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

	/** @brief 적의 공개 계획 또는 그 실행 결과가 바뀌었을 때 HUD가 스냅샷을 다시 읽는다. */
	FOnEnemyIntentsChangedUI OnEnemyIntentsChangedUI;

public:
	/**
	 * @brief 액터들 줌인 요청 시
	 */
	FOnZoomInToActorsUI OnZoomInToActorsUI;
	/**
	 * @brief 액터들 줌아웃 요청 시
	 */
	FOnZoomOutFromActorsUI OnZoomOutFromActorsUI;

protected:
	// @brief 현재 전투 방 상태
	UPROPERTY(Category = Combat, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatPhase"))
	ESRPGCombatRoomPhase mCombatPhase = ESRPGCombatRoomPhase::None;
	// @brief 전투 방 결과
	UPROPERTY(Category = Combat, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatResult"))
	ESRPGCombatResult mCombatResult;

protected:
	// @brief 등록된 턴 객체들
	TCircularDoubleLinkedList<int32> mTurnContextOrder;
	TCircularDoubleLinkedList<int32>::TCircularDoubleLinkedListNode* mCurTurnContextOrder = nullptr;
	
	UPROPERTY(Category = Turn, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TurnContextMaxIndex"))
	int32 mTurnContextMaxIndex = 0;
	UPROPERTY(Category = Turn, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TurnContextMap"))
	TMap<int32, TObjectPtr<USRPGTurnContext>> mTurnContextMap;

protected:
	// @brief 미뤄진 턴 제거 요청
	UPROPERTY(Category = Turn, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "PendingTurnRequests"))
	TArray<FSRPGTurnUnregisterRequest> mPendingTurnRequests;
	// @brief 액션 큐 처리용 헤드 인덱스
	UPROPERTY(Category = Turn, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "HeadRequestIndex"))
	int32 mHeadRequestIndex = 0;
	/** @brief 이번 라운드에 아직 행동하지 않은 적 턴 id. 플레이어 행동 뒤 앞에서 하나씩 꺼내 전원을 연속 실행한다. */
	UPROPERTY(Category = Turn, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "PendingEnemyResponseTurnIds"))
	TArray<int32> mPendingEnemyResponseTurnIds;

protected:
	UPROPERTY(Category = Round, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "RoundCount"))
	int32 mRoundCount = 0;
	/** @brief 첫 전투 적 구성을 증원 풀로 재사용한다. 에셋 원본은 수정하지 않는다. */
	UPROPERTY(Category = Round, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "EnemyReinforcementTemplates"))
	TArray<FEnemyUnitPlacementData> mEnemyReinforcementTemplates;
	UPROPERTY(Category = Round, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TotalReinforcementsSpawned"))
	int32 mTotalReinforcementsSpawned = 0;
	UPROPERTY(Category = Round, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "PlayerActionsSinceReinforcement"))
	int32 mPlayerActionsSinceReinforcement = 0;
	/** @brief 한 행동 전부터 바닥에 공개하고 다음 라운드 시작에 실제 투입하는 위치. */
	UPROPERTY(Category = Round, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "PendingReinforcementTransforms"))
	TArray<FTileTransform> mPendingReinforcementTransforms;

	static constexpr int32 ReinforcementFinalRound = 8;
	static constexpr int32 ReinforcementActionInterval = 3;

	/** @brief 라운드 시작에 공개되고 플레이어의 실제 행동이 끝날 때마다 갱신되는 행동 예고. */
	UPROPERTY(Category = Intent, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "EnemyIntents"))
	TArray<FSRPGEnemyIntent> mEnemyIntents;
	UPROPERTY(Category = Intent, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "WarriorAftermathStance"))
	ESRPGWarriorAftermathStance mWarriorAftermathStance = ESRPGWarriorAftermathStance::None;
	ESRPGWarriorAftermathStance mPendingWarriorAftermathStance = ESRPGWarriorAftermathStance::None;
	UPROPERTY(Category = Intent, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "WarriorAftermathTarget"))
	TWeakObjectPtr<UUnitModel> mWarriorAftermathTarget;
	int32 mWarriorAftermathReactions = 0;
	UPROPERTY(Category = Intent, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "WarriorCombo"))
	int32 mWarriorCombo = 0;
	UPROPERTY(Category = Intent, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "WarriorMomentum"))
	int32 mWarriorMomentum = 0;
	int32 mWarriorComboRound = INDEX_NONE;

protected:
	// @brief 배치된 타일맵
	UPROPERTY(Category = TileMap, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TileMap"))
	TObjectPtr<UTileMapModel> mTileMap;

	// @brief 모든 등록 유닛 (플레이어 포함)
	UPROPERTY(Category = BoardActor, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "Units"))
	TArray<TObjectPtr<UUnitModel>> mUnits;
	// @brief 모든 등록 장애물 (타격 가능한 장애물들 포함)
	UPROPERTY(Category = BoardActor, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "Obstacles"))
	TArray<TObjectPtr<UBoardActorModel>> mObstacles;

protected:
	// @brief 등록된 Player 유닛
	UPROPERTY(Category = BoardActor, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "PlayerUnit"))
	TObjectPtr<UUnitModel> mPlayerUnit;
	// @brief 모든 타격 가능 장애물
	UPROPERTY(Category = BoardActor, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatTargets"))
	TArray<TScriptInterface<IBoardCombatTarget>> mCombatTargetObstacles;

protected:
	// @brief 플레이어 턴 진입 시, 중단해야될 필요가 있는지
	bool mShouldTerminateBeforePlayerTurnStart = false;
};
