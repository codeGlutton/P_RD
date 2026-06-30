#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "Simulation/Factory/ObjectModelFactory.h"

#include "SRPGFramework/SRPGCommand.h"
#include "SRPGFramework/SRPGTurnEndAction.h"
#include "Setting/RDWorldSettings.h"

#include "Actor/TileMap/TileMapModel.h"
#include "Pawn/UnitModel.h"

#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"
#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"
#include "DataAsset/ObstacleSpawnData/StaticObstacleSpawnData.h"

DEFINE_LOG_CATEGORY(LogSRPGCombat)

void USRPGCombatModel::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsSaving() == true)
	{
		auto* Node = mTurnContextOrder.GetHead();
		int32 TotalContextNum = mTurnContextOrder.Num();
		Ar << TotalContextNum;
		for (int32 i = 0; i < TotalContextNum; ++i)
		{
			Ar << Node->GetValue();
			Node = Node->GetNextNode();
		}
	}
	if (Ar.IsLoading() == true)
	{
		int32 TotalContextNum = INDEX_NONE;
		Ar << OUT TotalContextNum;
		for (int32 i = 0; i < TotalContextNum; ++i)
		{
			int32 TurnIndex = INDEX_NONE;
			Ar << OUT TurnIndex;
			mTurnContextOrder.AddTail(TurnIndex);
		}
	}
}

void USRPGCombatModel::Tick(float DeltaTime)
{
	if (mCombatPhase == ESRPGCombatRoomPhase::CombatPlay && mCurTurnContextOrder != nullptr)
	{
		mTurnContextMap[mCurTurnContextOrder->GetValue()]->TickTurn(DeltaTime);
	}
}

bool USRPGCombatModel::IsTickable() const
{
	return true;
}

TStatId USRPGCombatModel::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USRPGCombatModel, STATGROUP_Tickables);
}

void USRPGCombatModel::InitCombat(UStaticCombatRoomSpawnData* RoomSpawnData, UUnitModel* PlayerUnit)
{
	checkf(RoomSpawnData != nullptr, TEXT("해당하는 룸 정보 탐색 실패"));
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));

	checkf(mCombatPhase == ESRPGCombatRoomPhase::None, TEXT("중복 초기화"));
	mCombatPhase = ESRPGCombatRoomPhase::CombatInit;

	SpawnTileMap();
	RegisterPlayerUnit(PlayerUnit, RoomSpawnData->mPlayerTransform);
	RegisterEnemyUnits(RoomSpawnData->mEnemyUnitPlacementDatas);
	RegisterObstacles(RoomSpawnData->mObstaclePlacementDatas);

	UE_LOG(LogSRPGCombat, Log, TEXT("SRPG 전투 초기화 완료"))
}

void USRPGCombatModel::BeginCombat()
{
	checkf(mCombatPhase == ESRPGCombatRoomPhase::CombatInit, TEXT("전투 시작 전 초기화 우선 필요"));
	mCombatPhase = ESRPGCombatRoomPhase::CombatStart;

	UE_LOG(LogSRPGCombat, Log, TEXT("SRPG 전투 시작"))

	// 전투 시작 시, 보여지는 UI의 애니메이션의 특정 시점 종료 이후 전투 로직이 시작됨
	auto PresentationBarrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateWeakLambda(this, [this]() {
		for (TObjectPtr<UUnitModel>& Unit : mUnits)
		{
			// 현 스텟을 캡처
			/*FGameplayEventData EventData;
			EventData.TargetData = UGASTargetFunctionLibrary::MakeSnapshotTargetDataHandle(Unit);
			EventData.Instigator = Unit;
			EventData.Target = Unit;*/

			// On Start Room 패시브 실행
			//UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Unit, AbilityTags::GameplayAbility_Passive_OnStartRoom, MoveTemp(EventData));
		}
		// 턴 실행
		checkf(mCombatPhase == ESRPGCombatRoomPhase::CombatStart, TEXT("전투 진입 절차 오류"));
		mCombatPhase = ESRPGCombatRoomPhase::CombatPlay;
		AdvanceTurn(true);
		}));
	OnBeginCombatUI.Broadcast(PresentationBarrier);
}

void USRPGCombatModel::EndCombat()
{
	checkf(mCombatPhase == ESRPGCombatRoomPhase::CombatAbort, TEXT("전투 종료 절차 오류"));
	mCombatPhase = ESRPGCombatRoomPhase::CombatEnd;

	for (TObjectPtr<UUnitModel>& Unit : mUnits)
	{
		// 현 스텟을 캡처
		/*FGameplayEventData EventData;
		EventData.TargetData = UGASTargetFunctionLibrary::MakeSnapshotTargetDataHandle(Unit);
		EventData.Instigator = Unit;
		EventData.Target = Unit;*/

		// On End Room 패시브 실행
		//UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Unit, AbilityTags::GameplayAbility_Passive_OnEndRoom, MoveTemp(EventData));
	}

	// 전투 종료 시, 보여지는 UI의 애니메이션의 특정 시점 종료 이후 맵 보상 로직이 시작됨
	auto PresentationBarrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateWeakLambda(this, [this]() {
		
		// TODO : 보상 로직

		UE_LOG(LogSRPGCombat, Log, TEXT("SRPG 전투 종료"))
		}));
	OnEndCombatUI.Broadcast(PresentationBarrier, mCombatResult);
}

void USRPGCombatModel::EvaluateCombatStates()
{
	EvaluateCombatEndState();
	if (mCurTurnContextOrder != nullptr)
	{
		const bool ForceAbort = mCombatPhase == ESRPGCombatRoomPhase::CombatAbort;
		mTurnContextMap[mCurTurnContextOrder->GetValue()]->EvaluateTurnEndState(ForceAbort);
	}
}

void USRPGCombatModel::OnEndCurrentTurn(USRPGTurnContext* TurnContext, ESRPGTurnResult TurnResult)
{
	// 전투 상태 평가
	EvaluateCombatStates();

	// 전투 종료 여부 체크
	if (mCombatPhase == ESRPGCombatRoomPhase::CombatAbort)
	{
		// 강제 중단
		if (mShouldTerminateBeforePlayerTurnStart == true)
		{
			mShouldTerminateBeforePlayerTurnStart = false;
			return;
		}

		EndCombat();
		return;
	}

	// 수명이 다 된 Turn Context 제거
	if (TurnContext->GetLifeCount() == 0)
	{
		UnregisterTurn(TurnContext);
	}

	AdvanceTurn();
}

void USRPGCombatModel::EvaluateCombatEndState()
{
	checkf(mPlayerUnit != nullptr, TEXT("플레이어 미동록 오류"));

	/* 이미 중단 */

	if (mCombatPhase == ESRPGCombatRoomPhase::CombatAbort)
	{
		return;
	}

	/* 플레이어가 죽어서 전투가 종료되는가? */

	if (mPlayerUnit->IsDead() == true)
	{
		mCombatResult = ESRPGCombatResult::PlayerLose;
		mCombatPhase = ESRPGCombatRoomPhase::CombatAbort;
		return;
	}

	/* 적군이 모두 죽어 전투가 종료되는가? */

    // TODO : (추후 삭제 필요) 적 미배치 방(테스트 등)에서 "생존 적 0 == 즉시 승리"로 빠져 입장하자마자 승리 처리되는 것을 막는다.
    const bool NeedToPreventToEndCombatForDebug = true;

	bool AnyEnemyAlive = false;
	for (const TObjectPtr<UUnitModel>& Unit : mUnits)
	{
		if (Unit->GetTeamAttitudeTowards(*mPlayerUnit) == ETeamAttitude::Hostile)
		{
			if (Unit->IsDead() == false)
			{
				AnyEnemyAlive = true;
			}
		}
	}

	if (NeedToPreventToEndCombatForDebug == true && AnyEnemyAlive == false)
	{
		mCombatResult = ESRPGCombatResult::PlayerWin;
		mCombatPhase = ESRPGCombatRoomPhase::CombatAbort;
		return;
	}
}

USRPGTurnContext* USRPGCombatModel::RegisterTurn(UUnitModel* Owner, int32 LifeCount)
{
	USRPGTurnContext* TurnContext = NewObject<USRPGTurnContext>(this);
	TurnContext->InitTurn(this, Owner, mTurnContextMaxIndex++, LifeCount);
	TurnContext->OnBeginTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext) {
		OnBeginAnyTurnUI.Broadcast(Barrier, TurnContext);
		});
	TurnContext->OnEndTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, ESRPGTurnResult Result) {
		OnEndAnyTurnUI.Broadcast(Barrier, TurnContext, Result);
		});
	TurnContext->OnBeginAnyActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action) {
		OnBeginAnyTurnActionUI.Broadcast(Barrier, TurnContext, Action);
		});
	TurnContext->OnEndAnyActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action, ESRPGActionResult Result) {
		OnEndAnyTurnActionUI.Broadcast(Barrier, TurnContext, Action, Result);
		});

	if (mCurTurnContextOrder == nullptr)
	{
		mTurnContextOrder.AddHead(TurnContext->GetTurnId());
		mCurTurnContextOrder = mTurnContextOrder.GetHead();
	}
	else
	{
		mTurnContextOrder.InsertNode(TurnContext->GetTurnId(), mCurTurnContextOrder->GetPrevNode());

	}
	// UE TMap::operator[]는 FindChecked(키 없으면 assert)라 삽입에 쓰면 크래시한다. 새 키는 Add로 삽입.
	mTurnContextMap.Add(TurnContext->GetTurnId(), TurnContext);

	return TurnContext;
}

void USRPGCombatModel::UnregisterTurn(USRPGTurnContext* TurnContext)
{
	if (mTurnContextMap[mCurTurnContextOrder->GetValue()] == TurnContext)
	{
		// 턴 종료 후 예약
		FSRPGTurnUnregisterRequest Request;
		Request.mTargetTurnContext = TurnContext;
		mPendingTurnRequests.Add(Request);
		return;
	}

	UnregisterTurnImmediately(TurnContext);
}

void USRPGCombatModel::UnregisterTurns(UUnitModel* Owner)
{
	if (mTurnContextMap[mCurTurnContextOrder->GetValue()]->GetOwner() == Owner)
	{
		// 턴 종료 후 예약
		FSRPGTurnUnregisterRequest Request;
		Request.mTargetOwner = Owner;
		mPendingTurnRequests.Add(Request);
		return;
	}

	UnregisterTurnsImmediately(Owner);
}

bool USRPGCombatModel::UnregisterTurnImmediately(USRPGTurnContext* TurnContext)
{
	auto* CurNode = mTurnContextOrder.FindNode(TurnContext->GetTurnId());
	if (CurNode != nullptr)
	{
		// 현재 진행 중인 턴을 삭제하게 되어 노드 전환이 필요한지 여부
		const bool NeedToChangeTurnContext = mTurnContextMap[mCurTurnContextOrder->GetValue()] == TurnContext;

		auto* NextNode = mTurnContextOrder.RemoveNode(CurNode);
		mTurnContextMap.Remove(CurNode->GetValue());

		if (NeedToChangeTurnContext == true)
		{
			mCurTurnContextOrder = NextNode;
		}
		return true;
	}
	return false;
}

int32 USRPGCombatModel::UnregisterTurnsImmediately(UUnitModel* Owner)
{
	auto* CurNode = mTurnContextOrder.GetHead();

	int32 UnregisterCount = 0;
	const int32 TotalContextNum = mTurnContextOrder.Num();
	for (int32 i = 0; i < TotalContextNum; ++i)
	{
		auto* NextNode = CurNode->GetNextNode();

		USRPGTurnContext* CurTurnContext = mTurnContextMap[CurNode->GetValue()];
		if (CurTurnContext->GetOwner() == Owner)
		{
			// 현재 진행 중인 턴을 삭제하게 되어 노드 전환이 필요한지 여부
			const bool NeedToChangeTurnContext = mTurnContextMap[mCurTurnContextOrder->GetValue()] == CurTurnContext;

			NextNode = mTurnContextOrder.RemoveNode(CurNode);
			mTurnContextMap.Remove(CurNode->GetValue());
			if (NeedToChangeTurnContext == true)
			{
				mCurTurnContextOrder = NextNode;
			}
			++UnregisterCount;
		}

		CurNode = NextNode;
	}

	return UnregisterCount;
}

void USRPGCombatModel::FlushPendingTurnRequests()
{
	while (mPendingTurnRequests.Num() > mHeadRequestIndex)
	{
		FSRPGTurnUnregisterRequest& Request = mPendingTurnRequests[mHeadRequestIndex];

		USRPGTurnContext* TargetTurnContext = Request.mTargetTurnContext;
		if (TargetTurnContext != nullptr)
		{
			UnregisterTurnImmediately(TargetTurnContext);
		}
		UUnitModel* TargetOwner = Request.mTargetOwner;
		if (TargetOwner != nullptr)
		{
			UnregisterTurnsImmediately(TargetOwner);
		}

		++mHeadRequestIndex;
		if (mHeadRequestIndex > 5)
		{
			mPendingTurnRequests.RemoveAt(0, mHeadRequestIndex);
			mHeadRequestIndex = 0;
		}
	}
}

void USRPGCombatModel::RegisterEnemyUnit(FEnemyUnitPlacementData& EnemyPlacementData)
{
	checkf(mTileMap != nullptr, TEXT("타일맵 미존재"));

	// 적 유닛 스폰 & 초기 위치 등록 (소프트 포인터는 동기 로드해야 한다. .Get()은 미로드 시 null → 크래시)
	UStaticEnemyUnitSpawnData* EnemyUnitSpawnData = EnemyPlacementData.mSpawnData.LoadSynchronous();
	checkf(EnemyUnitSpawnData != nullptr, TEXT("적 유닛 스폰 데이터 로드 실패"));
	UClass* EnemyModelClass = EnemyUnitSpawnData->mModelClass.LoadSynchronous();
	checkf(EnemyModelClass != nullptr, TEXT("적 유닛 ModelClass 로드 실패 — DataAsset의 mModelClass 확인"));
	UUnitModel* EnemyUnit = GetWorldModelFactory(this)->NewModelDeferred<UUnitModel>(EnemyModelClass);
	EnemyUnit->SetStaticSpawnData(EnemyUnitSpawnData);
	EnemyUnit->FinishCreating(mTileMap->TileToWorldTransform(EnemyPlacementData.mTransform));
	EnemyUnit->OnUnitDied.AddDynamic(this, &USRPGCombatModel::OnUnitDied);

	checkf(mTileMap->CanPlace(EnemyPlacementData.mTransform.mIndex, EnemyUnit), TEXT("액터 배치 불가능"));
	
	// 타일 위에 배치
	mTileMap->PlaceActor(EnemyPlacementData.mTransform, EnemyUnit);
	mUnits.Push(EnemyUnit);

	// 턴 등록
	RegisterTurn(EnemyUnit);
}

void USRPGCombatModel::RegisterObstacle(FObstaclePlacementData& ObstaclePlacementData)
{
	checkf(mTileMap != nullptr, TEXT("타일맵 미존재"));

	// 장애물 스폰 & 초기 위치 등록 (소프트 포인터 동기 로드)
	UStaticObstacleSpawnData* ObstacleSpawnData = ObstaclePlacementData.mSpawnData.LoadSynchronous();
	checkf(ObstacleSpawnData != nullptr, TEXT("장애물 스폰 데이터 로드 실패"));
	UClass* ObstacleModelClass = ObstacleSpawnData->mModelClass.LoadSynchronous();
	checkf(ObstacleModelClass != nullptr, TEXT("장애물 ModelClass 로드 실패 — DataAsset의 mModelClass 확인"));
	UBoardActorModel* Obstacle = GetWorldModelFactory(this)->NewModelDeferred<UBoardActorModel>(ObstacleModelClass);
	Obstacle->SetStaticSpawnData(ObstacleSpawnData);
	Obstacle->FinishCreating(mTileMap->TileToWorldTransform(ObstaclePlacementData.mTransform));

	checkf(mTileMap->CanPlace(ObstaclePlacementData.mTransform.mIndex, Obstacle), TEXT("액터 배치 불가능"));

	// 타일 위에 배치
	mTileMap->PlaceActor(ObstaclePlacementData.mTransform, Obstacle);
}

void USRPGCombatModel::UnregisterUnit(UUnitModel* Unit)
{

}

void USRPGCombatModel::UnregisterObstacle(UBoardActorModel* Obstcle)
{

}

void USRPGCombatModel::SpawnTileMap()
{
	checkf(mTileMap == nullptr, TEXT("이미 타일 존재"));

	// 타일맵 스폰
	mTileMap = GetWorldModelFactory(this)->NewModel<UTileMapModel>();

	// 모델이 자기 타일 저장소(mTiles)를 직접 빌드한다.
	// 기존엔 View(ATileMap)만 RebuildTiles를 호출해서, View 스폰/타이밍에 따라 모델 타일이 비어 있었고
	// 이어지는 유닛 배치(PlaceActor→CanPlace)가 빈 타일맵에서 크래시했다. 배치 전에 모델이 직접 빌드한다.
	mTileMap->RebuildTiles();
}

void USRPGCombatModel::RegisterPlayerUnit(UUnitModel* PlayerUnit, const FTileTransform& Transform)
{
	checkf(mTileMap != nullptr, TEXT("타일맵 미존재"));
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));

	PlayerUnit->OnUnitDied.AddDynamic(this, &USRPGCombatModel::OnUnitDied);

	// 타일 위에 배치
	mTileMap->PlaceActor(Transform, PlayerUnit);
	mPlayerUnit = PlayerUnit;
	mUnits.Push(PlayerUnit);

	// 턴 등록
	RegisterTurn(PlayerUnit);
}

void USRPGCombatModel::RegisterEnemyUnits(TArray<FEnemyUnitPlacementData>& EnemyPlacementDatas)
{
	checkf(mTileMap != nullptr, TEXT("타일맵 미존재"));

	// 우선 순위 별로 유닛 재정렬
	TSortedMap<int32, TArray<FEnemyUnitPlacementData*>> RegisterMap;
	for (FEnemyUnitPlacementData& PlacementData : EnemyPlacementDatas)
	{
		// UE TSortedMap::operator[]는 FindChecked(키 없으면 assert)라 삽입에 쓰면 크래시. 새 키는 FindOrAdd로.
		RegisterMap.FindOrAdd(PlacementData.mTurnPriority).Push(&PlacementData);
	}

	// 동일 우선 순위 유닛은 랜덤 등록
	for (auto& RegisterPair : RegisterMap)
	{
		Algo::RandomShuffle(RegisterPair.Value);
		for (FEnemyUnitPlacementData*& Data : RegisterPair.Value)
		{
			RegisterEnemyUnit(*Data);
		}
	}
}

void USRPGCombatModel::RegisterObstacles(TArray<FObstaclePlacementData>& ObstaclePlacementDatas)
{
	checkf(mTileMap != nullptr, TEXT("타일맵 미존재"));

	for (FObstaclePlacementData& PlacementData : ObstaclePlacementDatas)
	{
		RegisterObstacle(PlacementData);
	}
}

void USRPGCombatModel::AdvanceTurn(bool IsInitialRound)
{
	if (IsInitialRound == false)
	{
		// 라운드 종료 시 이벤트 처리
		NotifyRoundEndIfNeeded();

		// 다음 턴 진행
		mCurTurnContextOrder = mCurTurnContextOrder->GetNextNode();
		// 밀려있던 턴 구성 변경 요청안들 처리
		FlushPendingTurnRequests();
	}

	// 강제 중단
	if (mShouldTerminateBeforePlayerTurnStart == true && mTurnContextMap[mCurTurnContextOrder->GetValue()]->GetOwner()->IsPlayerUnitModel() == true)
	{
		mShouldTerminateBeforePlayerTurnStart = false;
		return;
	}

	// 라운드 시작 시 이벤트 처리
	NotifyRoundStartIfNeeded();

	mTurnContextMap[mCurTurnContextOrder->GetValue()]->BeginTurn();
}

void USRPGCombatModel::NotifyRoundStartIfNeeded()
{
	if (mTurnContextOrder.IsEmpty() == false && mCurTurnContextOrder == mTurnContextOrder.GetHead())
	{
		for (const TObjectPtr<UUnitModel>& Unit : mUnits)
		{
			Unit->OnBeginRound();
		}
		for (const TObjectPtr<UBoardActorModel>& Obstacle : mObstacles)
		{
			Obstacle->OnBeginRound();
		}
	}
}

void USRPGCombatModel::NotifyRoundEndIfNeeded()
{
	if (mTurnContextOrder.IsEmpty() == false && mCurTurnContextOrder == mTurnContextOrder.GetTail())
	{
		for (const TObjectPtr<UUnitModel>& Unit : mUnits)
		{
			Unit->OnEndRound();
		}
		for (const TObjectPtr<UBoardActorModel>& Obstacle : mObstacles)
		{
			Obstacle->OnEndRound();
		}
	}
}

void USRPGCombatModel::OnUnitDied(UUnitModel* UnitModel)
{
	UnregisterTurns(UnitModel);
	EvaluateCombatStates();
}

bool USRPGCombatModel::PushAction(USRPGAction* Action)
{
	checkf(Action != nullptr, TEXT("유효하지 않은 액션"));

	if (mCurTurnContextOrder == nullptr || mTurnContextMap[mCurTurnContextOrder->GetValue()] == nullptr)
	{
		UE_LOG(LogSRPGCombat, Log, TEXT("현재 등록된 턴 객체가 존재하지 않음"));
		return false;
	}

	mTurnContextMap[mCurTurnContextOrder->GetValue()]->EnqueueAction(Action);
	return true;
}

USRPGTurnContext* USRPGCombatModel::GetCurrentTurnContext()
{
	if (mCurTurnContextOrder == nullptr)
	{
		return nullptr;
	}
	return mTurnContextMap[mCurTurnContextOrder->GetValue()];
}

USRPGTurnContext* USRPGCombatModel::GetTurnContext(const UUnitModel* Owner)
{
	auto* HeadNode = mTurnContextOrder.GetHead();
	const int32 TurnCount = mTurnContextOrder.Num();
	for (int32 i = 0; i < TurnCount; ++i)
	{
		if (mTurnContextMap[mCurTurnContextOrder->GetValue()]->GetOwner() == Owner)
		{
			return mTurnContextMap[mCurTurnContextOrder->GetValue()];
		}
		HeadNode = HeadNode->GetNextNode();
	}
	return nullptr;
}

TArray<TObjectPtr<USRPGTurnContext>> USRPGCombatModel::GetTurnContexts(const UUnitModel* Owner)
{
	TArray<TObjectPtr<USRPGTurnContext>> Contexts;

	auto* HeadNode = mTurnContextOrder.GetHead();
	const int32 TurnCount = mTurnContextOrder.Num();
	for (int32 i = 0; i < TurnCount; ++i)
	{
		if (mTurnContextMap[mCurTurnContextOrder->GetValue()]->GetOwner() == Owner)
		{
			Contexts.Push(mTurnContextMap[mCurTurnContextOrder->GetValue()]);
		}
		HeadNode = HeadNode->GetNextNode();
	}

	return Contexts;
}

UTileMapModel* USRPGCombatModel::GetTileMap()
{
	return mTileMap;
}

TArray<TObjectPtr<UUnitModel>>& USRPGCombatModel::GetUnits()
{
	return mUnits;
}

TArray<TObjectPtr<UBoardActorModel>>& USRPGCombatModel::GetObstacles()
{
	return mObstacles;
}

void USRPGCombatModel::ForcedAdvanceUntilNextAction(TInstancedStruct<FSRPGCommand> NextCommand, bool NeedEndCurrentAction)
{
	checkf(mCurTurnContextOrder != nullptr, TEXT("현재 전투가 진행 중이 아님"));
	TObjectPtr<USRPGTurnContext>& CurTurnContext = mTurnContextMap[mCurTurnContextOrder->GetValue()];

	if (NeedEndCurrentAction == true)
	{
		CurTurnContext->ForcedClearActions();
	}
	CurTurnContext->ForcedAdvanceUntilNextAction(MoveTemp(NextCommand));
}

void USRPGCombatModel::ForcedAdvanceUntilNextPlayerTurn(bool NeedEndCurrentAction)
{
	checkf(mCurTurnContextOrder != nullptr, TEXT("현재 전투가 진행 중이 아님"));
	TObjectPtr<USRPGTurnContext>& CurTurnContext = mTurnContextMap[mCurTurnContextOrder->GetValue()];

	if (NeedEndCurrentAction == true)
	{
		CurTurnContext->ForcedClearActions();
	}

	mShouldTerminateBeforePlayerTurnStart = true;

	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 서브시스템 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> TurnEndCommand;
	TurnEndCommand.InitializeAs<FSRPGTurnEndCommand>();

	CommandRouterModel->SummitCommand(TurnEndCommand);
}

