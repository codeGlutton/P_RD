/*****************************************************************//**
 * @file   SRPGCombatModel.h
 * @brief  SRPG 턴제 전투를 제어하기 위한 서브 시스템의 모델 구현 헤더
 * @author 모호재
 * @date   2026-06-16
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectModel.h"

#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGFramework/SRPGTurnContext.h"

#include "SRPGCombatModel.generated.h"

 // SRPG Combat 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogSRPGCombat, Log, All)

struct FPresentationBarrier;

class USRPGTurnContext;
class USRPGAction;

class UBoardActorModel;
class IBoardCombatTarget;
class UUnitModel;
class UPlayerUnitModel;
class UTileMapModel;

class UStaticCombatRoomSpawnData;
class UStaticObstacleSpawnData;
struct FEnemyUnitPlacementData;
struct FObstaclePlacementData;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnRegisterUnitUI, UUnitModel* /*Unit*/)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUnregisterUnitUI, UUnitModel* /*Unit*/)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRegisterObstacleUI, UBoardActorModel* /*Actor*/)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUnregisterObstacleUI, UBoardActorModel* /*Actor*/)

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBeginCombatUI, TSharedPtr<FPresentationBarrier> /*Barrier*/)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEndCombatUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, ESRPGCombatResult /*Result*/)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBeginAnyTurnUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const USRPGTurnContext* /*TurnContext*/)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBeginAnyRoundUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, int32 /*RoundCount*/)
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnEndAnyTurnUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const USRPGTurnContext* /*TurnContext*/, ESRPGTurnResult /*Result*/)
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnBeginAnyTurnActionUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const USRPGTurnContext* /*TurnContext*/, const USRPGAction* /*Action*/)
DECLARE_MULTICAST_DELEGATE_FourParams(FOnEndAnyTurnActionUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, const USRPGTurnContext* /*TurnContext*/, const USRPGAction* /*Action*/, ESRPGActionResult /*Result*/)

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSaveCombatPlay, const TArray<TObjectPtr<UUnitModel>>& /*PlayerModels*/, int32 /*RoundCount*/, int32 /*TurnCount*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnShowCombatResultUI, ESRPGCombatResult /*Result*/);

/**
 * @brief 턴 후보 데이터
 */
USTRUCT()
struct FSRPGTurnCandidate
{
	GENERATED_BODY()

public:
	bool operator<(const FSRPGTurnCandidate& Other) const
	{
		if (mRemainSpeedPoint == Other.mRemainSpeedPoint)
		{
			return mRandomTieBreaker < Other.mRandomTieBreaker;
		}
		return mRemainSpeedPoint < Other.mRemainSpeedPoint;
	}
	bool operator>(const FSRPGTurnCandidate& Other) const
	{
		return Other < *this;
	}

public:
	float mRandomTieBreaker = 0.f;
	int32 mRemainSpeedPoint = 0;
	int32 mRechargedSpeedPoint = 0;
	TObjectPtr<UUnitModel> mOwner = nullptr;
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
	void InitCombat(UStaticCombatRoomSpawnData* RoomSpawnData, const TArray<TObjectPtr<UPlayerUnitModel>>& PlayerUnits, const FTransform& RoomStartTransform, const FRoomClearData& ClearData);
	void BeginCombat();
	void EndCombat();

protected:
	void InitBoardActorModels(UStaticCombatRoomSpawnData* RoomSpawnData, const TArray<TObjectPtr<UPlayerUnitModel>>& PlayerUnits);
	void RegisterRoundEvents(UStaticCombatRoomSpawnData* RoomSpawnData);
	void RestoreBoardActorModels(UStaticCombatRoomSpawnData* RoomSpawnData, const TArray<TObjectPtr<UPlayerUnitModel>>& PlayerUnits, const FRoomClearData& ClearData);

	/* 전투 진행 함수 */
protected:
	void AdvanceTurn();
	void AdvanceRoundIfNeeded(TSharedPtr<FPresentationBarrier> RoundPresentationBarrier);

	void BeginRound(TSharedPtr<FPresentationBarrier> RoundPresentationBarrier);
	void EndRound(TSharedPtr<FPresentationBarrier> RoundPresentationBarrier);

	/* 이벤트 등록 함수 */
public:
	void AddRoundStartEvent(TInstancedStruct<FSRPGCombatRoundEvent> Event);
	void AddRoundEndEvent(TInstancedStruct<FSRPGCombatRoundEvent> Event);

	/**
	 * @brief 라운드 끝 이벤트 등록. 단, 같은 타입 이벤트가 이미 등록돼 있으면 무시
	 * @details "여러 번 불러도 한 번만 등록"이 필요한 이벤트용.
	 *          예: 장판 일괄 발동 이벤트는 전투에 1개만 있어야 하는데,
	 *          장판들은 자기가 첫 스폰인지 알 수 없어 저마다 등록을 시도함
	 *          → 첫 시도만 등록되고 나머지는 무시됨
	 * @param Event 등록할 이벤트
	 */
	void AddUniqueRoundEndEvent(TInstancedStruct<FSRPGCombatRoundEvent> Event);

protected:
	void TriggerRoundEvents(TSharedPtr<FPresentationBarrier> RoundPresentationBarrier, TArray<TInstancedStruct<FSRPGCombatRoundEvent>>& RoundEvents, int32 EventIndex);

	/* 전투 상태 평가 */
public:
	/**
	 * 현재 턴이 종료되었을 시, 전투를 유지할지 처리하는 함수
	 * @param TurnContext 종료된 현재 턴 객체
	 * @param TurnResult 턴 종료 결과 타입
	 */
	void OnEndCurrentTurn(USRPGTurnContext* TurnContext, ESRPGTurnResult TurnResult);
	/**
	 * 현재 전투 상태를 측정하여, 전투가 종료되어야 할지. 턴이 종료되어야 할지 내부적으로 기록해두는 함수.
	 * ex) 현재 턴의 Owner가 죽은 경우, 턴이 종료되도록 유도
	 * ex) 현재 턴의 Player가 죽은 경우, 턴과 전투가 순차적으로 종료되도록 유도
	 */
	void EvaluateCombatStates();

protected:
	void ClearDeadActorModels();
	void ClearAllCombatTargetModels(bool IgnorePlayers = true);
	void EvaluateCombatEndState();

	/* 턴 등록 및 해제 함수 */
protected:
	/**
	 * 존재하는 유닛의 새로운 턴 등록 함수. 새로운 턴은 항상 라운드 최후방에 등록
	 * @param Owner 유닛 객체
	 * @return 생성된 턴 컨텍스트
	 */
	USRPGTurnContext* RegisterTurn(UUnitModel* Owner);

	/**
	 * 턴을 삭재하는 함수. 해당 턴이 현재 진행 중이면, Pending. 아니라면 즉시 제거.
	 * @param TurnContext 삭제할 TurnContext
	 * @param IgnoreCurTurn 현재 턴을 제거 대상에서 무시할지
	 * @return 제거 여부
	 */
	bool UnregisterTurn(USRPGTurnContext* TurnContext, bool IgnoreCurTurn = true);
	bool UnregisterTurn(UUnitModel* Owner, bool IncludeCurTurn = true);

protected:
	/**
	 * 턴을 정렬하여 새로운 라운드의 턴 객체 후보들 얻기
	 * @param RoundOffset 라운드 진행 오프셋 (음수의 경우, 유효한 라운드에서 종료)
	 * @param Candidates 새로운 라운드의 턴 객체 후보들
	 * @param NextRoundRandomSeed 새로운 라운드의 랜덤 시드값
	 * @return 라운드 진행 가능 여부
	 */
	bool CheckOrderedTurnCandidates(int32 RoundOffset, OUT int32& ResultRoundOffset, OUT TArray<FSRPGTurnCandidate>& Candidates, OUT int32& NextRoundRandomSeed) const;
	void ApplyOrderedTurnCandidates(const TArray<FSRPGTurnCandidate>& Candidates, int32 NextRoundRandomSeed);

	/* 유닛 등록 및 해제 함수 */
public:
	void RegisterTileMapModel(const FTransform& RoomStartTransform);
	void RegisterPlayerUnitModel(UUnitModel* PlayerUnitModel, const FTileTransform& Transform);
	void RegisterEnemyUnitModel(FEnemyUnitPlacementData& EnemyPlacementData);
	void RegisterObstacleModel(FObstaclePlacementData& ObstaclePlacementDatas);

public:
	void UnregisterUnitModel(UUnitModel* UnitModel);
	void UnregisterObstacleModel(UBoardActorModel* ObstcleModel);

private:
	UTileMapModel* SpawnTileMapModel(const FTransform& RoomStartTransform);
	UBoardActorModel* SpawnBoardActorModel(UStaticObstacleSpawnData* SpawnData, const FTileTransform& TileTransform);
	void DestroyBoardActorModel(UBoardActorModel* Model);
	void PlaceBoardActorModel(UBoardActorModel* Model, const FTileTransform& TileTransform);
	void RemoveBoardActorModel(UBoardActorModel* Model);

	/* 액션 등록 함수 */
public:
	/**
	 * 유저 입력 처리
	 * @param Action 추가할 액션 객체
	 * @return 액션 등록 여부
	 */
	bool PushAction(USRPGAction* Action);

	/* 시뮬 함수 */
public:
	void ForcedAdvanceUntilNextAction(TInstancedStruct<FSRPGCommand> NextCommand, bool NeedEndCurrentAction);
	void ForcedAdvanceUntilNextPlayerTurn(bool NeedEndCurrentAction);

	/* 외부 API 함수 */
public:
	bool HasAnyTurnContext() const;
	int32 GetTurnContextCount() const;

	USRPGTurnContext* GetCurrentTurnContext() const;
	USRPGTurnContext* GetTurnContext(const UUnitModel* Owner) const;
	TArray<TObjectPtr<USRPGTurnContext>> GetOrderedTurnContexts() const;
	TArray<FSRPGTurnCandidate> GetOrderedTurnCandidates(uint32 RoundOffset = 0) const;
	void GetValidRoundAndOrderedTurnCandidates(OUT TArray<FSRPGTurnCandidate>& Candidates, OUT int32& RoundOffset) const;
	UTileMapModel* GetTileMap() const;

	const TArray<TObjectPtr<UUnitModel>>& GetPlayerUnits() const;
	const TArray<TObjectPtr<UUnitModel>>& GetUnits() const;
	const TArray<TObjectPtr<UBoardActorModel>>& GetObstacles() const;

	int32 GetRoundCount() const;
	int32 GetTurnCount() const;

	/* 정적 변수 */
private:
	static constexpr float BOARD_ACTOR_DESTROY_DELAY_TIME = 7.f;

	/* 대리자들 */
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
	 * @brief 전투 저장 대리자
	 */
	FOnSaveCombatPlay OnSaveCombatPlay;
	/**
	 * @brief 전투 결과 확인 타이밍 대리자
	 */
	FOnShowCombatResultUI OnShowCombatResultUI;

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

	/* 전투 상태 */
protected:
	UPROPERTY(Category = Combat, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "IsClearCombat"))
	bool mIsClearCombat = false;
	// @brief 현재 전투 방 상태
	UPROPERTY(Category = Combat, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatPhase"))
	ESRPGCombatRoomPhase mCombatPhase = ESRPGCombatRoomPhase::None;
	// @brief 전투 방 결과
	UPROPERTY(Category = Combat, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatResult"))
	ESRPGCombatResult mCombatResult;

	// @brief 진행 라운드
	UPROPERTY(Category = Round, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "RoundCount"))
	int32 mRoundCount = 0;
	// @brief 진행 턴
	UPROPERTY(Category = Turn, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TurnCount"))
	int32 mTurnCount = 0;

	/* 등록된 턴 */
protected:
	// @brief 등록된 턴 객체들
	TDoubleLinkedList<int32> mTurnContextOrders;
	
	UPROPERTY(Category = Turn, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TurnContextMaxIndex"))
	int32 mTurnContextMaxIndex = 0;
	UPROPERTY(Category = Turn, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TurnContextMap"))
	TMap<int32, TObjectPtr<USRPGTurnContext>> mTurnContextMap;

	/* 등록된 객체들 */
protected:
	// @brief 배치된 타일맵
	UPROPERTY(Category = TileMap, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TileMapModel"))
	TObjectPtr<UTileMapModel> mTileMapModel;

	// @brief 모든 등록 유닛
	UPROPERTY(Category = BoardActor, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "UnitModels"))
	TArray<TObjectPtr<UUnitModel>> mUnitModels;
	// @brief 모든 등록 장애물
	UPROPERTY(Category = BoardActor, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "ObstacleModels"))
	TArray<TObjectPtr<UBoardActorModel>> mObstacleModels;

protected:
	// @brief 등록된 Player 유닛 캐싱
	UPROPERTY(Category = BoardActor, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "PlayerUnitModels"))
	TArray<TObjectPtr<UUnitModel>> mPlayerUnitModels;
	// @brief 모든 타격 가능 객체들 캐싱
	UPROPERTY(Category = BoardActor, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatTargetObstacleModels"))
	TArray<TScriptInterface<IBoardCombatTarget>> mCombatTargetObstacleModels;

protected:
	// @brief 등록된 라운드 시작 이벤트
	UPROPERTY(Category = Event, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RoundStartEvents"))
	TArray<TInstancedStruct<FSRPGCombatRoundEvent>> mRoundStartEvents;
	// @brief 등록된 라운드 종료 이벤트
	UPROPERTY(Category = Event, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RoundEndEvents"))
	TArray<TInstancedStruct<FSRPGCombatRoundEvent>> mRoundEndEvents;

protected:
	// @brief 플레이어 턴 진입 시, 중단해야될 필요가 있는지
	bool mShouldTerminateBeforePlayerTurnStart = false;

	/* 임시 객체 */
protected:
	UPROPERTY(Category = Round, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "NextRoundRandomSeed"))
	int32 mNextRoundRandomSeed = INDEX_NONE;
};
