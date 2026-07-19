#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "Simulation/Factory/ObjectModelFactory.h"

#include "SRPGFramework/SRPGCommand.h"
#include "SRPGFramework/SRPGEnemyTurnPlanner.h"
#include "SRPGFramework/SRPGMoveAction.h"
#include "SRPGFramework/SRPGSkillAction.h"
#include "SRPGFramework/SRPGTurnEndAction.h"

#include "Actor/TileMap/TileMapModel.h"
#include "Actor/TileMap/TileHighlight.h"
#include "Pawn/Enemy/EnemyUnitModel.h"

#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "AttributeSet/CombatTargetAttributeSet.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "FunctionLibrary/RandomStreamFunctionLibrary.h"

#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"
#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"
#include "DataAsset/ObstacleSpawnData/StaticObstacleSpawnData.h"

#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogSRPGCombat)

namespace
{
	constexpr int32 MaxEnemyIntentReactionsPerRound = 1;

	FText GetEnemyIntentGoalText(EMoveTendency Tendency)
	{
		switch (Tendency)
		{
		case EMoveTendency::MoveClose:
			return NSLOCTEXT("EnemyIntent", "GoalChase", "추격 · 플레이어에게 접근");
		case EMoveTendency::MoveAway:
			return NSLOCTEXT("EnemyIntent", "GoalKeepAway", "견제 · 거리를 벌려 공격");
		case EMoveTendency::HoldRange:
		default:
			return NSLOCTEXT("EnemyIntent", "GoalHoldRange", "사선 유지 · 가능한 위치에서 공격");
		}
	}

	void ApplyIntentCollisionDamage(IBoardCombatTarget* Target)
	{
		if (Target == nullptr || Target->IsTargetable() == false)
		{
			return;
		}

		if (UAttributeSetComponentModel* AttributeComponent = Target->GetAttributeComponentModel())
		{
			// 충돌은 공격력 계수와 무관한 작은 고정 피해다. 핵심 결과는 경로 중단과 위치 관계다.
			AttributeComponent->ApplyModToAttribute(
				UCombatTargetAttributeSet::GetHPAttribute(),
				ETacticalModOp::Additive,
				-1.0f);
		}
	}
}

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
		// 시뮬레이션 복제본에서도 현재 턴을 알 수 있게 현재 턴 id를 저장한다.
		int32 CurrentTurnId = (mCurTurnContextOrder != nullptr) ? mCurTurnContextOrder->GetValue() : INDEX_NONE;
		Ar << CurrentTurnId;
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
		// 저장한 id로 현재 턴 노드를 다시 찾는다.
		int32 CurrentTurnId = INDEX_NONE;
		Ar << OUT CurrentTurnId;
		mCurTurnContextOrder = (CurrentTurnId != INDEX_NONE) ? mTurnContextOrder.FindNode(CurrentTurnId) : nullptr;
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

void USRPGCombatModel::InitCombat(UStaticCombatRoomSpawnData* RoomSpawnData, UUnitModel* PlayerUnit, const FTransform& RoomStartTransform)
{
	checkf(RoomSpawnData != nullptr, TEXT("해당하는 룸 정보 탐색 실패"));
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));

	checkf(mCombatPhase == ESRPGCombatRoomPhase::None, TEXT("중복 초기화"));
	mCombatPhase = ESRPGCombatRoomPhase::CombatInit;
	mEnemyIntents.Reset();

	SpawnTileMap(RoomStartTransform);
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
		// 전투 종료 알림
		for (TObjectPtr<UUnitModel>& Unit : mUnits)
		{
			Unit->OnBeginRoom();
		}
		for (TObjectPtr<UBoardActorModel>& Obstacle : mObstacles)
		{
			Obstacle->OnBeginRoom();
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

	// 전투 종료 알림
	for (TObjectPtr<UUnitModel>& Unit : mUnits)
	{
		Unit->OnEndRoom();
	}
	for (TObjectPtr<UBoardActorModel>& Obstacle : mObstacles)
	{
		Obstacle->OnEndRoom();
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
	ClearDeadActors();
	EvaluateCombatEndState();
	if (mCurTurnContextOrder != nullptr)
	{
		const bool ForceAbort = mCombatPhase == ESRPGCombatRoomPhase::CombatAbort;
		mTurnContextMap[mCurTurnContextOrder->GetValue()]->EvaluateTurnEndState(ForceAbort);
	}
}

void USRPGCombatModel::OnEndCurrentTurn(USRPGTurnContext* TurnContext, ESRPGTurnResult TurnResult)
{
	if (TurnContext != nullptr && TurnContext->GetOwner() != nullptr && TurnContext->GetOwner()->IsPlayerUnitModel() == false)
	{
		CompleteEnemyIntent(TurnContext->GetOwner());
	}

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

void USRPGCombatModel::ClearDeadActors()
{
	checkf(mPlayerUnit != nullptr, TEXT("플레이어 미동록 오류"));

	TArray<TObjectPtr<UUnitModel>> DeadUnits;
	TArray<TScriptInterface<IBoardCombatTarget>> DeadObstacles;

	for (const TObjectPtr<UUnitModel>& Unit : mUnits)
	{
		if (Unit->IsDead() == true)
		{
			DeadUnits.Add(Unit);
		}
	}
	for (const TScriptInterface<IBoardCombatTarget>& Obstacle : mCombatTargetObstacles)
	{
		if (Obstacle->IsDead() == true)
		{
			DeadObstacles.Add(Obstacle);
		}
	}

	for (const TObjectPtr<UUnitModel>& DeadUnit : DeadUnits)
	{
		UnregisterTurns(DeadUnit);
	}
	for (const TScriptInterface<IBoardCombatTarget>& DeadObstacle : DeadObstacles)
	{
		UnregisterObstacle(Cast<UBoardActorModel>(DeadObstacle.GetObject()));
	}
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

	if (AnyEnemyAlive == false)
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
	TurnContext->OnShowDicePanelAtTurnStartUI.AddWeakLambda(this, [this](const USRPGTurnContext* TurnContext) {
		OnShowDicePanelAnyTurnUI.Broadcast(TurnContext);
		});
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

		TArray<TObjectPtr<USRPGTurnContext>> ActiveTurns = GetTurnContexts(TurnContext->GetOwner());
		if (ActiveTurns.Num() == 0)
		{
			UnregisterUnit(TurnContext->GetOwner());
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

			mTurnContextMap.Remove(CurNode->GetValue());
			NextNode = mTurnContextOrder.RemoveNode(CurNode);
			if (NeedToChangeTurnContext == true)
			{
				mCurTurnContextOrder = NextNode;
			}
			++UnregisterCount;
		}

		CurNode = NextNode;
	}

	if (UnregisterCount > 0)
	{
		UnregisterUnit(Owner);
	}

	return UnregisterCount;
}

void USRPGCombatModel::FlushPendingTurnRequests()
{
	while (mPendingTurnRequests.Num() > mHeadRequestIndex)
	{
		FSRPGTurnUnregisterRequest& Request = mPendingTurnRequests[mHeadRequestIndex];

		USRPGTurnContext* TargetTurnContext = Request.mTargetTurnContext;
		UUnitModel* TargetOwner = Request.mTargetOwner;
		if (TargetTurnContext != nullptr)
		{
			UnregisterTurnImmediately(TargetTurnContext);
		}
		else if (TargetOwner != nullptr)
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
	UEnemyUnitModel* EnemyUnit = GetWorldModelFactory(this)->NewModelDeferred<UEnemyUnitModel>(EnemyModelClass);
	EnemyUnit->SetStaticSpawnData(EnemyUnitSpawnData);
	EnemyUnit->SetDifficulty(EnemyPlacementData.mDifficulty);
	EnemyUnit->FinishCreating(mTileMap->TileToWorldTransform(EnemyPlacementData.mTransform));

	RegisterUnit(EnemyUnit, EnemyPlacementData.mTransform);
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

	mObstacles.Push(Obstacle);
	if (ObstacleModelClass->ImplementsInterface(UBoardCombatTarget::StaticClass()) == true)
	{
		mCombatTargetObstacles.Push(Obstacle);
	}

	// 타일 위에 배치
	mTileMap->PlaceActor(ObstaclePlacementData.mTransform, Obstacle);

	if (mCombatPhase == ESRPGCombatRoomPhase::CombatPlay)
	{
		Obstacle->OnBeginRoom();
	}

	OnRegisterObstacleUI.Broadcast(Obstacle);
}

void USRPGCombatModel::UnregisterUnit(UUnitModel* Unit)
{
	checkf(mTileMap != nullptr, TEXT("타일맵 미존재"));

	if (Unit != nullptr && Unit->IsPlayerUnitModel() == false)
	{
		if (FSRPGEnemyIntent* Intent = FindEnemyIntent(Unit))
		{
			if (Intent->mResult == ESRPGEnemyIntentResult::Planned || Intent->mResult == ESRPGEnemyIntentResult::Executing)
			{
				AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::Cancelled,
					NSLOCTEXT("EnemyIntent", "DefeatedBeforeIntent", "행동 전에 쓰러져 예정 행동 취소"));
				BroadcastEnemyIntentChanged();
				RefreshEnemyIntentHighlights();
			}
		}
	}

	if (mCombatPhase == ESRPGCombatRoomPhase::CombatPlay)
	{
		Unit->OnEndRoom();
	}

	// 타일 위에서 제거
	mTileMap->RemoveActor(Unit);

	mUnits.RemoveSingleSwap(Unit);

	if (Unit->IsPlayerUnitModel() == false)
	{
		FTimerHandle Handle;

		GetWorld()->GetTimerManager().SetTimer(OUT Handle, FTimerDelegate::CreateWeakLambda(Unit, [Unit]() {
			Unit->Destroy();
			}), 3.f, false);
	}

	OnUnregisterUnitUI.Broadcast(Unit);
}

void USRPGCombatModel::UnregisterObstacle(UBoardActorModel* Obstacle)
{
	checkf(mTileMap != nullptr, TEXT("타일맵 미존재"));

	if (mCombatPhase == ESRPGCombatRoomPhase::CombatPlay)
	{
		Obstacle->OnEndRoom();
	}

	// 타일 위에서 제거
	mTileMap->RemoveActor(Obstacle);

	mObstacles.RemoveSingleSwap(Obstacle);
	if (Obstacle->GetClass()->ImplementsInterface(UBoardCombatTarget::StaticClass()) == true)
	{
		mCombatTargetObstacles.RemoveSingleSwap(Obstacle);
	}

	OnUnregisterObstacleUI.Broadcast(Obstacle);
}

void USRPGCombatModel::SpawnTileMap(const FTransform& RoomStartTransform)
{
	checkf(mTileMap == nullptr, TEXT("이미 타일 존재"));

	// 타일맵 스폰
	mTileMap = GetWorldModelFactory(this)->NewModel<UTileMapModel>(RoomStartTransform);

	// 모델이 자기 타일 저장소(mTiles)를 직접 빌드한다.
	// 기존엔 View(ATileMap)만 RebuildTiles를 호출해서, View 스폰/타이밍에 따라 모델 타일이 비어 있었고
	// 이어지는 유닛 배치(PlaceActor→CanPlace)가 빈 타일맵에서 크래시했다. 배치 전에 모델이 직접 빌드한다.
	mTileMap->RebuildTiles();
}

void USRPGCombatModel::RegisterPlayerUnit(UUnitModel* PlayerUnit, const FTileTransform& Transform)
{
	checkf(mTileMap != nullptr, TEXT("타일맵 미존재"));
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));

	mPlayerUnit = PlayerUnit;
	RegisterUnit(PlayerUnit, Transform);
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

void USRPGCombatModel::RegisterUnit(UUnitModel* Unit, const FTileTransform& Transform)
{
	mUnits.Push(Unit);

	// 타일 위에 배치
	checkf(mTileMap->CanPlace(Transform.mIndex, Unit), TEXT("액터 배치 불가능"));
	mTileMap->PlaceActor(Transform, Unit);

	// 턴 등록
	RegisterTurn(Unit);

	if (mCombatPhase == ESRPGCombatRoomPhase::CombatPlay)
	{
		Unit->OnBeginRoom();
	}

	OnRegisterUnitUI.Broadcast(Unit);
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
		// 밀려있던 턴 구성 변경 요청안들 처리
		FlushPendingTurnRequests();
		
		// 라운드 종료 시 이벤트 처리
		NotifyRoundEndIfNeeded();

		// 턴 변경
		mCurTurnContextOrder = mCurTurnContextOrder->GetNextNode();
	}

	// 강제 중단
	if (mShouldTerminateBeforePlayerTurnStart == true && mTurnContextMap[mCurTurnContextOrder->GetValue()]->GetOwner()->IsPlayerUnitModel() == true)
	{
		mShouldTerminateBeforePlayerTurnStart = false;
		return;
	}

	auto PresentationBarrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateWeakLambda(this, [this]() {
		// 다음 턴 시작
		mTurnContextMap[mCurTurnContextOrder->GetValue()]->BeginTurn();
		}));

	// 라운드 시작 시 이벤트 처리
	NotifyRoundStartIfNeeded(PresentationBarrier);
}

void USRPGCombatModel::NotifyRoundStartIfNeeded(TSharedPtr<FPresentationBarrier> RoundPresentationBarrier)
{
	if (mTurnContextOrder.IsEmpty() == false && mCurTurnContextOrder == mTurnContextOrder.GetHead())
	{
		++mRoundCount;

		for (const TObjectPtr<UUnitModel>& Unit : mUnits)
		{
			Unit->OnBeginRound();
		}
		for (const TObjectPtr<UBoardActorModel>& Obstacle : mObstacles)
		{
			Obstacle->OnBeginRound();
		}

		// 플레이어가 주사위를 고르기 전에 목표와 현재 경로를 공개한다. 강제 이동을 받은 적은
		// 같은 목표/스킬을 유지한 채 라운드당 한 번만 경로를 갱신할 수 있다.
		PrepareEnemyIntents();

		OnBeginAnyRoundUI.Broadcast(RoundPresentationBarrier, mRoundCount);
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

USRPGTurnContext* USRPGCombatModel::GetCurrentTurnContext() const
{
	if (mCurTurnContextOrder == nullptr)
	{
		return nullptr;
	}
	return mTurnContextMap[mCurTurnContextOrder->GetValue()];
}

USRPGTurnContext* USRPGCombatModel::GetTurnContext(const UUnitModel* Owner) const
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

TArray<TObjectPtr<USRPGTurnContext>> USRPGCombatModel::GetTurnContexts(const UUnitModel* Owner) const
{
	TArray<TObjectPtr<USRPGTurnContext>> Contexts;

	auto* CurNode = mTurnContextOrder.GetHead();
	const int32 TurnCount = mTurnContextOrder.Num();
	for (int32 i = 0; i < TurnCount; ++i)
	{
		if (mTurnContextMap[CurNode->GetValue()]->GetOwner() == Owner)
		{
			Contexts.Push(mTurnContextMap[CurNode->GetValue()]);
		}
		CurNode = CurNode->GetNextNode();
	}

	return Contexts;
}

TArray<TObjectPtr<USRPGTurnContext>> USRPGCombatModel::GetOrderedTurnContexts() const
{
	TArray<TObjectPtr<USRPGTurnContext>> Contexts;

	auto* CurNode = mCurTurnContextOrder;
	const int32 TurnCount = mTurnContextOrder.Num();
	for (int32 i = 0; i < TurnCount; ++i)
	{
		Contexts.Push(mTurnContextMap[CurNode->GetValue()]);
		CurNode = CurNode->GetNextNode();
	}

	return Contexts;
}

UTileMapModel* USRPGCombatModel::GetTileMap() const
{
	return mTileMap;
}

UUnitModel* USRPGCombatModel::GetPlayerUnit() const
{
	return mPlayerUnit;
}

const TArray<TObjectPtr<UUnitModel>>& USRPGCombatModel::GetUnits() const
{
	return mUnits;
}

const TArray<TObjectPtr<UBoardActorModel>>& USRPGCombatModel::GetObstacles() const
{
	return mObstacles;
}

int32 USRPGCombatModel::GetRoundCount() const
{
	return mRoundCount;
}

const TArray<FSRPGEnemyIntent>& USRPGCombatModel::GetEnemyIntents() const
{
	return mEnemyIntents;
}

bool USRPGCombatModel::RebuildEnemyIntentPlan(FSRPGEnemyIntent& Intent, bool bPreserveSkill)
{
	UEnemyUnitModel* Enemy = Intent.mEnemy.Get();
	if (Enemy == nullptr || Enemy->IsDead() || mTileMap == nullptr || mPlayerUnit == nullptr || mPlayerUnit->IsDead())
	{
		return false;
	}

	int32 PlannedSkillIndex = INDEX_NONE;
	TArray<TInstancedStruct<FSRPGCommand>> Commands = USRPGEnemyTurnPlanner::PlanTurn(
		Enemy,
		mPlayerUnit,
		mTileMap,
		URandomStreamFunctionLibrary::GetEventStream(this),
		Intent.mPlannedMoveRange,
		bPreserveSkill ? Intent.mSkillIndex : INDEX_NONE,
		&PlannedSkillIndex);

	Intent.mSkillIndex = PlannedSkillIndex;
	Intent.mSkillName = FText::GetEmpty();
	Intent.mPlannedOrigin = Enemy->GetTileTransform().mIndex;
	Intent.mPlannedDestination = Intent.mPlannedOrigin;
	Intent.mTargetTile = FTileIndex::Invalid;
	Intent.mPathTileIndexes.Reset();
	Intent.mEffectTileIndexes.Reset();

	for (TInstancedStruct<FSRPGCommand>& Command : Commands)
	{
		switch (Command.Get().GetCommandType())
		{
		case ESRPGCommandType::MoveCast:
		{
			FSRPGMoveCommand& MoveCommand = Command.GetMutable<FSRPGMoveCommand>();
			MoveCommand.mUseFixedIntent = true;
			Intent.mPathTileIndexes = MoveCommand.mPathTileIndexes;
			if (Intent.mPathTileIndexes.IsEmpty() == false)
			{
				Intent.mPlannedDestination = Intent.mPathTileIndexes.Last();
			}
			break;
		}
		case ESRPGCommandType::SkillCast:
		{
			FSRPGSkillCastCommand& SkillCommand = Command.GetMutable<FSRPGSkillCastCommand>();
			SkillCommand.mUseFixedIntent = true;
			SkillCommand.mAllowFriendlyFire = true;
			Intent.mTargetTile = SkillCommand.mTargetIndex;
			Intent.mEffectTileIndexes = SkillCommand.mFixedEffectTileIndexes;
			break;
		}
		default:
			break;
		}
	}

	if (Intent.mPathTileIndexes.IsEmpty())
	{
		Intent.mPathTileIndexes.Add(Intent.mPlannedOrigin);
	}
	if (Intent.mTargetTile != FTileIndex::Invalid)
	{
		if (USkillComponentModel* SkillComponent = Enemy->GetSkillComponentModel())
		{
			const FSkillEntry* SkillEntry = SkillComponent->GetSkill(Intent.mSkillIndex);
			if (SkillEntry != nullptr && SkillEntry->mData != nullptr)
			{
				Intent.mSkillName = SkillEntry->mData->mName.IsEmpty()
					? FText::FromName(SkillEntry->mData->GetFName())
					: SkillEntry->mData->mName;
			}
		}
	}
	if (Intent.mSkillName.IsEmpty())
	{
		Intent.mSkillName = Intent.mPlannedDestination != Intent.mPlannedOrigin
			? NSLOCTEXT("EnemyIntent", "MoveOnly", "이동")
			: NSLOCTEXT("EnemyIntent", "Wait", "대기");
	}

	if (TObjectPtr<USRPGTurnContext>* TurnContext = mTurnContextMap.Find(Intent.mTurnId);
		TurnContext != nullptr && IsValid(TurnContext->Get()))
	{
		TurnContext->Get()->SetFixedEnemyPlan(MoveTemp(Commands));
		return true;
	}
	return false;
}

void USRPGCombatModel::PrepareEnemyIntents()
{
	mEnemyIntents.Reset();

	if (mTileMap == nullptr || mPlayerUnit == nullptr || mPlayerUnit->IsDead())
	{
		BroadcastEnemyIntentChanged();
		RefreshEnemyIntentHighlights();
		return;
	}

	int32 ExecutionOrder = 1;
	for (USRPGTurnContext* TurnContext : GetOrderedTurnContexts())
	{
		if (TurnContext == nullptr)
		{
			continue;
		}

		UEnemyUnitModel* Enemy = Cast<UEnemyUnitModel>(TurnContext->GetOwner());
		if (Enemy == nullptr || Enemy->IsDead())
		{
			continue;
		}

		UAttributeSetComponentModel* Attributes = Enemy->GetAttributeComponentModel();
		const int32 PlannedMoveRange = Attributes != nullptr
			? FMath::Max(FMath::RoundToInt(Attributes->GetAttributeCurrentValue(UEnemyUnitAttributeSet::GetRechargeMovementAttribute())), 0)
			: 0;

		FSRPGEnemyIntent Intent;
		Intent.mTurnId = TurnContext->GetTurnId();
		Intent.mExecutionOrder = ExecutionOrder++;
		Intent.mEnemy = Enemy;
		Intent.mGoalText = GetEnemyIntentGoalText(Enemy->GetMoveTendency());
		Intent.mPlannedMoveRange = PlannedMoveRange;
		Intent.mResultText = NSLOCTEXT(
			"EnemyIntent",
			"PlanTelegraphed",
			"목표와 현재 경로 공개 · 강제 이동을 받으면 같은 목표로 한 번 대응");
		RebuildEnemyIntentPlan(Intent, /*bPreserveSkill*/false);
		mEnemyIntents.Add(MoveTemp(Intent));
	}

	// 첫 안내가 말뿐인 예시가 되지 않도록, 현재 플레이어 위치에서 실제로 한 칸 이상
	// 밀 수 있고 공격 없이 이동만 예정한 적만 연습 대상으로 고른다. 모든 주사위 눈은
	// 최소 1칸을 밀기 때문에 이 표식이 붙은 적은 어떤 굴림 조합으로도 위치 개입이 성립한다.
	const FTileIndex PlayerTile = mPlayerUnit->GetTileTransform().mIndex;
	auto CanBePushedFromPlayer = [this, PlayerTile](const FSRPGEnemyIntent& Intent)
	{
		if (Intent.mEnemy == nullptr || Intent.mEnemy->IsDead())
		{
			return false;
		}
		const FTileIndex EnemyTile = Intent.mEnemy->GetTileTransform().mIndex;
		return mTileMap->GetPushDestination(PlayerTile, EnemyTile, 1) != EnemyTile;
	};

	FSRPGEnemyIntent* RecommendedIntent = mEnemyIntents.FindByPredicate(
		[&CanBePushedFromPlayer](const FSRPGEnemyIntent& Intent)
		{
			const bool bMoveOnly = Intent.mPlannedDestination != Intent.mPlannedOrigin
				&& Intent.mEffectTileIndexes.IsEmpty();
			return bMoveOnly && CanBePushedFromPlayer(Intent);
		});
	if (RecommendedIntent != nullptr)
	{
		RecommendedIntent->mIsRecommendedInterventionTarget = true;
	}

	BroadcastEnemyIntentChanged();
	RefreshEnemyIntentHighlights();
}

FSRPGEnemyIntent* USRPGCombatModel::FindEnemyIntent(UUnitModel* Enemy)
{
	if (const USRPGTurnContext* CurrentTurn = GetCurrentTurnContext();
		CurrentTurn != nullptr && CurrentTurn->GetOwner() == Enemy)
	{
		if (FSRPGEnemyIntent* CurrentIntent = mEnemyIntents.FindByPredicate([Enemy, CurrentTurn](const FSRPGEnemyIntent& Intent)
			{
				return Intent.mEnemy == Enemy && Intent.mTurnId == CurrentTurn->GetTurnId();
			}))
		{
			return CurrentIntent;
		}
	}

	if (FSRPGEnemyIntent* ExecutingIntent = mEnemyIntents.FindByPredicate([Enemy](const FSRPGEnemyIntent& Intent)
		{
			return Intent.mEnemy == Enemy && Intent.mResult == ESRPGEnemyIntentResult::Executing;
		}))
	{
		return ExecutingIntent;
	}

	if (FSRPGEnemyIntent* PlannedIntent = mEnemyIntents.FindByPredicate([Enemy](const FSRPGEnemyIntent& Intent)
		{
			return Intent.mEnemy == Enemy && Intent.mResult == ESRPGEnemyIntentResult::Planned;
		}))
	{
		return PlannedIntent;
	}

	return mEnemyIntents.FindByPredicate([Enemy](const FSRPGEnemyIntent& Intent)
	{
		return Intent.mEnemy == Enemy;
	});
}

const FSRPGEnemyIntent* USRPGCombatModel::FindEnemyIntent(const UUnitModel* Enemy) const
{
	if (const USRPGTurnContext* CurrentTurn = GetCurrentTurnContext();
		CurrentTurn != nullptr && CurrentTurn->GetOwner() == Enemy)
	{
		if (const FSRPGEnemyIntent* CurrentIntent = mEnemyIntents.FindByPredicate([Enemy, CurrentTurn](const FSRPGEnemyIntent& Intent)
			{
				return Intent.mEnemy == Enemy && Intent.mTurnId == CurrentTurn->GetTurnId();
			}))
		{
			return CurrentIntent;
		}
	}

	return mEnemyIntents.FindByPredicate([Enemy](const FSRPGEnemyIntent& Intent)
	{
		return Intent.mEnemy == Enemy;
	});
}

void USRPGCombatModel::AppendEnemyIntentResult(FSRPGEnemyIntent& Intent, ESRPGEnemyIntentResult Result, const FText& Message)
{
	Intent.mResult = Result;
	if (Message.IsEmpty())
	{
		return;
	}

	Intent.mResultText = Intent.mResultText.IsEmpty()
		? Message
		: FText::Format(NSLOCTEXT("EnemyIntent", "ResultJoin", "{0}\n{1}"), Intent.mResultText, Message);
}

void USRPGCombatModel::BroadcastEnemyIntentChanged()
{
	OnEnemyIntentsChangedUI.Broadcast();
}

void USRPGCombatModel::MarkEnemyIntentExecuting(UUnitModel* Enemy, int32 TurnId)
{
	FSRPGEnemyIntent* Intent = TurnId != INDEX_NONE
		? mEnemyIntents.FindByPredicate([Enemy, TurnId](const FSRPGEnemyIntent& Candidate)
			{
				return Candidate.mEnemy == Enemy && Candidate.mTurnId == TurnId;
			})
		: FindEnemyIntent(Enemy);
	if (Intent != nullptr)
	{
		AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::Executing,
			NSLOCTEXT("EnemyIntent", "ExecutingTelegraphedPlan", "마지막으로 공개한 경로와 공격을 실행 중"));
		BroadcastEnemyIntentChanged();
		RefreshEnemyIntentHighlights();
	}
}

void USRPGCombatModel::CompleteEnemyIntent(UUnitModel* Enemy)
{
	if (FSRPGEnemyIntent* Intent = FindEnemyIntent(Enemy))
	{
		if (Intent->mResult == ESRPGEnemyIntentResult::Planned || Intent->mResult == ESRPGEnemyIntentResult::Executing)
		{
			AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::Completed,
				NSLOCTEXT("EnemyIntent", "CompletedAsPlanned", "예정 행동 종료"));
		}
		BroadcastEnemyIntentChanged();
		RefreshEnemyIntentHighlights();
	}
}

void USRPGCombatModel::ReportPlayerDisplacement(
	UUnitModel* Target,
	const FTileIndex& From,
	const FTileIndex& To,
	int32 DiceValue,
	ESRPGPlayerDisplacementType DisplacementType)
{
	if (Target == nullptr || From == To)
	{
		return;
	}

	const bool bWasPulled = DisplacementType == ESRPGPlayerDisplacementType::Pull;
	bool bChanged = false;
	for (FSRPGEnemyIntent& Intent : mEnemyIntents)
	{
		if (Intent.mEnemy != Target
			|| (Intent.mResult != ESRPGEnemyIntentResult::Planned && Intent.mResult != ESRPGEnemyIntentResult::Executing))
		{
			continue;
		}

		Intent.mWasDisplaced = true;
		Intent.mDisplacedToTile = To;

		const bool bCanReact = Intent.mReactionCount < MaxEnemyIntentReactionsPerRound;
		if (bCanReact)
		{
			++Intent.mReactionCount;
			RebuildEnemyIntentPlan(Intent, /*bPreserveSkill*/true);
		}
		else
		{
			// 한 차례 대응을 마친 적은 더 이상 완벽히 재계산하지 않는다. 마지막으로 공개한
			// 방향열만 새 출발점으로 옮겨 플레이어의 두 번째 개입이 확실한 보상이 되게 한다.
			const FTileIndex Delta(To.mX - From.mX, To.mY - From.mY);
			if (TObjectPtr<USRPGTurnContext>* TurnContext = mTurnContextMap.Find(Intent.mTurnId);
				TurnContext != nullptr && IsValid(TurnContext->Get()))
			{
				TurnContext->Get()->TranslateFixedEnemyMovementPlan(Delta);
			}
			auto TranslateTile = [&Delta](FTileIndex& TileIndex)
			{
				if (TileIndex != FTileIndex::Invalid)
				{
					TileIndex = FTileIndex(TileIndex.mX + Delta.mX, TileIndex.mY + Delta.mY);
				}
			};
			TranslateTile(Intent.mPlannedOrigin);
			TranslateTile(Intent.mPlannedDestination);
			for (FTileIndex& TileIndex : Intent.mPathTileIndexes)
			{
				TranslateTile(TileIndex);
			}
		}

		const FText Message = FText::Format(
			bCanReact
				? NSLOCTEXT("EnemyIntent", "PlayerDisplacedEnemyReacted", "{0}: 주사위 {1} · ({2},{3}) → ({4},{5}) · 목표 유지, 새 경로 계산 (대응 소진)")
				: NSLOCTEXT("EnemyIntent", "PlayerDisplacedEnemyCommitted", "{0}: 주사위 {1} · ({2},{3}) → ({4},{5}) · 대응 소진, 마지막 계획 유지"),
			bWasPulled ? NSLOCTEXT("EnemyIntent", "PulledLabel", "끌어오기") : NSLOCTEXT("EnemyIntent", "PushedLabel", "밀기"),
			FText::AsNumber(DiceValue),
			FText::AsNumber(From.mX),
			FText::AsNumber(From.mY),
			FText::AsNumber(To.mX),
			FText::AsNumber(To.mY));
		Intent.mResultText = FText::Format(NSLOCTEXT("EnemyIntent", "ResultJoin", "{0}\n{1}"), Intent.mResultText, Message);
		bChanged = true;
	}

	// 강제로 옮긴 적이 다른 적의 공개 경로를 실제로 막았다면 그 적도 자신의 1회 대응을 사용해
	// 우회한다. 관계없는 적까지 매번 전부 재계산하지 않아 조작의 결과와 예측 가능성을 보존한다.
	for (FSRPGEnemyIntent& Intent : mEnemyIntents)
	{
		if (Intent.mEnemy == Target
			|| Intent.mEnemy == nullptr
			|| Intent.mEnemy->IsDead()
			|| Intent.mReactionCount >= MaxEnemyIntentReactionsPerRound
			|| (Intent.mResult != ESRPGEnemyIntentResult::Planned && Intent.mResult != ESRPGEnemyIntentResult::Executing))
		{
			continue;
		}
		const bool bRouteBlocked = Intent.mPathTileIndexes.Num() > 1
			&& Intent.mPathTileIndexes.Contains(To)
			&& Intent.mEnemy->GetTileTransform().mIndex != To;
		if (bRouteBlocked == false)
		{
			continue;
		}

		++Intent.mReactionCount;
		if (RebuildEnemyIntentPlan(Intent, /*bPreserveSkill*/true))
		{
			Intent.mResultText = FText::Format(
				NSLOCTEXT("EnemyIntent", "ResultJoin", "{0}\n{1}"),
				Intent.mResultText,
				NSLOCTEXT("EnemyIntent", "BlockedRouteReacted", "다른 적이 진로를 막음 · 목표 유지, 우회 경로 계산 (대응 소진)"));
			bChanged = true;
		}
	}
	if (bChanged)
	{
		BroadcastEnemyIntentChanged();
		RefreshEnemyIntentHighlights();
	}
}

void USRPGCombatModel::ReportPlayerDisplacementCollision(
	UUnitModel* Target,
	UBoardActorModel* Blocker,
	ESRPGPlayerDisplacementType DisplacementType)
{
	if (Target == nullptr || Blocker == nullptr)
	{
		return;
	}

	ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(Target));
	ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(Blocker));
	const bool bEnemyBlocker = Cast<UUnitModel>(Blocker) != nullptr && Blocker != mPlayerUnit;
	if (FSRPGEnemyIntent* Intent = FindEnemyIntent(Target))
	{
		const FText Message = bEnemyBlocker
			? FText::Format(
				NSLOCTEXT("EnemyIntent", "PlayerChainCollision", "{0} 충돌! {1}과 부딪혀 양쪽 1 피해"),
				DisplacementType == ESRPGPlayerDisplacementType::Pull
					? NSLOCTEXT("EnemyIntent", "PullImpact", "당기기")
					: NSLOCTEXT("EnemyIntent", "PushImpact", "밀기"),
				Blocker->GetBoardActorDisplayName())
			: FText::Format(
				NSLOCTEXT("EnemyIntent", "PlayerObstacleCollision", "{0} 충돌! {1} 앞에서 정지"),
				DisplacementType == ESRPGPlayerDisplacementType::Pull
					? NSLOCTEXT("EnemyIntent", "PullImpact", "당기기")
					: NSLOCTEXT("EnemyIntent", "PushImpact", "밀기"),
				Blocker->GetBoardActorDisplayName());
		Intent->mResultText = FText::Format(
			NSLOCTEXT("EnemyIntent", "ResultJoin", "{0}\n{1}"),
			Intent->mResultText,
			Message);
		BroadcastEnemyIntentChanged();
		RefreshEnemyIntentHighlights();
	}
}

void USRPGCombatModel::ReportFixedIntentPathDisrupted(UUnitModel* Enemy, const FText& Reason)
{
	if (FSRPGEnemyIntent* Intent = FindEnemyIntent(Enemy))
	{
		AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::Cancelled, Reason);
		BroadcastEnemyIntentChanged();
		RefreshEnemyIntentHighlights();
	}
}

void USRPGCombatModel::ResolveFixedIntentCollision(UUnitModel* Enemy, UBoardActorModel* Blocker)
{
	if (Enemy == nullptr)
	{
		return;
	}

	ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(Enemy));
	ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(Blocker));

	if (FSRPGEnemyIntent* Intent = FindEnemyIntent(Enemy))
	{
		const bool IsEnemyBlocker = Blocker != nullptr
			&& Cast<UUnitModel>(Blocker) != nullptr
			&& Blocker != mPlayerUnit;
		const FText Message = IsEnemyBlocker
			? FText::Format(NSLOCTEXT("EnemyIntent", "EnemyCollision", "적끼리 충돌: {0}에 막혀 이동 취소 (양쪽 1 피해)"), Blocker->GetBoardActorDisplayName())
			: FText::Format(NSLOCTEXT("EnemyIntent", "ObstacleCollision", "예정 경로 충돌: {0}에 막혀 이동 취소"),
				Blocker != nullptr ? Blocker->GetBoardActorDisplayName() : NSLOCTEXT("EnemyIntent", "UnknownBlocker", "장애물"));
		AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::Collision, Message);
		BroadcastEnemyIntentChanged();
		RefreshEnemyIntentHighlights();
	}
}

void USRPGCombatModel::ResolveFixedIntentAttack(UUnitModel* Enemy, const TArray<IBoardCombatTarget*>& ResolvedTargets)
{
	FSRPGEnemyIntent* Intent = FindEnemyIntent(Enemy);
	if (Intent == nullptr)
	{
		return;
	}

	bool HitPlayer = false;
	bool HitFriendly = false;
	bool HitObstacle = false;
	for (IBoardCombatTarget* Target : ResolvedTargets)
	{
		UBoardActorModel* TargetActor = Cast<UBoardActorModel>(Target);
		if (TargetActor == nullptr)
		{
			continue;
		}

		if (TargetActor == mPlayerUnit)
		{
			HitPlayer = true;
		}
		else if (Cast<UUnitModel>(TargetActor) != nullptr)
		{
			HitFriendly = true;
		}
		else
		{
			HitObstacle = true;
		}
	}

	if (HitPlayer)
	{
		AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::HitPlayer,
			NSLOCTEXT("EnemyIntent", "HitPlayer", "예정 좌표에 플레이어가 남아 공격 적중"));
	}
	if (HitObstacle)
	{
		AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::HitObstacle,
			NSLOCTEXT("EnemyIntent", "HitObstacle", "공개된 공격선이 장애물에 적중"));
	}
	if (HitFriendly)
	{
		// 복합 명중이어도 HUD의 대표 배지는 가장 극적인 오사 결과를 유지한다.
		AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::FriendlyFire,
			NSLOCTEXT("EnemyIntent", "FriendlyFire", "아군 오사! 공개된 공격선에 다른 적이 맞음"));
	}
	if (HitPlayer == false && HitFriendly == false && HitObstacle == false)
	{
		AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::Missed,
			NSLOCTEXT("EnemyIntent", "AttackMissed", "공격 빗나감! 예정 타일이 비어 있음"));
	}

	BroadcastEnemyIntentChanged();
	RefreshEnemyIntentHighlights();
}

void USRPGCombatModel::RefreshEnemyIntentHighlights()
{
	if (mTileMap == nullptr)
	{
		return;
	}

	mTileMap->ClearTileHighlight(ETileHighlightFlag::Aim);
	mTileMap->ClearTileHighlight(ETileHighlightFlag::Effect);
	mTileMap->ClearTileHighlight(ETileHighlightFlag::Select);

	TArray<FTileIndex> TargetTiles;
	TArray<FEnemyIntentTileOverlay> EnemyIntentOverlays;
	EnemyIntentOverlays.Reserve(mEnemyIntents.Num());
	for (const FSRPGEnemyIntent& Intent : mEnemyIntents)
	{
		const bool bIsResolved = Intent.mResult != ESRPGEnemyIntentResult::Planned
			&& Intent.mResult != ESRPGEnemyIntentResult::Executing;

		FEnemyIntentTileOverlay& Overlay = EnemyIntentOverlays.AddDefaulted_GetRef();
		Overlay.mExecutionOrder = Intent.mExecutionOrder;
		Overlay.mPathTileIndexes = Intent.mPathTileIndexes;
		Overlay.mEffectTileIndexes = Intent.mEffectTileIndexes;
		Overlay.mTargetTile = Intent.mTargetTile;
		Overlay.mPlannedOrigin = Intent.mPlannedOrigin;
		Overlay.mCurrentTile = IsValid(Intent.mEnemy)
			? Intent.mEnemy->GetTileTransform().mIndex
			: FTileIndex::Invalid;
		if (Overlay.mCurrentTile == FTileIndex::Invalid)
		{
			Overlay.mCurrentTile = Intent.mDisplacedToTile != FTileIndex::Invalid
				? Intent.mDisplacedToTile
				: Intent.mPlannedOrigin;
		}
		Overlay.mWasDisplaced = Intent.mWasDisplaced;
		Overlay.mIsResolved = bIsResolved;

		if (bIsResolved)
		{
			continue;
		}

		// 경로/공격은 적별 색을 보존하는 전용 오버레이가 담당한다. 기존 Aim/Effect 합집합을
		// 함께 칠하면 모든 적이 같은 흰 타일처럼 보여 경로 소유자와 이동→공격 순서를 읽기 어렵다.
		if (Intent.mIsRecommendedInterventionTarget && Intent.mEnemy != nullptr)
		{
			TargetTiles.AddUnique(Intent.mEnemy->GetTileTransform().mIndex);
		}
	}

	// 전용 오버레이는 해결 상태도 유지한다. 빈 배열도 전달해 새 계획이 없을 때 이전 표시를 지운다.
	mTileMap->SetEnemyIntentOverlays(EnemyIntentOverlays);

	if (TargetTiles.IsEmpty() == false)
	{
		mTileMap->SetTileHighlight(TargetTiles, ETileHighlightFlag::Select);
	}
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

