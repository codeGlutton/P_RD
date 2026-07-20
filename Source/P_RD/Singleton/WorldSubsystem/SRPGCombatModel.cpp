#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "Simulation/Factory/ObjectModelFactory.h"

#include "SRPGFramework/SRPGCommand.h"
#include "SRPGFramework/SRPGEnemyTurnPlanner.h"
#include "SRPGFramework/SRPGMoveAction.h"
#include "SRPGFramework/SRPGSkillAction.h"
#include "SRPGFramework/SRPGWarriorAreaAction.h"
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
	FText GetEnemyIntentGoalText(const UEnemyUnitModel* Enemy)
	{
		if (Enemy != nullptr)
		{
			switch (Enemy->GetMovementRole())
			{
			case ESRPGEnemyMovementRole::Anchor:
				return NSLOCTEXT("EnemyIntent", "GoalAnchor", "포자 포격 · 밀치고 위험 지대 생성 · 다음 행동 전 이탈 필요");
			case ESRPGEnemyMovementRole::Flanker:
				return NSLOCTEXT("EnemyIntent", "GoalFlanker", "거미줄 사냥 · 당기고 속박 · 이동이 붙은 기술로 끊기");
			case ESRPGEnemyMovementRole::Slider:
				return NSLOCTEXT("EnemyIntent", "GoalSlider", "탄성 돌진 · 경로를 밀며 압박 누적 · 같은 칸 반복 금지");
			case ESRPGEnemyMovementRole::Bulwark:
				return NSLOCTEXT("EnemyIntent", "GoalBulwark", "방패 압박 · 명중하면 한 칸 밀쳐내고 길목 점유");
			case ESRPGEnemyMovementRole::Lancer:
				return NSLOCTEXT("EnemyIntent", "GoalLancer", "직선 돌진 · 경로에 낀 유닛과 충돌");
			case ESRPGEnemyMovementRole::Bomber:
				return NSLOCTEXT("EnemyIntent", "GoalBomber", "화약 폭발 · 목표 칸 주변까지 함께 피해");
			case ESRPGEnemyMovementRole::Standard:
			default:
				break;
			}
		}

		switch (Enemy != nullptr ? Enemy->GetMoveTendency() : EMoveTendency::HoldRange)
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

	void BroadcastCollisionPresentation(
		UTileMapModel* TileMap,
		UBoardActorModel* Source,
		UBoardActorModel* Receiver)
	{
		if (TileMap == nullptr || Source == nullptr || Receiver == nullptr)
		{
			return;
		}
		FVector Direction = TileMap->TileToWorldLocation(Receiver->GetTileTransform().mIndex)
			- TileMap->TileToWorldLocation(Source->GetTileTransform().mIndex);
		Direction.Z = 0.0f;
		Direction = Direction.GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			Direction = FVector::ForwardVector;
		}
		Source->OnPlayImpactPresentation.Broadcast(
			Direction,
			1.0f,
			EImpactPresentationType::Source);
		Receiver->OnPlayImpactPresentation.Broadcast(
			Direction,
			1.0f,
			EImpactPresentationType::Receiver);
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
	mEnemyReinforcementTemplates = RoomSpawnData->mEnemyUnitPlacementDatas;
	mTotalReinforcementsSpawned = 0;
	mPlayerActionsSinceReinforcement = 0;

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
	const bool bCompletedPlayerTurn = TurnContext != nullptr
		&& TurnContext->GetOwner() != nullptr
		&& TurnContext->GetOwner()->IsPlayerUnitModel();

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

	AdvanceTurn(false, bCompletedPlayerTurn);
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

	if (AnyEnemyAlive == false && HasFutureReinforcements() == false)
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

UEnemyUnitModel* USRPGCombatModel::RegisterEnemyUnit(FEnemyUnitPlacementData& EnemyPlacementData)
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
	ApplyHordeEnemyStats(EnemyUnit);

	RegisterUnit(EnemyUnit, EnemyPlacementData.mTransform);
	return EnemyUnit;
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

void USRPGCombatModel::AdvanceTurn(bool IsInitialRound, bool bCompletedPlayerTurn)
{
	bool bStartingNewRound = IsInitialRound;
	if (IsInitialRound == false)
	{
		// 밀려있던 턴 구성 변경 요청안들 처리
		FlushPendingTurnRequests();

		if (bCompletedPlayerTurn)
		{
			// 플레이어 행동이 끝나면 이번 라운드에 대기 중인 적 무리의 첫 행동을 시작한다.
			const int32 EnemyTurnId = PopNextEnemyResponseTurnId();
			if (EnemyTurnId != INDEX_NONE)
			{
				mCurTurnContextOrder = mTurnContextOrder.FindNode(EnemyTurnId);
			}
			else
			{
				// 살아 있는 적이 모두 제거되는 정상 경로는 전투 종료 평가에서 빠져나간다.
				// 여기까지 온 경우에도 입력이 잠기지 않도록 새 라운드의 플레이어 턴으로 안전하게 복귀한다.
				NotifyRoundEnd();
				const int32 PlayerTurnId = FindPlayerTurnId();
				mCurTurnContextOrder = PlayerTurnId != INDEX_NONE ? mTurnContextOrder.FindNode(PlayerTurnId) : nullptr;
				bStartingNewRound = true;
			}
		}
		else
		{
			// 적 행동이 끝날 때마다 아직 대기 중인 다음 적을 바로 이어서 실행한다.
			// 살아 있는 적 전원이 한 번씩 행동한 뒤에만 플레이어에게 조작권을 돌려준다.
			const int32 NextEnemyTurnId = PopNextEnemyResponseTurnId();
			if (NextEnemyTurnId != INDEX_NONE)
			{
				mCurTurnContextOrder = mTurnContextOrder.FindNode(NextEnemyTurnId);
			}
			else
			{
				NotifyRoundEnd();
				const int32 PlayerTurnId = FindPlayerTurnId();
				mCurTurnContextOrder = PlayerTurnId != INDEX_NONE ? mTurnContextOrder.FindNode(PlayerTurnId) : nullptr;
				bStartingNewRound = true;
			}
		}
	}

	if (mCurTurnContextOrder == nullptr || mTurnContextMap.Contains(mCurTurnContextOrder->GetValue()) == false)
	{
		return;
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

	// 최초 진입 또는 적 전원의 행동이 모두 끝난 시점에만 라운드 이벤트를 보낸다.
	if (bStartingNewRound)
	{
		NotifyRoundStartIfNeeded(PresentationBarrier);
	}
}

bool USRPGCombatModel::HasFutureReinforcements() const
{
	return mPendingReinforcementTransforms.IsEmpty() == false
		|| (mEnemyReinforcementTemplates.IsEmpty() == false
			&& mRoundCount < ReinforcementFinalRound);
}

bool USRPGCombatModel::FindReinforcementSpawnTile(FTileTransform& OutTransform) const
{
	if (mTileMap == nullptr)
	{
		return false;
	}

	TArray<FTileIndex> VisibleRingTiles;
	TArray<FTileIndex> EdgeTiles;
	const int32 Width = mTileMap->GetWidth();
	const int32 Height = mTileMap->GetHeight();
	const FTileIndex PlayerTile = mPlayerUnit != nullptr
		? mPlayerUnit->GetTileTransform().mIndex
		: FTileIndex(Width / 2, Height / 2);
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const FTileIndex Candidate(X, Y);
			if (mTileMap->GetActorsOnTile(Candidate, ETileLayerFlag::All).IsEmpty() == false)
			{
				continue;
			}
			if (mPendingReinforcementTransforms.ContainsByPredicate([Candidate](const FTileTransform& Pending)
				{
					return Pending.mIndex == Candidate;
				}))
			{
				continue;
			}

			// 고정 카메라에서도 등장 순간이 보이도록 플레이어 기준 4~6칸 고리를 우선한다.
			const int32 Distance = FMath::Max(
				FMath::Abs(X - PlayerTile.mX),
				FMath::Abs(Y - PlayerTile.mY));
			if (Distance >= 4 && Distance <= 6)
			{
				VisibleRingTiles.Add(Candidate);
			}
			if (X == 0 || Y == 0 || X == Width - 1 || Y == Height - 1)
			{
				EdgeTiles.Add(Candidate);
			}
		}
	}
	const TArray<FTileIndex>& SpawnTiles = VisibleRingTiles.IsEmpty() ? EdgeTiles : VisibleRingTiles;
	if (SpawnTiles.IsEmpty())
	{
		return false;
	}

	// 투입할 때마다 시작점을 회전시켜 한쪽 방향만 반복하지 않는다.
	const int32 PickIndex = (mRoundCount * 3 + mTotalReinforcementsSpawned * 5) % SpawnTiles.Num();
	const FTileIndex Picked = SpawnTiles[PickIndex];
	OutTransform = FTileTransform(
		Picked,
		UTileMapModel::TileDeltaToDirection(Picked, PlayerTile, ETileActorDirection::Forward));
	return true;
}

void USRPGCombatModel::ApplyHordeEnemyStats(UEnemyUnitModel* EnemyUnit) const
{
	if (EnemyUnit == nullptr)
	{
		return;
	}
	UAttributeSetComponentModel* Attributes = EnemyUnit->GetAttributeComponentModel();
	if (Attributes == nullptr)
	{
		return;
	}

	float HordeHP = 12.0f;
	switch (EnemyUnit->GetDisplacementWeight())
	{
	case ESRPGDisplacementWeight::Light:
		HordeHP = 8.0f;
		break;
	case ESRPGDisplacementWeight::Heavy:
		HordeHP = 16.0f;
		break;
	case ESRPGDisplacementWeight::Medium:
	case ESRPGDisplacementWeight::Invalid:
	default:
		break;
	}
	// 뒤 라운드도 체력 벽 대신 숫자와 배치로 압박한다. 상승 폭은 최대 +4로 제한한다.
	HordeHP += StaticCast<float>(FMath::Clamp((mRoundCount - 1) / 2, 0, 2) * 2);
	Attributes->ApplyModToAttribute(UCombatTargetAttributeSet::GetMaxHPAttribute(), ETacticalModOp::Override, HordeHP);
	Attributes->ApplyModToAttribute(UCombatTargetAttributeSet::GetHPAttribute(), ETacticalModOp::Override, HordeHP);
	Attributes->ApplyModToAttribute(UCombatTargetAttributeSet::GetDefenseAttribute(), ETacticalModOp::Override, 0.0f);
	Attributes->ApplyModToAttribute(UEnemyUnitAttributeSet::GetRechargeMovementAttribute(), ETacticalModOp::Override, 1.0f);
}

UEnemyUnitModel* USRPGCombatModel::SpawnOneReinforcement()
{
	if (mEnemyReinforcementTemplates.IsEmpty())
	{
		return nullptr;
	}

	FTileTransform SpawnTransform;
	if (FindReinforcementSpawnTile(SpawnTransform) == false)
	{
		UE_LOG(LogSRPGCombat, Warning, TEXT("생존전 증원 실패: 빈 투입 타일 없음"));
		return nullptr;
	}
	return SpawnOneReinforcementAt(SpawnTransform);
}

int32 USRPGCombatModel::GetReinforcementEnemyCapForRound() const
{
	if (mRoundCount <= 3)
	{
		return 4;
	}
	return mRoundCount <= 5 ? 6 : 8;
}

UEnemyUnitModel* USRPGCombatModel::SpawnOneReinforcementAt(const FTileTransform& SpawnTransform)
{
	if (mEnemyReinforcementTemplates.IsEmpty())
	{
		return nullptr;
	}

	int32 LivingEnemies = 0;
	for (const TObjectPtr<UUnitModel>& Unit : mUnits)
	{
		if (Unit != nullptr && Unit->IsPlayerUnitModel() == false && Unit->IsDead() == false)
		{
			++LivingEnemies;
		}
	}
	const int32 EnemyCap = GetReinforcementEnemyCapForRound();
	if (LivingEnemies >= EnemyCap
		|| (mTileMap != nullptr
			&& mTileMap->GetActorsOnTile(SpawnTransform.mIndex, ETileLayerFlag::All).IsEmpty() == false))
	{
		return nullptr;
	}

	FEnemyUnitPlacementData Placement = mEnemyReinforcementTemplates[
		mTotalReinforcementsSpawned % mEnemyReinforcementTemplates.Num()];
	Placement.mTransform = SpawnTransform;
	Placement.mTurnPriority = 100 + mTotalReinforcementsSpawned;
	UEnemyUnitModel* SpawnedEnemy = RegisterEnemyUnit(Placement);
	if (SpawnedEnemy != nullptr)
	{
		static constexpr ESRPGEnemyMovementRole ReinforcementRoles[] = {
			ESRPGEnemyMovementRole::Lancer,
			ESRPGEnemyMovementRole::Bomber,
			ESRPGEnemyMovementRole::Bulwark,
		};
		SpawnedEnemy->SetMovementRoleOverride(
			ReinforcementRoles[mTotalReinforcementsSpawned % UE_ARRAY_COUNT(ReinforcementRoles)]);
	}
	++mTotalReinforcementsSpawned;
	UE_LOG(
		LogSRPGCombat,
		Log,
		TEXT("생존전 증원 #%d: (%d,%d), 생존 적 %d/%d"),
		mTotalReinforcementsSpawned,
		SpawnTransform.mIndex.mX,
		SpawnTransform.mIndex.mY,
		LivingEnemies + 1,
		EnemyCap);
	return SpawnedEnemy;
}

void USRPGCombatModel::QueueReinforcements(int32 DesiredCount)
{
	if (DesiredCount <= 0 || mEnemyReinforcementTemplates.IsEmpty())
	{
		return;
	}
	int32 LivingEnemies = 0;
	for (const TObjectPtr<UUnitModel>& Unit : mUnits)
	{
		if (Unit != nullptr && Unit->IsPlayerUnitModel() == false && Unit->IsDead() == false)
		{
			++LivingEnemies;
		}
	}
	const int32 Available = FMath::Max(
		GetReinforcementEnemyCapForRound() - LivingEnemies - mPendingReinforcementTransforms.Num(),
		0);
	for (int32 Index = 0; Index < FMath::Min(DesiredCount, Available); ++Index)
	{
		FTileTransform SpawnTransform;
		if (FindReinforcementSpawnTile(SpawnTransform) == false)
		{
			break;
		}
		mPendingReinforcementTransforms.Add(SpawnTransform);
	}
	BroadcastEnemyIntentChanged();
	RefreshEnemyIntentHighlights();
}

void USRPGCombatModel::CommitPendingReinforcements()
{
	TArray<FTileTransform> StillPending;
	for (const FTileTransform& SpawnTransform : mPendingReinforcementTransforms)
	{
		const bool bIntercepted = mPlayerUnit != nullptr && mPlayerUnit->IsDead() == false
			&& mPlayerUnit->GetTileTransform().mIndex == SpawnTransform.mIndex;
		UEnemyUnitModel* SpawnedEnemy = nullptr;
		if (bIntercepted && mTileMap != nullptr)
		{
			for (int32 DeltaY = -1; DeltaY <= 1 && SpawnedEnemy == nullptr; ++DeltaY)
			{
				for (int32 DeltaX = -1; DeltaX <= 1 && SpawnedEnemy == nullptr; ++DeltaX)
				{
					if (DeltaX == 0 && DeltaY == 0) { continue; }
					const FTileIndex Candidate(
						SpawnTransform.mIndex.mX + DeltaX,
						SpawnTransform.mIndex.mY + DeltaY);
					if (mTileMap->IsValidIndex(Candidate) == false
						|| mTileMap->GetActorsOnTile(Candidate, ETileLayerFlag::All).IsEmpty() == false)
					{
						continue;
					}
					const FTileTransform InterceptedTransform(
						Candidate,
						UTileMapModel::TileDeltaToDirection(Candidate, SpawnTransform.mIndex, ETileActorDirection::Forward));
					SpawnedEnemy = SpawnOneReinforcementAt(InterceptedTransform);
				}
			}
			if (SpawnedEnemy != nullptr)
			{
				mInterceptedReinforcementUnits.Add(SpawnedEnemy);
				mWarriorFlow = FMath::Clamp(mWarriorFlow + 1, 0, 3);
				mStandstillPressure = FMath::Max(mStandstillPressure - 1, 0);
				AddWarriorCombo(2);
				BroadcastCollisionPresentation(mTileMap, mPlayerUnit, SpawnedEnemy);
			}
		}
		else
		{
			SpawnedEnemy = SpawnOneReinforcementAt(SpawnTransform);
		}
		if (SpawnedEnemy == nullptr)
		{
			StillPending.Add(SpawnTransform);
		}
	}
	mPendingReinforcementTransforms = MoveTemp(StillPending);
}

void USRPGCombatModel::SpawnRoundReinforcements()
{
	if (mRoundCount <= 1 || mRoundCount > ReinforcementFinalRound
		|| mEnemyReinforcementTemplates.IsEmpty())
	{
		return;
	}

	// 짝수 라운드에 다음 라운드 투입 위치를 먼저 공개한다. 후반만 두 방향을 동시에 예고한다.
	if ((mRoundCount % 2) == 0)
	{
		QueueReinforcements(mRoundCount >= 6 ? 2 : 1);
	}
}

void USRPGCombatModel::SpawnActionReinforcementIfNeeded()
{
	if (mRoundCount <= 0 || mRoundCount >= ReinforcementFinalRound)
	{
		return;
	}

	mPlayerActionsSinceReinforcement = FMath::Min(
		mPlayerActionsSinceReinforcement + 1,
		ReinforcementActionInterval);
	if (mPlayerActionsSinceReinforcement < ReinforcementActionInterval)
	{
		return;
	}

	QueueReinforcements(1);
	mPlayerActionsSinceReinforcement = 0;
}

void USRPGCombatModel::NotifyRoundStartIfNeeded(TSharedPtr<FPresentationBarrier> RoundPresentationBarrier)
{
	if (mTurnContextOrder.IsEmpty() == false && mCurTurnContextOrder == mTurnContextOrder.GetHead())
	{
		++mRoundCount;
		mPlayerMovedThisRound = false;
		if (mExecutionOpportunityExpireRound != INDEX_NONE
			&& mRoundCount > mExecutionOpportunityExpireRound)
		{
			mExecutionOpportunityTile = FTileIndex::Invalid;
			mExecutionOpportunityExpireRound = INDEX_NONE;
		}
		mPlayerRoundStartTile = mPlayerUnit != nullptr
			? mPlayerUnit->GetTileTransform().mIndex
			: FTileIndex::Invalid;
		// 전 라운드에 주황색으로 예고한 적만 지금 투입한다. 이번 적 응답 큐에는 들어가지만,
		// 플레이어가 투입 타일을 본 뒤 한 번 행동할 기회가 반드시 먼저 주어진다.
		CommitPendingReinforcements();
		SpawnActionReinforcementIfNeeded();
		SpawnRoundReinforcements();

		for (const TObjectPtr<UUnitModel>& Unit : mUnits)
		{
			Unit->OnBeginRound();
		}
		for (const TObjectPtr<UBoardActorModel>& Obstacle : mObstacles)
		{
			Obstacle->OnBeginRound();
		}

		// 플레이어 행동 전에 목표와 현재 경로를 공개한다. 이후 실제 이동/스킬이
		// 끝날 때마다 같은 전술 정체성을 유지한 채 현재 전장 기준 한 칸 행동을 갱신한다.
		PrepareEnemyIntents();
		RebuildEnemyResponseOrder();

		OnBeginAnyRoundUI.Broadcast(RoundPresentationBarrier, mRoundCount);
	}
}

void USRPGCombatModel::NotifyRoundEnd()
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

void USRPGCombatModel::RebuildEnemyResponseOrder()
{
	mPendingEnemyResponseTurnIds.Reset();
	if (mTurnContextOrder.IsEmpty())
	{
		return;
	}

	auto* Node = mTurnContextOrder.GetHead();
	const int32 TurnCount = mTurnContextOrder.Num();
	for (int32 Index = 0; Index < TurnCount; ++Index)
	{
		const int32 TurnId = Node->GetValue();
		const TObjectPtr<USRPGTurnContext>* TurnContext = mTurnContextMap.Find(TurnId);
		UUnitModel* Owner = TurnContext != nullptr && IsValid(TurnContext->Get())
			? TurnContext->Get()->GetOwner()
			: nullptr;
		if (Owner != nullptr && Owner->IsPlayerUnitModel() == false && Owner->IsDead() == false)
		{
			mPendingEnemyResponseTurnIds.Add(TurnId);
		}
		Node = Node->GetNextNode();
	}
}

int32 USRPGCombatModel::PopNextEnemyResponseTurnId()
{
	while (mPendingEnemyResponseTurnIds.IsEmpty() == false)
	{
		const int32 TurnId = mPendingEnemyResponseTurnIds[0];
		mPendingEnemyResponseTurnIds.RemoveAt(0);
		const TObjectPtr<USRPGTurnContext>* TurnContext = mTurnContextMap.Find(TurnId);
		UUnitModel* Owner = TurnContext != nullptr && IsValid(TurnContext->Get())
			? TurnContext->Get()->GetOwner()
			: nullptr;
		if (Owner != nullptr
			&& Owner->IsPlayerUnitModel() == false
			&& Owner->IsDead() == false
			&& mTurnContextOrder.FindNode(TurnId) != nullptr)
		{
			return TurnId;
		}
	}
	return INDEX_NONE;
}

int32 USRPGCombatModel::FindPlayerTurnId() const
{
	if (mTurnContextOrder.IsEmpty())
	{
		return INDEX_NONE;
	}

	auto* Node = mTurnContextOrder.GetHead();
	const int32 TurnCount = mTurnContextOrder.Num();
	for (int32 Index = 0; Index < TurnCount; ++Index)
	{
		const int32 TurnId = Node->GetValue();
		const TObjectPtr<USRPGTurnContext>* TurnContext = mTurnContextMap.Find(TurnId);
		if (TurnContext != nullptr
			&& IsValid(TurnContext->Get())
			&& TurnContext->Get()->GetOwner() != nullptr
			&& TurnContext->Get()->GetOwner()->IsPlayerUnitModel())
		{
			return TurnId;
		}
		Node = Node->GetNextNode();
	}
	return INDEX_NONE;
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

FText USRPGCombatModel::GetMovementDangerLabel() const
{
	if (mExecutionOpportunityTile != FTileIndex::Invalid)
	{
		return NSLOCTEXT("WarriorFlow", "ExecutionOpportunity", "충돌 균열! 다음 기술의 착지를 표시된 적 곁에 두면 FLOW 최대");
	}
	if (mTetheringEnemy.IsValid())
	{
		return NSLOCTEXT("WarriorFlow", "TetherDanger", "거미줄에 묶임 · 이동이 붙은 기술로 끊기");
	}
	if (mPersistentMovementHazardTile != FTileIndex::Invalid && mPlayerUnit != nullptr
		&& mPlayerUnit->GetTileTransform().mIndex == mPersistentMovementHazardTile)
	{
		return NSLOCTEXT("WarriorFlow", "SporeDanger", "발밑 포자 지대 · 머물면 피해");
	}
	if (mStandstillPressure >= 2)
	{
		return NSLOCTEXT("WarriorFlow", "SurroundedDanger", "포위 압박 · 다음 피격 피해 증가");
	}
	if (mStandstillPressure == 1)
	{
		return NSLOCTEXT("WarriorFlow", "AnchorWindow", "닻 자세 발동 · 한 번 더 머물면 포위 압박");
	}
	return NSLOCTEXT("WarriorFlow", "SkillMovementReady", "모든 기술이 이동+타격 · 착지 칸이 곧 다음 전술 위치");
}

void USRPGCombatModel::NotifyWarriorSkillMovement(
	const FTileIndex& From,
	const FTileIndex& To,
	const FText& SkillName)
{
	if (mPlayerUnit == nullptr || mPlayerUnit->IsDead() || From == To)
	{
		return;
	}
	mPlayerMovedThisRound = true;
	mWarriorFlow = FMath::Clamp(mWarriorFlow + 1, 0, 3);
	mStandstillPressure = FMath::Max(mStandstillPressure - 1, 0);
	if (mExecutionOpportunityTile != FTileIndex::Invalid
		&& FMath::Max(
			FMath::Abs(To.mX - mExecutionOpportunityTile.mX),
			FMath::Abs(To.mY - mExecutionOpportunityTile.mY)) <= 1)
	{
		mWarriorFlow = 3;
		AddWarriorCombo(2);
		mExecutionOpportunityTile = FTileIndex::Invalid;
		mExecutionOpportunityExpireRound = INDEX_NONE;
	}
	if (To != mPersistentMovementHazardTile)
	{
		mPersistentMovementHazardTile = FTileIndex::Invalid;
	}
	mTetheringEnemy.Reset();
	AddWarriorCombo(1);
	UE_LOG(LogSRPGCombat, Verbose, TEXT("전사 스킬 이동: %s (%d,%d)->(%d,%d)"),
		*SkillName.ToString(), From.mX, From.mY, To.mX, To.mY);
	BroadcastEnemyIntentChanged();
	RefreshEnemyIntentHighlights();
}

FText USRPGCombatModel::GetWarriorAftermathStanceLabel() const
{
	return NSLOCTEXT("WarriorFlow", "SelfContainedSkills", "후속 행동 없음 · 이동과 타격을 기술 안에서 즉시 해결");
}

const TArray<FSRPGEnemyIntent>& USRPGCombatModel::GetEnemyIntents() const
{
	return mEnemyIntents;
}

void USRPGCombatModel::ReplanEnemyIntentsAfterPlayerAction(
	const USRPGAction* Action,
	ESRPGActionResult ActionResult)
{
	if (Action == nullptr
		|| ActionResult != ESRPGActionResult::Succeeded
		|| Action->GetInstigator() != mPlayerUnit
		|| (Cast<USRPGMoveAction>(Action) == nullptr
			&& Cast<USRPGSkillAction>(Action) == nullptr
			&& Cast<USRPGWarriorAreaAction>(Action) == nullptr)
		|| mCombatPhase != ESRPGCombatRoomPhase::CombatPlay
		|| mPlayerUnit == nullptr
		|| mPlayerUnit->IsDead())
	{
		return;
	}
	ActivateWarriorAftermath(Action);

	bool bChanged = false;
	for (FSRPGEnemyIntent& Intent : mEnemyIntents)
	{
		if (Intent.mEnemy == nullptr
			|| Intent.mEnemy->IsDead()
			|| Intent.mResult != ESRPGEnemyIntentResult::Planned)
		{
			continue;
		}
		// 투입 칸을 선점당한 증원은 "등장 턴 행동 상실"이 보상이다. 플레이어 행동 뒤의
		// 일반 AI 재계산이 이 고정 대기 계획을 덮어쓰지 않게 그대로 유지한다.
		if (mInterceptedReinforcementUnits.ContainsByPredicate([Enemy = Intent.mEnemy.Get()](
			const TWeakObjectPtr<UEnemyUnitModel>& Unit)
			{
				return Unit.Get() == Enemy;
			}))
		{
			continue;
		}

		// 적의 성향과 공개했던 스킬은 유지한다. 플레이어 행동마다 스킬까지 무작위로 바뀌면
		// 영리해지는 대신 규칙이 자의적으로 보이므로, 경로/대상/효과 타일만 최신화한다.
		const TArray<FTileIndex> PreviousPath = Intent.mPathTileIndexes;
		const TArray<FTileIndex> PreviousEffect = Intent.mEffectTileIndexes;
		const FTileIndex PreviousDestination = Intent.mPlannedDestination;
		const FTileIndex PreviousTarget = Intent.mTargetTile;
		const FTileIndex OlderPreviousDestination = Intent.mPreviousDestination;
		const bool bRespondingToDisplacement = Intent.mHasPendingDisplacementResponse;
		// 역할 플래너가 "방금 공개했던 자리로 되감기"를 정확히 평가할 수 있게 현재 계획을 잠시 보존한다.
		Intent.mPreviousDestination = PreviousDestination;
		if (RebuildEnemyIntentPlan(Intent, /*bPreserveSkill*/true) == false)
		{
			Intent.mPreviousDestination = OlderPreviousDestination;
			continue;
		}
		const bool bPlanActuallyChanged = PreviousPath != Intent.mPathTileIndexes
				|| PreviousEffect != Intent.mEffectTileIndexes
				|| PreviousDestination != Intent.mPlannedDestination
				|| PreviousTarget != Intent.mTargetTile;
		if (bPlanActuallyChanged == false)
		{
			Intent.mPreviousDestination = OlderPreviousDestination;
			Intent.mHasPendingDisplacementResponse = false;
			if (bRespondingToDisplacement)
			{
				Intent.mResultText = NSLOCTEXT(
					"EnemyIntent",
					"DisplacedPositionAlreadyUseful",
					"착지 적응 · 현재 칸이 이미 역할 자리 · 원위치 복귀 안 함");
				bChanged = true;
			}
			continue;
		}

		Intent.mPreviousPathTileIndexes = PreviousPath;
		Intent.mPreviousDestination = PreviousDestination;
		++Intent.mPlanRevision;
		Intent.mResultText = bRespondingToDisplacement
			? FText::Format(
				NSLOCTEXT(
					"EnemyIntent",
					"PlayerDisplacementRoleReplanned",
					"착지 적응 #{0} · 현재 칸에서 한 칸 대응 · 충돌 회복 -{1}"),
				FText::AsNumber(Intent.mPlanRevision),
				FText::AsNumber(Intent.mRecoveryMovePenalty))
			: FText::Format(
				NSLOCTEXT(
					"EnemyIntent",
					"PlayerActionReplanned",
					"대응 #{0} · 현재 전장에서 한 칸 행동 재계산"),
				FText::AsNumber(Intent.mPlanRevision));
		Intent.mHasPendingDisplacementResponse = false;
		bChanged = true;
	}

	if (bChanged)
	{
		BroadcastEnemyIntentChanged();
		RefreshEnemyIntentHighlights();
	}
}

bool USRPGCombatModel::RebuildEnemyIntentPlan(FSRPGEnemyIntent& Intent, bool bPreserveSkill)
{
	UEnemyUnitModel* Enemy = Intent.mEnemy.Get();
	if (Enemy == nullptr || Enemy->IsDead() || mTileMap == nullptr || mPlayerUnit == nullptr || mPlayerUnit->IsDead())
	{
		return false;
	}

	int32 PlannedSkillIndex = INDEX_NONE;
	TArray<FTileIndex> ReservedDestinations;
	ReservedDestinations.Reserve(mEnemyIntents.Num());
	for (const FSRPGEnemyIntent& OtherIntent : mEnemyIntents)
	{
		if (&OtherIntent != &Intent
			&& OtherIntent.mEnemy != nullptr
			&& OtherIntent.mEnemy->IsDead() == false
			&& OtherIntent.mResult == ESRPGEnemyIntentResult::Planned)
		{
			ReservedDestinations.Add(OtherIntent.mPlannedDestination);
		}
	}
	const int32 EffectiveMoveRange = FMath::Clamp(
		Intent.mPlannedMoveRange - Intent.mRecoveryMovePenalty,
		0,
		1);
	TArray<TInstancedStruct<FSRPGCommand>> Commands = USRPGEnemyTurnPlanner::PlanTurn(
		Enemy,
		mPlayerUnit,
		mTileMap,
		URandomStreamFunctionLibrary::GetEventStream(this),
		EffectiveMoveRange,
		bPreserveSkill ? Intent.mSkillIndex : INDEX_NONE,
		&PlannedSkillIndex,
		&ReservedDestinations,
		Intent.mPreviousDestination,
		Intent.mHasPendingDisplacementResponse,
		Intent.mDisplacedFromTile);

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

	// 데이터 에셋의 모호한 원래 스킬명 앞에 이번 적이 실제로 만드는 전장 변화를 붙인다.
	// HUD 한 줄만 봐도 "어떻게 때리고, 맞으면 어디로 움직이는지"를 알 수 있다.
	switch (Enemy->GetMovementRole())
	{
	case ESRPGEnemyMovementRole::Anchor:
		if (Intent.mTargetTile != FTileIndex::Invalid)
		{
			Intent.mSkillName = FText::Format(
				NSLOCTEXT("EnemyIntent", "MushroomSignatureName", "포자 밀치기 → {0}"),
				Intent.mSkillName);
		}
		break;
	case ESRPGEnemyMovementRole::Flanker:
		if (Intent.mTargetTile != FTileIndex::Invalid)
		{
			Intent.mSkillName = FText::Format(
				NSLOCTEXT("EnemyIntent", "SpiderSignatureName", "거미줄 견인 → {0}"),
				Intent.mSkillName);
		}
		break;
	case ESRPGEnemyMovementRole::Slider:
		if (Intent.mPlannedDestination != Intent.mPlannedOrigin)
		{
			Intent.mSkillName = Intent.mTargetTile != FTileIndex::Invalid
				? FText::Format(
					NSLOCTEXT("EnemyIntent", "SlimeSignatureAttackName", "탄성 돌진 → {0}"),
					Intent.mSkillName)
				: NSLOCTEXT("EnemyIntent", "SlimeSignatureMoveName", "탄성 돌진");
		}
		break;
	case ESRPGEnemyMovementRole::Bulwark:
		Intent.mSkillName = FText::Format(
			NSLOCTEXT("EnemyIntent", "BulwarkSignatureName", "방패 압박 → {0}"), Intent.mSkillName);
		break;
	case ESRPGEnemyMovementRole::Lancer:
		Intent.mSkillName = FText::Format(
			NSLOCTEXT("EnemyIntent", "LancerSignatureName", "직선 돌진 → {0}"), Intent.mSkillName);
		break;
	case ESRPGEnemyMovementRole::Bomber:
		Intent.mSkillName = FText::Format(
			NSLOCTEXT("EnemyIntent", "BomberSignatureName", "화약 폭발 → {0}"), Intent.mSkillName);
		break;
	case ESRPGEnemyMovementRole::Standard:
	default:
		break;
	}

	if (TObjectPtr<USRPGTurnContext>* TurnContext = mTurnContextMap.Find(Intent.mTurnId);
		TurnContext != nullptr && IsValid(TurnContext->Get()))
	{
		TurnContext->Get()->SetFixedEnemyPlan(MoveTemp(Commands));
		return true;
	}
	return false;
}

bool USRPGCombatModel::BuildEnemyIntentForTurn(
	USRPGTurnContext* TurnContext,
	int32 ExecutionOrder,
	FSRPGEnemyIntent& OutIntent)
{
	if (TurnContext == nullptr)
	{
		return false;
	}

	UEnemyUnitModel* Enemy = Cast<UEnemyUnitModel>(TurnContext->GetOwner());
	if (Enemy == nullptr || Enemy->IsDead())
	{
		return false;
	}
	if (mInterceptedReinforcementUnits.ContainsByPredicate([Enemy](const TWeakObjectPtr<UEnemyUnitModel>& Unit)
		{
			return Unit.Get() == Enemy;
		}))
	{
		OutIntent = FSRPGEnemyIntent();
		OutIntent.mTurnId = TurnContext->GetTurnId();
		OutIntent.mExecutionOrder = ExecutionOrder;
		OutIntent.mEnemy = Enemy;
		OutIntent.mSkillName = NSLOCTEXT("EnemyIntent", "InterceptedReinforcementAction", "증원 요격 · 넘어짐");
		OutIntent.mGoalText = NSLOCTEXT("EnemyIntent", "InterceptedReinforcementGoal", "주황 투입 칸을 선점당해 이번 행동을 잃음");
		OutIntent.mPlannedOrigin = Enemy->GetTileTransform().mIndex;
		OutIntent.mPlannedDestination = OutIntent.mPlannedOrigin;
		OutIntent.mPathTileIndexes = { OutIntent.mPlannedOrigin };
		OutIntent.mResultText = NSLOCTEXT("EnemyIntent", "InterceptedReinforcementTelegraph", "요격 성공 · 이 적은 등장 직후 넘어져 대기");
		TArray<TInstancedStruct<FSRPGCommand>> SkippedPlan;
		TInstancedStruct<FSRPGCommand>& TurnEnd = SkippedPlan.AddDefaulted_GetRef();
		TurnEnd.InitializeAs<FSRPGTurnEndCommand>();
		TurnContext->SetFixedEnemyPlan(MoveTemp(SkippedPlan));
		return true;
	}

	UAttributeSetComponentModel* Attributes = Enemy->GetAttributeComponentModel();
	const int32 PlannedMoveRange = Attributes != nullptr
		? FMath::Clamp(FMath::RoundToInt(Attributes->GetAttributeCurrentValue(UEnemyUnitAttributeSet::GetRechargeMovementAttribute())), 0, 1)
		: 0;

	OutIntent = FSRPGEnemyIntent();
	OutIntent.mTurnId = TurnContext->GetTurnId();
	OutIntent.mExecutionOrder = ExecutionOrder;
	OutIntent.mEnemy = Enemy;
	OutIntent.mGoalText = GetEnemyIntentGoalText(Enemy);
	OutIntent.mPlannedMoveRange = PlannedMoveRange;
	OutIntent.mResultText = NSLOCTEXT(
		"EnemyIntent",
		"PlanTelegraphed",
		"초기 계획 공개 · 플레이어 행동 뒤 현재 위치에서 즉시 재계산");
	return RebuildEnemyIntentPlan(OutIntent, /*bPreserveSkill*/false);
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
		FSRPGEnemyIntent Intent;
		if (BuildEnemyIntentForTurn(TurnContext, ExecutionOrder, Intent))
		{
			++ExecutionOrder;
			mEnemyIntents.Add(MoveTemp(Intent));
		}
	}

	// 첫 안내 대상은 다음에 실제로 대응할 수 있도록 실행 순서가 가장 빠른 적을 우선한다.
	// 플레이어 발앞까지 실제로 끌 수 있는 적만 추천해 안내와 판정을 일치시킨다.
	const FTileIndex PlayerTile = mPlayerUnit->GetTileTransform().mIndex;
	auto CanBePulledToPlayer = [this, PlayerTile](const FSRPGEnemyIntent& Intent)
	{
		if (Intent.mEnemy == nullptr || Intent.mEnemy->IsDead())
		{
			return false;
		}
		const FTileIndex EnemyTile = Intent.mEnemy->GetTileTransform().mIndex;
		if (FMath::Max(
			FMath::Abs(EnemyTile.mX - PlayerTile.mX),
			FMath::Abs(EnemyTile.mY - PlayerTile.mY)) > 8)
		{
			return false;
		}
		const TArray<FTileIndex> PullPath = mTileMap->GetPullPath(PlayerTile, EnemyTile, 64);
		if (PullPath.IsEmpty())
		{
			return false;
		}
		const FTileIndex& PullEnd = PullPath.Last();
		return FMath::Max(
			FMath::Abs(PullEnd.mX - PlayerTile.mX),
			FMath::Abs(PullEnd.mY - PlayerTile.mY)) == 1;
	};

	FSRPGEnemyIntent* RecommendedIntent = mEnemyIntents.FindByPredicate(CanBePulledToPlayer);
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

void USRPGCombatModel::AddWarriorCombo(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	if (mWarriorComboRound != mRoundCount)
	{
		mWarriorComboRound = mRoundCount;
		mWarriorCombo = 0;
	}
	mWarriorCombo = FMath::Clamp(mWarriorCombo + Amount, 0, 99);
	mWarriorMomentum = FMath::Max(mWarriorMomentum, FMath::Clamp(mWarriorCombo / 2, 0, 3));
}

void USRPGCombatModel::ActivateWarriorAftermath(const USRPGAction* Action)
{
	if (Action == nullptr || Action->ConsumesTurn() == false)
	{
		return;
	}
	const FTileIndex CurrentPlayerTile = mPlayerUnit != nullptr
		? mPlayerUnit->GetTileTransform().mIndex
		: FTileIndex::Invalid;
	const USRPGMoveAction* MoveAction = Cast<USRPGMoveAction>(Action);
	if (CurrentPlayerTile != FTileIndex::Invalid
		&& CurrentPlayerTile != mPlayerRoundStartTile
		&& mPlayerMovedThisRound == false)
	{
		NotifyWarriorSkillMovement(
			mPlayerRoundStartTile,
			CurrentPlayerTile,
			NSLOCTEXT("WarriorFlow", "FallbackSkillMovement", "전사 기술"));
	}
	if (MoveAction != nullptr && MoveAction->IsWarriorLeap())
	{
		// 장애물과 전선을 뛰어넘는 도약은 가장 큰 위치 투자다. 착지 연계를 바로 쓸 수 있게 보상한다.
		mWarriorFlow = FMath::Max(mWarriorFlow, 2);
		AddWarriorCombo(1);
	}
	else if (MoveAction != nullptr && MoveAction->IsWarriorCharge())
	{
		mWarriorFlow = FMath::Clamp(mWarriorFlow + 1, 0, 3);
	}
	if (mPlayerMovedThisRound)
	{
		if (CurrentPlayerTile != mPersistentMovementHazardTile)
		{
			mPersistentMovementHazardTile = FTileIndex::Invalid;
		}
		mTetheringEnemy.Reset();
	}
	else
	{
		mStandstillPressure = FMath::Clamp(mStandstillPressure + 1, 0, 3);
		const bool bStandingInHazard = CurrentPlayerTile != FTileIndex::Invalid
			&& CurrentPlayerTile == mPersistentMovementHazardTile;
		const bool bStillTethered = mTetheringEnemy.IsValid();
		if (bStandingInHazard || bStillTethered)
		{
			ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(mPlayerUnit));
			const FVector ImpactDirection = bStillTethered && mTileMap != nullptr
				? (mTileMap->TileToWorldLocation(CurrentPlayerTile)
					- mTileMap->TileToWorldLocation(mTetheringEnemy->GetTileTransform().mIndex)).GetSafeNormal()
				: FVector::ForwardVector;
			mPlayerUnit->OnPlayImpactPresentation.Broadcast(
				ImpactDirection,
				1.0f,
				EImpactPresentationType::Receiver);
		}
	}

	// 자동 발차기/패링/사슬 반동 같은 후속 행동은 두지 않는다. 각 기술이 자신의 이동과
	// 충돌을 끝낸 뒤 적 페이즈로 한 번만 넘어가야 입력·재계산 순서가 흔들리지 않는다.
	mWarriorAftermathStance = ESRPGWarriorAftermathStance::None;
	mWarriorAftermathReactions = 0;
	mPendingWarriorAftermathStance = ESRPGWarriorAftermathStance::None;
	mWarriorAftermathTarget.Reset();
	AddWarriorCombo(1);
	BroadcastEnemyIntentChanged();
}

UBoardActorModel* USRPGCombatModel::FindBlockingActorAt(
	const FTileIndex& Tile,
	const UBoardActorModel* MovingActor) const
{
	if (mTileMap == nullptr || MovingActor == nullptr)
	{
		return nullptr;
	}
	for (UBoardActorModel* Actor : mTileMap->GetActorsOnTile(Tile, ETileLayerFlag::All))
	{
		if (Actor != nullptr && Actor != MovingActor
			&& EnumHasAnyFlags(Actor->GetBlockLayerFlags(), MovingActor->GetTileLayerFlags()))
		{
			return Actor;
		}
	}
	return nullptr;
}

void USRPGCombatModel::ResolveWarriorMobilityImpact(UUnitModel* Warrior, bool bLeapImpact)
{
	if (Warrior == nullptr || Warrior != mPlayerUnit || Warrior->IsDead() || mTileMap == nullptr)
	{
		return;
	}
	const FTileIndex PlayerTile = Warrior->GetTileTransform().mIndex;
	TArray<UEnemyUnitModel*> Targets;
	for (const TObjectPtr<UUnitModel>& Unit : mUnits)
	{
		UEnemyUnitModel* Enemy = Cast<UEnemyUnitModel>(Unit);
		if (Enemy == nullptr || Enemy->IsDead()
			|| Enemy->GetTeamAttitudeTowards(*Warrior) != ETeamAttitude::Hostile)
		{
			continue;
		}
		const FTileIndex UnitTile = Enemy->GetTileTransform().mIndex;
		if (FMath::Max(FMath::Abs(UnitTile.mX - PlayerTile.mX), FMath::Abs(UnitTile.mY - PlayerTile.mY)) == 1)
		{
			Targets.Add(Enemy);
		}
	}
	for (UEnemyUnitModel* Target : Targets)
	{
		const int32 HitCount = bLeapImpact ? 5 : 3;
		for (int32 Hit = 0; Hit < HitCount; ++Hit)
		{
			ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(Target));
		}
		BroadcastCollisionPresentation(mTileMap, Warrior, Target);
		AddWarriorCombo(1);
		if (FSRPGEnemyIntent* Intent = FindEnemyIntent(Target))
		{
			Intent->mResultText = FText::Format(
				NSLOCTEXT("WarriorFlow", "MobilityImpactResult", "{0}\n{1} 착지 타격 · 이동 액션 안에서 즉시 해결"),
				Intent->mResultText,
				bLeapImpact
					? NSLOCTEXT("WarriorFlow", "LeapAssault", "도약 강습")
					: NSLOCTEXT("WarriorFlow", "StepSlash", "질풍 베기"));
		}
	}
	BroadcastEnemyIntentChanged();
	RefreshEnemyIntentHighlights();
}

bool USRPGCombatModel::TryResolveWarriorAftermath(FSRPGEnemyIntent& Intent)
{
	UUnitModel* Enemy = Intent.mEnemy.Get();
	if (Enemy == nullptr || Enemy->IsDead() || mPlayerUnit == nullptr || mPlayerUnit->IsDead()
		|| mWarriorAftermathStance == ESRPGWarriorAftermathStance::None
		|| mWarriorAftermathReactions <= 0)
	{
		return false;
	}
	const FTileIndex PlayerTile = mPlayerUnit->GetTileTransform().mIndex;
	const FTileIndex EnemyTile = Enemy->GetTileTransform().mIndex;
	const bool bMoves = Intent.mPlannedDestination != FTileIndex::Invalid
		&& Intent.mPlannedDestination != EnemyTile;
	const bool bThreatensPlayer = Intent.mTargetTile == PlayerTile
		|| Intent.mEffectTileIndexes.Contains(PlayerTile);
	const bool bApproachesPlayer = Intent.mPlannedDestination != FTileIndex::Invalid
		&& FMath::Max(
			FMath::Abs(Intent.mPlannedDestination.mX - PlayerTile.mX),
			FMath::Abs(Intent.mPlannedDestination.mY - PlayerTile.mY)) <= 1;
	const bool bIsAftermathTarget = mWarriorAftermathTarget.IsValid()
		&& mWarriorAftermathTarget.Get() == Enemy;

	FText ResultText;
	switch (mWarriorAftermathStance)
	{
	case ESRPGWarriorAftermathStance::Guard:
		if (bThreatensPlayer)
		{
			ResultText = NSLOCTEXT("WarriorFlow", "AutoParry", "연계 반격! 첫 공격을 검으로 튕겨 행동 취소");
		}
		break;
	case ESRPGWarriorAftermathStance::Tether:
		if (bIsAftermathTarget && bMoves)
		{
			ResultText = NSLOCTEXT("WarriorFlow", "TetherRebound", "사슬 반동! 움직이려던 적을 다시 잡아당겨 넘어뜨림");
		}
		break;
	case ESRPGWarriorAftermathStance::Trip:
		if (bIsAftermathTarget && bMoves)
		{
			ResultText = NSLOCTEXT("WarriorFlow", "TripReaction", "다리 걸기 발동! 첫걸음에 넘어져 행동 취소");
		}
		break;
	case ESRPGWarriorAftermathStance::Mobility:
		if (bApproachesPlayer)
		{
			ResultText = NSLOCTEXT("WarriorFlow", "MobilityCounter", "기동 반격! 접근 경로를 어깨로 끊어 행동 취소");
		}
		break;
	case ESRPGWarriorAftermathStance::None:
	default:
		break;
	}
	if (ResultText.IsEmpty())
	{
		return false;
	}

	ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(Enemy));
	if (mWarriorMomentum >= 3)
	{
		ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(Enemy));
	}
	BroadcastCollisionPresentation(mTileMap, mPlayerUnit, Enemy);
	if (TObjectPtr<USRPGTurnContext>* TurnContext = mTurnContextMap.Find(Intent.mTurnId);
		TurnContext != nullptr && IsValid(TurnContext->Get()))
	{
		TArray<TInstancedStruct<FSRPGCommand>> CancelledPlan;
		TInstancedStruct<FSRPGCommand>& TurnEnd = CancelledPlan.AddDefaulted_GetRef();
		TurnEnd.InitializeAs<FSRPGTurnEndCommand>();
		TurnContext->Get()->SetFixedEnemyPlan(MoveTemp(CancelledPlan));
	}
	AppendEnemyIntentResult(Intent, ESRPGEnemyIntentResult::Cancelled, ResultText);
	--mWarriorAftermathReactions;
	AddWarriorCombo(2);
	if (mWarriorAftermathReactions <= 0)
	{
		mWarriorAftermathStance = ESRPGWarriorAftermathStance::None;
		mWarriorAftermathTarget.Reset();
	}
	BroadcastEnemyIntentChanged();
	RefreshEnemyIntentHighlights();
	return true;
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
		if (TryResolveWarriorAftermath(*Intent))
		{
			return;
		}
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
	mInterceptedReinforcementUnits.RemoveAll([Enemy](const TWeakObjectPtr<UEnemyUnitModel>& Unit)
	{
		return Unit.IsValid() == false || Unit.Get() == Enemy;
	});
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

	const FText DisplacementLabel = DisplacementType == ESRPGPlayerDisplacementType::Swap
		? NSLOCTEXT("EnemyIntent", "SwappedLabel", "자리 바꾸기")
		: (DisplacementType == ESRPGPlayerDisplacementType::Throw
		? NSLOCTEXT("EnemyIntent", "ThrownLabel", "붙잡아 던지기")
		: (DisplacementType == ESRPGPlayerDisplacementType::Pull
			? NSLOCTEXT("EnemyIntent", "PulledLabel", "끌어오기")
			: NSLOCTEXT("EnemyIntent", "PushedLabel", "밀기")));
	mPendingWarriorAftermathStance = (DisplacementType == ESRPGPlayerDisplacementType::Pull
		|| DisplacementType == ESRPGPlayerDisplacementType::Throw
		|| DisplacementType == ESRPGPlayerDisplacementType::Swap)
		? ESRPGWarriorAftermathStance::Tether
		: ESRPGWarriorAftermathStance::Trip;
	mWarriorAftermathTarget = Target;
	AddWarriorCombo(1);

	FText FinisherText;
	const int32 MoveDistance = FMath::Max(FMath::Abs(To.mX - From.mX), FMath::Abs(To.mY - From.mY));
	if (DisplacementType == ESRPGPlayerDisplacementType::Pull && mPlayerUnit != nullptr)
	{
		const FTileIndex PlayerTile = mPlayerUnit->GetTileTransform().mIndex;
		const int32 PlayerDistance = FMath::Max(FMath::Abs(To.mX - PlayerTile.mX), FMath::Abs(To.mY - PlayerTile.mY));
		if (PlayerDistance == 1)
		{
			ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(Target));
			BroadcastCollisionPresentation(mTileMap, mPlayerUnit, Target);
			FinisherText = NSLOCTEXT("WarriorFlow", "PullKnee", "상황 마무리 · 끌어온 적에게 무릎차기 (1 피해)");
			AddWarriorCombo(1);
		}
	}
	else if (DisplacementType == ESRPGPlayerDisplacementType::Throw && MoveDistance >= 2)
	{
		ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(Target));
		FinisherText = NSLOCTEXT("WarriorFlow", "ThrowSlam", "상황 마무리 · 착지 충격으로 내리꽂기 (1 피해)");
		AddWarriorCombo(1);
	}
	bool bFoundIntent = false;
	for (FSRPGEnemyIntent& Intent : mEnemyIntents)
	{
		if (Intent.mEnemy != Target
			|| (Intent.mResult != ESRPGEnemyIntentResult::Planned && Intent.mResult != ESRPGEnemyIntentResult::Executing))
		{
			continue;
		}

		Intent.mWasDisplaced = true;
		Intent.mHasPendingDisplacementResponse = true;
		Intent.mDisplacedFromTile = From;
		Intent.mDisplacedToTile = To;
		const int32 LandingPenalty = DisplacementType == ESRPGPlayerDisplacementType::Throw
			|| DisplacementType == ESRPGPlayerDisplacementType::Push
			? 1
			: 0;
		Intent.mRecoveryMovePenalty = FMath::Max(Intent.mRecoveryMovePenalty, LandingPenalty);
		Intent.mResultText = FText::Format(
			NSLOCTEXT(
				"EnemyIntent",
				"PlayerDisplacedEnemyPendingReplan",
				"{0}: 주사위 {1} · ({2},{3}) → ({4},{5}) · 새 위치 반영 중"),
			DisplacementLabel,
			FText::AsNumber(DiceValue),
			FText::AsNumber(From.mX),
			FText::AsNumber(From.mY),
			FText::AsNumber(To.mX),
			FText::AsNumber(To.mY));
		if (FinisherText.IsEmpty() == false)
		{
			Intent.mResultText = FText::Format(
				NSLOCTEXT("EnemyIntent", "ResultJoin", "{0}\n{1}"),
				Intent.mResultText,
				FinisherText);
		}
		bFoundIntent = true;
	}

	if (bFoundIntent && mTileMap != nullptr)
	{
		// 물리 이동이 끝난 프레임에 이전 출발점의 화살표를 즉시 지운다. 액션 종료 직후
		// ReplanEnemyIntentsAfterPlayerAction이 모든 적의 새 경로를 한 번에 다시 그린다.
		mTileMap->ClearEnemyIntentOverlays();
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
	// 벽꿍은 목표에게 한 번 더, 중량 적을 무기로 쓴 충돌은 막힌 대상에게 한 번 더 피해를 준다.
	if (Cast<UUnitModel>(Blocker) == nullptr)
	{
		ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(Target));
	}
	else if (const UEnemyUnitModel* ThrownEnemy = Cast<UEnemyUnitModel>(Target);
		ThrownEnemy != nullptr && ThrownEnemy->GetDisplacementWeight() == ESRPGDisplacementWeight::Heavy)
	{
		ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(Blocker));
	}
	BroadcastCollisionPresentation(mTileMap, Target, Blocker);
	AddWarriorCombo(2);
	// 충돌 지점을 다음 플레이어 차례까지 남긴다. 해당 적 곁으로 기술 착지를 잡으면
	// 무기를 다시 휘두를 공간을 회수했다는 의미로 FLOW가 즉시 최대가 된다.
	mExecutionOpportunityTile = Target->GetTileTransform().mIndex;
	mExecutionOpportunityExpireRound = mRoundCount + 1;
	const bool bEnemyBlocker = Cast<UUnitModel>(Blocker) != nullptr && Blocker != mPlayerUnit;
	if (FSRPGEnemyIntent* Intent = FindEnemyIntent(Target))
	{
		// 벽/유닛과 실제 충돌하면 단순 착지보다 한 단계 더 회복해야 한다.
		Intent->mRecoveryMovePenalty = FMath::Clamp(Intent->mRecoveryMovePenalty + 1, 0, 2);
		const FText ImpactLabel = DisplacementType == ESRPGPlayerDisplacementType::Swap
			? NSLOCTEXT("EnemyIntent", "SwapImpact", "자리 바꾸기")
			: (DisplacementType == ESRPGPlayerDisplacementType::Throw
			? NSLOCTEXT("EnemyIntent", "ThrowImpact", "던지기")
			: (DisplacementType == ESRPGPlayerDisplacementType::Pull
				? NSLOCTEXT("EnemyIntent", "PullImpact", "당기기")
				: NSLOCTEXT("EnemyIntent", "PushImpact", "밀기")));
		const FText Message = bEnemyBlocker
			? FText::Format(
				NSLOCTEXT("EnemyIntent", "PlayerChainCollision", "{0} 충돌! {1}과 부딪혀 양쪽 1 피해"),
				ImpactLabel,
				Blocker->GetBoardActorDisplayName())
			: FText::Format(
				NSLOCTEXT("EnemyIntent", "PlayerObstacleCollision", "{0} 충돌! {1} 앞에서 정지"),
				ImpactLabel,
				Blocker->GetBoardActorDisplayName());
		Intent->mResultText = FText::Format(
			NSLOCTEXT("EnemyIntent", "ResultJoin", "{0}\n{1}"),
			Intent->mResultText,
			Message);
		BroadcastEnemyIntentChanged();
		RefreshEnemyIntentHighlights();
	}
}

void USRPGCombatModel::ReportPlayerStagger(UUnitModel* Target, int32 DiceValue)
{
	FSRPGEnemyIntent* Intent = FindEnemyIntent(Target);
	if (Intent == nullptr || Intent->mResult != ESRPGEnemyIntentResult::Planned)
	{
		return;
	}
	mPendingWarriorAftermathStance = ESRPGWarriorAftermathStance::Trip;
	mWarriorAftermathTarget = Target;
	AddWarriorCombo(1);
	const int32 MovePenalty = FMath::Clamp(FMath::CeilToInt(FMath::Max(DiceValue, 1) / 3.0f), 1, 2);
	const TArray<FTileIndex> PreviousPath = Intent->mPathTileIndexes;
	const FTileIndex PreviousDestination = Intent->mPlannedDestination;
	Intent->mPlannedMoveRange = FMath::Max(Intent->mPlannedMoveRange - MovePenalty, 0);
	Intent->mResponseCostSpent += MovePenalty;
	Intent->mPreviousPathTileIndexes = PreviousPath;
	Intent->mPreviousDestination = PreviousDestination;
	RebuildEnemyIntentPlan(*Intent, /*bPreserveSkill*/true);
	++Intent->mPlanRevision;
	Intent->mResultText = FText::Format(
		NSLOCTEXT("EnemyIntent", "PlayerStaggered", "다리 걸기! 이동력 -{0} · 비틀거리며 새 계획"),
		FText::AsNumber(MovePenalty));
	BroadcastEnemyIntentChanged();
	RefreshEnemyIntentHighlights();
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
	BroadcastCollisionPresentation(mTileMap, Enemy, Blocker);
	AddWarriorCombo(Cast<UUnitModel>(Blocker) != nullptr && Blocker != mPlayerUnit ? 2 : 1);

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
		mWarriorMomentum = 0;
		mWarriorFlow = 0;
		mWarriorCombo = 0;
		AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::HitPlayer,
			NSLOCTEXT("EnemyIntent", "HitPlayer", "예정 좌표에 플레이어가 남아 공격 적중"));
		if (mStandstillPressure >= 2)
		{
			ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(mPlayerUnit));
			AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::HitPlayer,
				NSLOCTEXT("EnemyIntent", "StandstillPunished", "포위 협공! 같은 자리에 머문 탓에 추가 1 피해"));
		}
		if (const UEnemyUnitModel* ActingEnemy = Cast<UEnemyUnitModel>(Enemy))
		{
			switch (ActingEnemy->GetMovementRole())
			{
			case ESRPGEnemyMovementRole::Anchor:
				mPersistentMovementHazardTile = mPlayerUnit != nullptr
					? mPlayerUnit->GetTileTransform().mIndex
					: FTileIndex::Invalid;
				AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::HitPlayer,
					NSLOCTEXT("EnemyIntent", "AnchorSporeZone", "포자 지대 생성 · 다음 행동 전에 이 칸을 떠나야 함"));
				break;
			case ESRPGEnemyMovementRole::Flanker:
				mTetheringEnemy = Cast<UEnemyUnitModel>(Enemy);
				AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::HitPlayer,
					NSLOCTEXT("EnemyIntent", "FlankerTether", "거미줄 속박 · 다음 이동 기술로 끊지 않으면 1 피해"));
				break;
			case ESRPGEnemyMovementRole::Slider:
				mStandstillPressure = FMath::Clamp(mStandstillPressure + 1, 0, 3);
				AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::HitPlayer,
					NSLOCTEXT("EnemyIntent", "SliderPressure", "탄성 압박 · 제자리에 버틸 공간이 줄어듦"));
				break;
			case ESRPGEnemyMovementRole::Bulwark:
				mStandstillPressure = FMath::Clamp(mStandstillPressure + 1, 0, 3);
				AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::HitPlayer,
					NSLOCTEXT("EnemyIntent", "BulwarkPressure", "방패 압박 · 다음 기술의 착지 칸을 바꿔 포위를 벗어나야 함"));
				break;
			case ESRPGEnemyMovementRole::Lancer:
				mStandstillPressure = FMath::Clamp(mStandstillPressure + 1, 0, 3);
				AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::HitPlayer,
					NSLOCTEXT("EnemyIntent", "LancerPressure", "창 돌진 적중 · 제자리 압박 증가"));
				break;
			case ESRPGEnemyMovementRole::Bomber:
				mPersistentMovementHazardTile = Intent->mTargetTile;
				AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::HitPlayer,
					NSLOCTEXT("EnemyIntent", "BomberHazard", "화약 잔불 · 다음 기술로 이 칸을 떠나야 함"));
				break;
			case ESRPGEnemyMovementRole::Standard:
			default:
				break;
			}
		}
	}
	if (HitObstacle)
	{
		AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::HitObstacle,
			NSLOCTEXT("EnemyIntent", "HitObstacle", "공개된 공격선이 장애물에 적중"));
	}
	if (HitFriendly)
	{
		AddWarriorCombo(2);
		// 복합 명중이어도 HUD의 대표 배지는 가장 극적인 오사 결과를 유지한다.
		AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::FriendlyFire,
			NSLOCTEXT("EnemyIntent", "FriendlyFire", "아군 오사! 공개된 공격선에 다른 적이 맞음"));
	}
	if (HitPlayer == false && HitFriendly == false && HitObstacle == false)
	{
		AppendEnemyIntentResult(*Intent, ESRPGEnemyIntentResult::Missed,
			NSLOCTEXT("EnemyIntent", "AttackMissed", "공격 빗나감! 예정 타일이 비어 있음"));
	}
	if (const UEnemyUnitModel* ActingEnemy = Cast<UEnemyUnitModel>(Enemy);
		ActingEnemy != nullptr && ActingEnemy->GetMovementRole() == ESRPGEnemyMovementRole::Bomber
		&& Intent->mTargetTile != FTileIndex::Invalid)
	{
		int32 BlastVictims = 0;
		for (const TObjectPtr<UUnitModel>& Unit : mUnits)
		{
			if (Unit == nullptr || Unit == Enemy || Unit->IsDead())
			{
				continue;
			}
			const FTileIndex Tile = Unit->GetTileTransform().mIndex;
			if (FMath::Max(FMath::Abs(Tile.mX - Intent->mTargetTile.mX), FMath::Abs(Tile.mY - Intent->mTargetTile.mY)) <= 1)
			{
				ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(Unit));
				BroadcastCollisionPresentation(mTileMap, Enemy, Unit);
				++BlastVictims;
			}
		}
		AppendEnemyIntentResult(*Intent,
			BlastVictims > 0 ? ESRPGEnemyIntentResult::Collision : ESRPGEnemyIntentResult::Missed,
			FText::Format(
				NSLOCTEXT("EnemyIntent", "BomberBlastResult", "화약 폭발! 목표 주변 {0}명 충격 피해"),
				FText::AsNumber(BlastVictims)));
	}

	BroadcastEnemyIntentChanged();
	RefreshEnemyIntentHighlights();
}

void USRPGCombatModel::ReportEnemySkillDisplacement(
	UUnitModel* Enemy,
	UUnitModel* Target,
	const FTileIndex& From,
	const FTileIndex& To,
	const FText& SkillName,
	bool bApplyImpactDamage)
{
	if (Enemy == nullptr || Target == nullptr || From == To)
	{
		return;
	}

	if (bApplyImpactDamage)
	{
		ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(Target));
		BroadcastCollisionPresentation(mTileMap, Enemy, Target);
	}

	if (FSRPGEnemyIntent* Intent = FindEnemyIntent(Enemy))
	{
		const FText Message = FText::Format(
			bApplyImpactDamage
				? NSLOCTEXT("EnemyIntent", "EnemySkillImpactPush", "{0}! {1}을 ({2},{3})로 들이받아 밀침 (1 피해)")
				: NSLOCTEXT("EnemyIntent", "EnemySkillDisplacement", "{0}! {1}을 ({2},{3})로 강제 이동"),
			SkillName,
			Target->GetBoardActorDisplayName(),
			FText::AsNumber(To.mX),
			FText::AsNumber(To.mY));
		AppendEnemyIntentResult(*Intent, Intent->mResult, Message);
		BroadcastEnemyIntentChanged();
		RefreshEnemyIntentHighlights();
	}
}

void USRPGCombatModel::ReportEnemySkillCollision(
	UUnitModel* Enemy,
	UUnitModel* Target,
	UBoardActorModel* Blocker,
	const FText& SkillName)
{
	if (Enemy == nullptr || Target == nullptr || Blocker == nullptr)
	{
		return;
	}

	ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(Target));
	ApplyIntentCollisionDamage(Cast<IBoardCombatTarget>(Blocker));
	BroadcastCollisionPresentation(mTileMap, Target, Blocker);

	if (FSRPGEnemyIntent* Intent = FindEnemyIntent(Enemy))
	{
		const FText Message = FText::Format(
			NSLOCTEXT("EnemyIntent", "EnemySkillChainCollision", "{0} 충돌! {1}이 {2}에 부딪힘 (양쪽 1 피해)"),
			SkillName,
			Target->GetBoardActorDisplayName(),
			Blocker->GetBoardActorDisplayName());
		AppendEnemyIntentResult(*Intent, Intent->mResult, Message);
		BroadcastEnemyIntentChanged();
		RefreshEnemyIntentHighlights();
	}
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
	TArray<FTileIndex> MovementHazardTiles;
	if (mPersistentMovementHazardTile != FTileIndex::Invalid)
	{
		MovementHazardTiles.Add(mPersistentMovementHazardTile);
	}
	if (mExecutionOpportunityTile != FTileIndex::Invalid)
	{
		TargetTiles.AddUnique(mExecutionOpportunityTile);
	}
	TArray<FEnemyIntentTileOverlay> EnemyIntentOverlays;
	EnemyIntentOverlays.Reserve(mEnemyIntents.Num() + mPendingReinforcementTransforms.Num());
	for (const FSRPGEnemyIntent& Intent : mEnemyIntents)
	{
		const bool bIsResolved = Intent.mResult != ESRPGEnemyIntentResult::Planned
			&& Intent.mResult != ESRPGEnemyIntentResult::Executing;

		FEnemyIntentTileOverlay& Overlay = EnemyIntentOverlays.AddDefaulted_GetRef();
		Overlay.mExecutionOrder = Intent.mExecutionOrder;
		Overlay.mPathTileIndexes = Intent.mPathTileIndexes;
		Overlay.mPreviousPathTileIndexes = Intent.mPreviousPathTileIndexes;
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
	for (int32 Index = 0; Index < mPendingReinforcementTransforms.Num(); ++Index)
	{
		FEnemyIntentTileOverlay& Overlay = EnemyIntentOverlays.AddDefaulted_GetRef();
		Overlay.mExecutionOrder = mEnemyIntents.Num() + Index + 1;
		Overlay.mCurrentTile = mPendingReinforcementTransforms[Index].mIndex;
		Overlay.mIsReinforcementWarning = true;
	}

	// 전용 오버레이는 해결 상태도 유지한다. 빈 배열도 전달해 새 계획이 없을 때 이전 표시를 지운다.
	mTileMap->SetEnemyIntentOverlays(EnemyIntentOverlays);

	if (MovementHazardTiles.IsEmpty() == false)
	{
		mTileMap->SetTileHighlight(MovementHazardTiles, ETileHighlightFlag::Effect);
	}
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

