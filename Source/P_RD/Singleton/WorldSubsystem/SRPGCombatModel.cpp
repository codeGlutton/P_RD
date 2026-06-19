#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/PresentationSyncSubsystem.h"
#include "SRPGFramework/SRPGCommand.h"
#include "Setting/RDWorldSettings.h"

#include "Actor/TileMap/TileMap.h"
#include "Pawn/Unit.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"
#include "DataAsset/ObstacleSpawnData/StaticObstacleSpawnData.h"

#include "AIController/EnemyAIController.h"

#include "FunctionLibrary/GASTargetFunctionLibrary.h"

void USRPGCombatModel::Tick(float DeltaTime)
{
	if (mCombatPhase == ESRPGCombatRoomPhase::CombatPlay && mCurTurnContextNode != nullptr)
	{
		mCurTurnContextNode->GetValue()->TickTurn(DeltaTime);
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

void USRPGCombatModel::Initialize()
{
	USRPGCombatSubsystem* SRPGCombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	checkf(SRPGCombatSubsystem != nullptr, TEXT("SRPG 전투 서브시스템 nullptr"));

	SRPGCombatSubsystem->BindModel(this);
}

void USRPGCombatModel::InitCombat(const UStaticCombatRoomSpawnData* RoomSpawnData, AUnit* PlayerUnit)
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

	UPresentationSyncSubsystem* PresentationSyncSubsystem = GetWorld()->GetSubsystem<UPresentationSyncSubsystem>();
	checkf(PresentationSyncSubsystem != nullptr, TEXT("연출 동기화 서브시스템 nullptr"));

	// 전투 시작 시, 보여지는 UI의 애니메이션의 특정 시점 종료 이후 전투 로직이 시작됨
	auto PresentationBarrier = PresentationSyncSubsystem->MakePresentationBarrier(FOnFinishPresentation::CreateWeakLambda(this, [this]() {
		for (TObjectPtr<AUnit>& Unit : mUnits)
		{
			// 현 스텟을 캡처
			FGameplayEventData EventData;
			EventData.TargetData = UGASTargetFunctionLibrary::MakeSnapshotTargetDataHandle(Unit);
			EventData.Instigator = Unit;
			EventData.Target = Unit;

			// On Start Room 패시브 실행
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Unit, AbilityTags::GameplayAbility_Passive_OnStartRoom, MoveTemp(EventData));
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

	for (TObjectPtr<AUnit>& Unit : mUnits)
	{
		// 현 스텟을 캡처
		FGameplayEventData EventData;
		EventData.TargetData = UGASTargetFunctionLibrary::MakeSnapshotTargetDataHandle(Unit);
		EventData.Instigator = Unit;
		EventData.Target = Unit;

		// On End Room 패시브 실행
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Unit, AbilityTags::GameplayAbility_Passive_OnEndRoom, MoveTemp(EventData));
	}

	UPresentationSyncSubsystem* PresentationSyncSubsystem = GetWorld()->GetSubsystem<UPresentationSyncSubsystem>();
	checkf(PresentationSyncSubsystem != nullptr, TEXT("연출 동기화 서브시스템 nullptr"));

	// 전투 종료 시, 보여지는 UI의 애니메이션의 특정 시점 종료 이후 맵 보상 로직이 시작됨
	auto PresentationBarrier = PresentationSyncSubsystem->MakePresentationBarrier(FOnFinishPresentation::CreateWeakLambda(this, [this]() {
		
		// TODO : 보상 로직

		UE_LOG(LogSRPGCombat, Log, TEXT("SRPG 전투 종료"))
		}));
	OnEndCombatUI.Broadcast(PresentationBarrier, mCombatResult);
}

void USRPGCombatModel::EvaluateCombatStates()
{
	EvaluateCombatEndState();
	if (mCurTurnContextNode != nullptr)
	{
		const bool ForceAbort = mCombatPhase == ESRPGCombatRoomPhase::CombatAbort;
		mCurTurnContextNode->GetValue()->EvaluateTurnEndState(ForceAbort);
	}
}

void USRPGCombatModel::OnEndCurrentTurn(TSharedRef<FSRPGTurnContext> TurnContext, ESRPGTurnResult TurnResult)
{
	// 전투 상태 평가
	EvaluateCombatStates();

	// 전투 종료 여부 체크
	if (mCombatPhase == ESRPGCombatRoomPhase::CombatAbort)
	{
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

	// 적이 한 번이라도 배치됐는지(생존/사망 무관)와 현재 생존 여부를 함께 본다.
	// 적 미배치 방(테스트 등)에서 "생존 적 0 == 즉시 승리"로 빠져 입장하자마자 승리 처리되는 것을 막는다.
	// → 적이 애초에 없었으면 승리로 끝내지 않고 전투를 유지한다. (PR #115; 리팩토링으로 모델 이관 시 누락된 게이트 재적용)
	bool AnyEnemyAlive = false;
	bool AnyEnemyExists = false;
	for (const TObjectPtr<AUnit>& Unit : mUnits)
	{
		if (Unit->GetTeamAttitudeTowards(*mPlayerUnit) == ETeamAttitude::Hostile)
		{
			AnyEnemyExists = true;
			if (Unit->IsDead() == false)
			{
				AnyEnemyAlive = true;
			}
		}
	}
	// AnyEnemyExists gate가 핵심 불변식이다. "적이 없음"과 "있던 적을 모두 처치함"은 전투 결과가 다르다.
	if (AnyEnemyExists && AnyEnemyAlive == false)
	{
		mCombatResult = ESRPGCombatResult::PlayerWin;
		mCombatPhase = ESRPGCombatRoomPhase::CombatAbort;
		return;
	}
}

TWeakPtr<FSRPGTurnContext> USRPGCombatModel::RegisterTurn(AUnit* Owner, int32 LifeCount)
{
	TSharedPtr<FSRPGTurnContext> TurnContext = TSharedPtr<FSRPGTurnContext>(new FSRPGTurnContext(), [](FSRPGTurnContext* TurnContext) {
		delete TurnContext;
		});
	TurnContext->InitTurn(this, Owner, LifeCount);
	TurnContext->OnBeginTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const FSRPGTurnContext& TurnContext) {
		OnBeginAnyTurnUI.Broadcast(Barrier, TurnContext);
		});
	TurnContext->OnEndTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const FSRPGTurnContext& TurnContext, ESRPGTurnResult Result) {
		OnEndAnyTurnUI.Broadcast(Barrier, TurnContext, Result);
		});
	TurnContext->OnBeginAnyActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const FSRPGTurnContext& TurnContext, const FSRPGAction& Action) {
		OnBeginAnyTurnActionUI.Broadcast(Barrier, TurnContext, Action);
		});
	TurnContext->OnEndAnyActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const FSRPGTurnContext& TurnContext, const FSRPGAction& Action, ESRPGActionResult Result) {
		OnEndAnyTurnActionUI.Broadcast(Barrier, TurnContext, Action, Result);
		});

	if (mCurTurnContextNode == nullptr)
	{
		mTurnContexts.AddHead(TurnContext);
		mCurTurnContextNode = mTurnContexts.GetHead();
	}
	else
	{
		mTurnContexts.InsertNode(TurnContext, mCurTurnContextNode->GetPrevNode());
	}

	return TurnContext;
}

void USRPGCombatModel::UnregisterTurn(TSharedRef<FSRPGTurnContext> TurnContext)
{
	if (mCurTurnContextNode->GetValue() == TurnContext)
	{
		// 턴 종료 후 예약
		FSRPGTurnUnregisterRequest Request;
		Request.mTargetTurnContext = TurnContext;
		mPendingTurnRequests.Enqueue(Request);
		return;
	}

	UnregisterTurnImmediately(TurnContext);
}

void USRPGCombatModel::UnregisterTurns(const AUnit* Owner)
{
	if (mCurTurnContextNode->GetValue()->GetOwner() == Owner)
	{
		// 턴 종료 후 예약
		FSRPGTurnUnregisterRequest Request;
		Request.mTargetOwner = Owner;
		mPendingTurnRequests.Enqueue(Request);
		return;
	}

	UnregisterTurnsImmediately(Owner);
}

bool USRPGCombatModel::UnregisterTurnImmediately(TSharedRef<FSRPGTurnContext> TurnContext)
{
	auto* CurNode = mTurnContexts.FindNode(TurnContext);
	if (CurNode != nullptr)
	{
		// 현재 진행 중인 턴을 삭제하게 되어 노드 전환이 필요한지 여부
		const bool NeedToChangeTurnContext = mCurTurnContextNode->GetValue() == TurnContext;

		auto* NextNode = mTurnContexts.RemoveNode(CurNode);
		if (NeedToChangeTurnContext == true)
		{
			mCurTurnContextNode = NextNode;
		}
		return true;
	}
	return false;
}

int32 USRPGCombatModel::UnregisterTurnsImmediately(const AUnit* Owner)
{
	auto* CurNode = mTurnContexts.GetHead();

	int32 UnregisterCount = 0;
	const int32 TotalContextNum = mTurnContexts.Num();
	for (int32 i = 0; i < TotalContextNum; ++i)
	{
		auto* NextNode = CurNode->GetNextNode();

		TSharedPtr<FSRPGTurnContext> CurTurnContext = CurNode->GetValue();
		if (CurTurnContext->GetOwner() == Owner)
		{
			// 현재 진행 중인 턴을 삭제하게 되어 노드 전환이 필요한지 여부
			const bool NeedToChangeTurnContext = mCurTurnContextNode->GetValue() == CurTurnContext;

			NextNode = mTurnContexts.RemoveNode(CurNode);
			if (NeedToChangeTurnContext == true)
			{
				mCurTurnContextNode = NextNode;
			}
			++UnregisterCount;
		}

		CurNode = NextNode;
	}

	return UnregisterCount;
}

void USRPGCombatModel::FlushPendingTurnRequests()
{
	while (mPendingTurnRequests.IsEmpty() == false)
	{
		FSRPGTurnUnregisterRequest* Request = mPendingTurnRequests.Peek();

		TSharedPtr<FSRPGTurnContext>& TargetTurnContext = Request->mTargetTurnContext;
		if (TargetTurnContext != nullptr)
		{
			UnregisterTurnImmediately(TargetTurnContext.ToSharedRef());
		}
		const AUnit* TargetOwner = Request->mTargetOwner;
		if (TargetOwner != nullptr)
		{
			UnregisterTurnsImmediately(TargetOwner);
		}

		mPendingTurnRequests.Pop();
	}
}

void USRPGCombatModel::RegisterEnemyUnit(const FEnemyUnitPlacementData& EnemyPlacementData)
{
	checkf(mTileMap != nullptr, TEXT("타일맵 미존재"));

	// 적 유닛 스폰 & 초기 위치 등록
	const UStaticEnemyUnitSpawnData* EnemyUnitSpawnData = EnemyPlacementData.mSpawnData.Get();
	AUnit* EnemyUnit = GetWorld()->SpawnActorDeferred<AUnit>(EnemyUnitSpawnData->mUnitClass.Get(), mTileMap->TileToWorldTransform(EnemyPlacementData.mTransform));
	EnemyUnit->SetStaticSpawnData(EnemyUnitSpawnData);
	EnemyUnit->FinishSpawning(mTileMap->TileToWorldTransform(EnemyPlacementData.mTransform));
	EnemyUnit->OnUnitDied.AddWeakLambda(this, [this](AUnit* Unit) {
		UnregisterTurns(Unit);
		EvaluateCombatStates();
		});

	checkf(mTileMap->CanPlace(EnemyPlacementData.mTransform.mIndex, EnemyUnit), TEXT("액터 배치 불가능"));
	
	// 타일 위에 배치
	mTileMap->PlaceActor(EnemyPlacementData.mTransform, EnemyUnit);
	mUnits.Push(EnemyUnit);

	// 턴 등록
	RegisterTurn(EnemyUnit);

	// 적 AI 시작
	AEnemyAIController* AIController = EnemyUnit->GetController<AEnemyAIController>();
	AIController->StartLogic(EnemyUnitSpawnData->mStateTree.Get());
}

void USRPGCombatModel::RegisterObstacle(const FObstaclePlacementData& ObstaclePlacementData)
{
	checkf(mTileMap != nullptr, TEXT("타일맵 미존재"));

	// 적 유닛 스폰 & 초기 위치 등록
	const UStaticObstacleSpawnData* ObstacleSpawnData = ObstaclePlacementData.mSpawnData.Get();
	ITileActor* Obstacle = GetWorld()->SpawnActor<ITileActor>(ObstacleSpawnData->mClass.Get(), mTileMap->TileToWorldTransform(ObstaclePlacementData.mTransform));

	checkf(mTileMap->CanPlace(ObstaclePlacementData.mTransform.mIndex, Obstacle), TEXT("액터 배치 불가능"));

	// 타일 위에 배치
	mTileMap->PlaceActor(ObstaclePlacementData.mTransform, Obstacle);
}

void USRPGCombatModel::SpawnTileMap()
{
	checkf(mTileMap == nullptr, TEXT("이미 타일 존재"));

	UWorld* World = GetWorld();
	checkf(World != nullptr, TEXT("월드 nullptr"));

	const ARDWorldSettings* WorldSettings = Cast<ARDWorldSettings>(World->GetWorldSettings());
	checkf(WorldSettings != nullptr, TEXT("RDWorldSettings nullptr"));

	AActor* RoomStartPoint = WorldSettings->GetRoomStartPoint();
	checkf(RoomStartPoint != nullptr, TEXT("RoomStartPoint nullptr"));

	// 타일맵 스폰
	mTileMap = World->SpawnActor<ATileMap>(ATileMap::StaticClass(), RoomStartPoint->GetTransform());
}

void USRPGCombatModel::RegisterPlayerUnit(AUnit* PlayerUnit, const FTileTransform& Transform)
{
	checkf(mTileMap != nullptr, TEXT("타일맵 미존재"));
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));

	// 초기 위치 등록
	PlayerUnit->SetActorTransform(mTileMap->TileToWorldTransform(Transform));
	PlayerUnit->OnUnitDied.AddWeakLambda(this, [this](AUnit* Unit) {
		UnregisterTurns(Unit);
		EvaluateCombatStates();
		});

	// 타일 위에 배치
	mTileMap->PlaceActor(Transform, PlayerUnit);
	mPlayerUnit = PlayerUnit;
	mUnits.Push(PlayerUnit);

	// 턴 등록
	RegisterTurn(PlayerUnit);
}

void USRPGCombatModel::RegisterEnemyUnits(const TArray<FEnemyUnitPlacementData>& EnemyPlacementDatas)
{
	checkf(mTileMap != nullptr, TEXT("타일맵 미존재"));

	// 우선 순위 별로 유닛 재정렬
	TSortedMap<int32, TArray<const FEnemyUnitPlacementData*>> RegisterMap;
	for (const FEnemyUnitPlacementData& PlacementData : EnemyPlacementDatas)
	{
		RegisterMap[PlacementData.mTurnPriority].Push(&PlacementData);
	}

	// 동일 우선 순위 유닛은 랜덤 등록
	for (auto& RegisterPair : RegisterMap)
	{
		Algo::RandomShuffle(RegisterPair.Value);
		for (const FEnemyUnitPlacementData*& Data : RegisterPair.Value)
		{
			RegisterEnemyUnit(*Data);
		}
	}
}

void USRPGCombatModel::RegisterObstacles(const TArray<FObstaclePlacementData>& ObstaclePlacementDatas)
{
	checkf(mTileMap != nullptr, TEXT("타일맵 미존재"));

	for (const FObstaclePlacementData& PlacementData : ObstaclePlacementDatas)
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
		mCurTurnContextNode = mCurTurnContextNode->GetNextNode();
		// 밀려있던 턴 구성 변경 요청안들 처리
		FlushPendingTurnRequests();
	}

	// 라운드 시작 시 이벤트 처리
	NotifyRoundStartIfNeeded();

	mCurTurnContextNode->GetValue()->BeginTurn();
}

void USRPGCombatModel::NotifyRoundStartIfNeeded()
{
	if (mTurnContexts.IsEmpty() == false && mCurTurnContextNode == mTurnContexts.GetHead())
	{
		for (const TObjectPtr<AUnit>& Unit : mUnits)
		{
			Unit->OnBeginRound();
		}
		for (const TScriptInterface<ITileActor>& Obstacle : mObstacles)
		{
			Obstacle->OnBeginRound();
		}
	}
}

void USRPGCombatModel::NotifyRoundEndIfNeeded()
{
	if (mTurnContexts.IsEmpty() == false && mCurTurnContextNode == mTurnContexts.GetTail())
	{
		for (const TObjectPtr<AUnit>& Unit : mUnits)
		{
			Unit->OnEndRound();
		}
		for (const TScriptInterface<ITileActor>& Obstacle : mObstacles)
		{
			Obstacle->OnEndRound();
		}
	}
}

bool USRPGCombatModel::PushAction(TSharedPtr<FSRPGAction> Action)
{
	checkf(Action != nullptr, TEXT("유효하지 않은 액션"));

	if (mCurTurnContextNode == nullptr || mCurTurnContextNode->GetValue() == nullptr)
	{
		UE_LOG(LogSRPGCombat, Log, TEXT("현재 등록된 턴 객체가 존재하지 않음"));
		return false;
	}

	mCurTurnContextNode->GetValue()->EnqueueAction(Action);
	return true;
}

TWeakPtr<FSRPGTurnContext> USRPGCombatModel::GetCurrentTurnContext()
{
	if (mCurTurnContextNode == nullptr)
	{
		return nullptr;
	}
	return mCurTurnContextNode->GetValue();
}

TWeakPtr<FSRPGTurnContext> USRPGCombatModel::GetTurnContext(const AUnit* Owner)
{
	auto* HeadNode = mTurnContexts.GetHead();
	const int32 TurnCount = mTurnContexts.Num();
	for (int32 i = 0; i < TurnCount; ++i)
	{
		if (HeadNode->GetValue()->GetOwner() == Owner)
		{
			return HeadNode->GetValue();
		}
		HeadNode = HeadNode->GetNextNode();
	}

	checkNoEntry();
	return nullptr;
}

TArray<TWeakPtr<FSRPGTurnContext>> USRPGCombatModel::GetTurnContexts(const AUnit* Owner)
{
	TArray<TWeakPtr<FSRPGTurnContext>> Contexts;

	auto* HeadNode = mTurnContexts.GetHead();
	const int32 TurnCount = mTurnContexts.Num();
	for (int32 i = 0; i < TurnCount; ++i)
	{
		if (HeadNode->GetValue()->GetOwner() == Owner)
		{
			Contexts.Push(HeadNode->GetValue());
		}
		HeadNode = HeadNode->GetNextNode();
	}

	return Contexts;
}

ATileMap* USRPGCombatModel::GetTileMap()
{
	return mTileMap;
}

TArray<TObjectPtr<AUnit>>& USRPGCombatModel::GetUnits()
{
	return mUnits;
}
