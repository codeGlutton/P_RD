#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"

#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "Simulation/Factory/ObjectModelFactory.h"

#include "SRPGFramework/SRPGCommand.h"
#include "SRPGFramework/SRPGTurnEndAction.h"
#include "SRPGFramework/SRPGFrameworkType.h"

#include "Actor/TileMap/TileMapModel.h"
#include "Pawn/Player/PlayerUnitModel.h"
#include "Pawn/Enemy/EnemyUnitModel.h"

#include "Actor/BoardActor/BoardCombatTarget.h"

#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"
#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"
#include "DataAsset/ObstacleSpawnData/StaticObstacleSpawnData.h"

#include "TimerManager.h"
#include "Setting/GameTeamType.h"

#include "FunctionLibrary/RandomStreamFunctionLibrary.h"
#include "Setting/GameBalanceSettings.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "TAS/Effect/Stat/TacticalEffect_SpeedPoint.h"

DEFINE_LOG_CATEGORY(LogSRPGCombat)

void USRPGCombatModel::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsSaving() == true)
	{
		auto* Node = mTurnContextOrders.GetHead();
		int32 TotalContextNum = mTurnContextOrders.Num();
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
			mTurnContextOrders.AddTail(TurnIndex);
		}
	}
}

void USRPGCombatModel::Tick(float DeltaTime)
{
	USRPGTurnContext* CurTurnContext = GetCurrentTurnContext();
	if (mCombatPhase == ESRPGCombatRoomPhase::CombatPlay && CurTurnContext != nullptr)
	{
		CurTurnContext->TickTurn(DeltaTime);
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

void USRPGCombatModel::InitCombat(UStaticCombatRoomSpawnData* RoomSpawnData, const TArray<TObjectPtr<UPlayerUnitModel>>& PlayerUnits, const FTransform& RoomStartTransform, const FRoomClearData& ClearData)
{
	checkf(RoomSpawnData != nullptr, TEXT("해당하는 룸 정보 탐색 실패"));
	checkf(mCombatPhase == ESRPGCombatRoomPhase::None, TEXT("중복 초기화"));
	mCombatPhase = ESRPGCombatRoomPhase::CombatInit;

	/* 모델들 등록 */

	RegisterTileMapModel(RoomStartTransform);
	if (ClearData.mIsCleared == true)
	{
		// 클리어 데이터로 유닛들 복구
		RestoreBoardActorModels(RoomSpawnData, PlayerUnits, ClearData);
	}
	else
	{
		// 액터들 초기 데이터로 등록
		InitBoardActorModels(RoomSpawnData, PlayerUnits);
		// 이벤트 초기 데이터로 등록
		RegisterRoundEvents(RoomSpawnData);
	}

	UE_LOG(LogSRPGCombat, Log, TEXT("SRPG 전투 초기화 완료"));
}

void USRPGCombatModel::BeginCombat()
{
	checkf(mCombatPhase == ESRPGCombatRoomPhase::CombatInit, TEXT("전투 시작 전 초기화 우선 필요"));
	mCombatPhase = ESRPGCombatRoomPhase::CombatStart;

	UE_LOG(LogSRPGCombat, Log, TEXT("SRPG 전투 시작"));

	/* 전투 시작 연출 */
	
	auto PresentationBarrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateWeakLambda(this, [this]() {
		
		/* 전투 시작 */

		for (TObjectPtr<UUnitModel>& Unit : mUnitModels)
		{
			Unit->OnBeginRoom();
		}
		for (TObjectPtr<UBoardActorModel>& Obstacle : mObstacleModels)
		{
			Obstacle->OnBeginRoom();
		}
		
		checkf(mCombatPhase == ESRPGCombatRoomPhase::CombatStart, TEXT("전투 진입 절차 오류"));
		mCombatPhase = ESRPGCombatRoomPhase::CombatPlay;

		if (mIsClearCombat == true)
		{
			// 이전 클리어 전적으로 전투 종료
			
			mCombatPhase = ESRPGCombatRoomPhase::CombatAbort;
			mCombatResult = ESRPGCombatResult::PlayerWin;
			EndCombat();
		}
		else
		{
			// 턴 시작

			mNextRoundRandomSeed = URandomStreamFunctionLibrary::GetEventStream(this).GetCurrentSeed();
			AdvanceTurn();
		}
		}));
	OnBeginCombatUI.Broadcast(PresentationBarrier);
}

void USRPGCombatModel::EndCombat()
{
	checkf(mCombatPhase == ESRPGCombatRoomPhase::CombatAbort, TEXT("전투 종료 절차 오류"));
	mCombatPhase = ESRPGCombatRoomPhase::CombatEnd;

	/* 전투 종료 */

	for (TObjectPtr<UUnitModel>& Unit : mUnitModels)
	{
		Unit->OnEndRoom();
	}
	for (TObjectPtr<UBoardActorModel>& Obstacle : mObstacleModels)
	{
		Obstacle->OnEndRoom();
	}

	mShouldTerminateBeforePlayerTurnStart = false;
	if (mCombatResult == ESRPGCombatResult::PlayerWin)
	{
		ClearAllCombatTargetModels();

		mIsClearCombat = true;
		OnSaveCombatPlay.Broadcast(mPlayerUnitModels, mRoundCount, mTurnCount);
	}

	/* 전투 종료 연출 */

	auto PresentationBarrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateWeakLambda(this, [this]() {
		
		/* 전투 종료 */

		OnShowCombatResultUI.Broadcast(mCombatResult);
		UE_LOG(LogSRPGCombat, Log, TEXT("SRPG 전투 종료"))
		}));
	OnEndCombatUI.Broadcast(PresentationBarrier, mCombatResult);
}

void USRPGCombatModel::InitBoardActorModels(UStaticCombatRoomSpawnData* RoomSpawnData, const TArray<TObjectPtr<UPlayerUnitModel>>& PlayerUnits)
{
	const int32 PlayerMaxNum = PlayerUnits.Num();

	/* 랜덤으로 위치 결정 */

	TArray<int32> TransformIndexes;
	TransformIndexes.Reserve(PlayerMaxNum);
	for (int32 PlayerIndex = 0; PlayerIndex < PlayerMaxNum; ++PlayerIndex)
	{
		TransformIndexes.Add(PlayerIndex);
	}
	const FRandomStream& RandomStream = URandomStreamFunctionLibrary::GetEventStream(this);
	URandomStreamFunctionLibrary::ShuffleArray(RandomStream, TransformIndexes);

	/* 플레이어 유닛 스폰 */

	for (int32 PlayerIndex = 0; PlayerIndex < PlayerMaxNum; ++PlayerIndex)
	{
		const TObjectPtr<UPlayerUnitModel>& PlayerUnit = PlayerUnits[PlayerIndex];
		if (PlayerUnit == nullptr)
		{
			continue;
		}

		const FTileTransform& PlayerTileTransform = RoomSpawnData->mPlayerTransforms[TransformIndexes[PlayerIndex]];
		RegisterPlayerUnitModel(PlayerUnit, PlayerTileTransform);
	}

	/* 적군 스폰 */

	for (FEnemyUnitPlacementData& EnemyUnitPlacementData : RoomSpawnData->mEnemyUnitPlacementDatas)
	{
		RegisterEnemyUnitModel(EnemyUnitPlacementData);
	}

	/* 장애물 스폰 */

	for (FObstaclePlacementData& ObstaclePlacementData : RoomSpawnData->mObstaclePlacementDatas)
	{
		RegisterObstacleModel(ObstaclePlacementData);
	}
}

void USRPGCombatModel::RegisterRoundEvents(UStaticCombatRoomSpawnData* RoomSpawnData)
{
	for (const TInstancedStruct<FSRPGCombatRoundEvent>& RoundStartEvent : RoomSpawnData->mRoundStartEvents)
	{
		AddRoundStartEvent(RoundStartEvent);
	}
	for (const TInstancedStruct<FSRPGCombatRoundEvent>& RoundEndEvent : RoomSpawnData->mRoundEndEvents)
	{
		AddRoundEndEvent(RoundEndEvent);
	}
}

void USRPGCombatModel::RestoreBoardActorModels(UStaticCombatRoomSpawnData* RoomSpawnData, const TArray<TObjectPtr<UPlayerUnitModel>>& PlayerUnits, const FRoomClearData& ClearData)
{
	mIsClearCombat = ClearData.mIsCleared;

	mRoundCount = ClearData.mRoundCount;
	mTurnCount = ClearData.mTurnCount;

	/* 플레이어 스폰 */

	const int32 PlayerMaxNum = PlayerUnits.Num();
	for (int32 PlayerIndex = 0; PlayerIndex < PlayerMaxNum; ++PlayerIndex)
	{
		const TObjectPtr<UPlayerUnitModel>& PlayerUnit = PlayerUnits[PlayerIndex];
		if (PlayerUnit == nullptr)
		{
			continue;
		}

		RegisterPlayerUnitModel(PlayerUnit, ClearData.mPlayerTileTransforms[PlayerIndex]);
	}

	/* 비전투 장애물만 스폰 */

	for (FObstaclePlacementData& ObstaclePlacementData : RoomSpawnData->mObstaclePlacementDatas)
	{
		UStaticObstacleSpawnData* ObstacleSpawnData = ObstaclePlacementData.mSpawnData.LoadSynchronous();
		checkf(ObstacleSpawnData != nullptr, TEXT("장애물 스폰 데이터 로드 실패"));

		if (ObstacleSpawnData->GetClass()->ImplementsInterface(UBoardCombatTarget::StaticClass()) == false)
		{
			RegisterObstacleModel(ObstaclePlacementData);
		}
	}
}

void USRPGCombatModel::AdvanceTurn()
{
	const bool IsFirstTurn = mTurnCount == 0;
	if (IsFirstTurn == false)
	{
		/* 턴 마무리 */

		UnregisterTurn(GetCurrentTurnContext(), false);
	}

	auto PresentationBarrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateWeakLambda(this, [this]() {

		/* 필요 시 전투 강제 중단 */

		const bool IsPlayerTurn = GetCurrentTurnContext()->GetOwner()->IsPlayerUnitModel() == true;
		if (mShouldTerminateBeforePlayerTurnStart == true && IsPlayerTurn == true)
		{
			mShouldTerminateBeforePlayerTurnStart = false;
			return;
		}

		/* 턴 준비 */

		++mTurnCount;
		UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
		checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));
		TacticalFrameworkModel->AdvanceTurnDuration(mTurnCount);

		/* 턴 시작 */

		GetCurrentTurnContext()->BeginTurn();
		}));

	/* 라운드 진행 */

	AdvanceRoundIfNeeded(PresentationBarrier);
}

void USRPGCombatModel::AdvanceRoundIfNeeded(TSharedPtr<FPresentationBarrier> RoundPresentationBarrier)
{
	/* 라운드 진행이 필요한지 체크 */

	if (HasAnyTurnContext() == true)
	{
		return;
	}

	/* 라운드 진행 */

	auto NewRoundBeginPresentationBarrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateWeakLambda(this, [this, RoundPresentationBarrier]() {
		
		/* 새로운 턴 채우기 */

		EvaluateRound();

		/* 새 라운드 실행 */

		BeginRound(RoundPresentationBarrier);
		}));

	const bool IsFirstRound = mRoundCount == 0;
	if (IsFirstRound == false)
	{
		EndRound(NewRoundBeginPresentationBarrier);
	}
}

void USRPGCombatModel::BeginRound(TSharedPtr<FPresentationBarrier> RoundPresentationBarrier)
{
	++mRoundCount;
	UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
	checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));
	TacticalFrameworkModel->AdvanceRoundDuration(mRoundCount);

	auto RoundStartPresentationBarrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateWeakLambda(this, [this, RoundPresentationBarrier]() {
		for (const TObjectPtr<UUnitModel>& Unit : mUnitModels)
		{
			Unit->OnBeginRound(mRoundCount);
		}
		for (const TObjectPtr<UBoardActorModel>& Obstacle : mObstacleModels)
		{
			Obstacle->OnBeginRound(mRoundCount);
		}

		OnBeginAnyRoundUI.Broadcast(RoundPresentationBarrier, mRoundCount);
		}));

	TriggerRoundEvents(RoundStartPresentationBarrier, mRoundStartEvents, 0);
}

void USRPGCombatModel::EndRound(TSharedPtr<FPresentationBarrier> RoundPresentationBarrier)
{
	for (const TObjectPtr<UUnitModel>& Unit : mUnitModels)
	{
		Unit->OnEndRound(mRoundCount);
	}
	for (const TObjectPtr<UBoardActorModel>& Obstacle : mObstacleModels)
	{
		Obstacle->OnEndRound(mRoundCount);
	}

	TriggerRoundEvents(RoundPresentationBarrier, mRoundEndEvents, 0);
}

void USRPGCombatModel::AddRoundStartEvent(TInstancedStruct<FSRPGCombatRoundEvent> Event)
{
	mRoundStartEvents.AddEvent(Event);
}

void USRPGCombatModel::AddRoundEndEvent(TInstancedStruct<FSRPGCombatRoundEvent> Event)
{
	mRoundEndEvents.AddEvent(Event);
}

FSRPGCombatRoundEvent* USRPGCombatModel::FindRoundStartEvent(const FName& EventName)
{
	return mRoundStartEvents.FindEvent(EventName);
}

const FSRPGCombatRoundEvent* USRPGCombatModel::FindRoundStartEvent(const FName& EventName) const
{
	return mRoundStartEvents.FindEvent(EventName);
}

FSRPGCombatRoundEvent* USRPGCombatModel::FindRoundEndEvent(const FName& EventName)
{
	return mRoundEndEvents.FindEvent(EventName);
}

const FSRPGCombatRoundEvent* USRPGCombatModel::FindRoundEndEvent(const FName& EventName) const
{
	return mRoundEndEvents.FindEvent(EventName);
}

void USRPGCombatModel::TriggerRoundEvents(TSharedPtr<FPresentationBarrier> RoundPresentationBarrier, FSRPGCombatRoundEventContainer& RoundEvents, int32 EventIndex)
{
	if (RoundEvents.IsValidIndex(EventIndex) == false)
	{
		return;
	}

	RoundEvents.Lock();
	auto RoundEventBarrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateLambda([this, RoundPresentationBarrier, &RoundEvents, EventIndex]() {
		if (RoundEvents[EventIndex].GetMutable().IsActivated() == false)
		{
			RoundEvents.RemoveAt(EventIndex);
		}
		TriggerRoundEvents(RoundPresentationBarrier, RoundEvents, EventIndex + 1);
		RoundEvents.Unlock();
		}));

	RoundEvents[EventIndex].GetMutable().TryToTrigger(RoundEventBarrier.ToSharedRef(), this);
}

void USRPGCombatModel::OnEndCurrentTurn(USRPGTurnContext* TurnContext, ESRPGTurnResult TurnResult)
{
	/* 전투 종료 체크 */

	EvaluateCombatStates();
	if (mCombatPhase == ESRPGCombatRoomPhase::CombatAbort)
	{
		EndCombat();
	}
	else
	{
		AdvanceTurn();
	}
}

void USRPGCombatModel::EvaluateCombatStates()
{
	ClearDeadActorModels();
	EvaluateCombatEndState();

	if (HasAnyTurnContext() == true)
	{
		const bool ForceAbort = mCombatPhase == ESRPGCombatRoomPhase::CombatAbort;
		GetCurrentTurnContext()->EvaluateTurnEndState(ForceAbort);
	}
}

void USRPGCombatModel::ClearDeadActorModels()
{
	TArray<TObjectPtr<UUnitModel>> DeadUnits;
	TArray<TScriptInterface<IBoardCombatTarget>> DeadObstacles;

	/* 대상 수집 */

	for (const TObjectPtr<UUnitModel>& Unit : mUnitModels)
	{
		if (Unit->IsDead() == true)
		{
			DeadUnits.Add(Unit);
		}
	}
	for (const TScriptInterface<IBoardCombatTarget>& Obstacle : mCombatTargetObstacleModels)
	{
		if (Obstacle->IsDead() == true)
		{
			DeadObstacles.Add(Obstacle);
		}
	}

	/* 제거 처리 */

	for (const TObjectPtr<UUnitModel>& DeadUnit : DeadUnits)
	{
		UnregisterUnitModel(DeadUnit);
	}
	for (const TScriptInterface<IBoardCombatTarget>& DeadObstacle : DeadObstacles)
	{
		UnregisterObstacleModel(Cast<UBoardActorModel>(DeadObstacle.GetObject()));
	}
}

void USRPGCombatModel::ClearAllCombatTargetModels(bool IgnorePlayers)
{
	TArray<TObjectPtr<UUnitModel>> TargetUnits;
	TArray<TScriptInterface<IBoardCombatTarget>> TargetObstacles = mCombatTargetObstacleModels;

	/* 대상 수집 */

	for (const TObjectPtr<UUnitModel>& Unit : mUnitModels)
	{
		if (IgnorePlayers == false || Unit->IsPlayerUnitModel() == false)
		{
			TargetUnits.Add(Unit);
		}
	}

	/* 제거 처리 */

	for (const TObjectPtr<UUnitModel>& TargetUnit : TargetUnits)
	{
		UnregisterUnitModel(TargetUnit);
	}
	for (const TScriptInterface<IBoardCombatTarget>& TargetObstacle : TargetObstacles)
	{
		UnregisterObstacleModel(Cast<UBoardActorModel>(TargetObstacle.GetObject()));
	}
}

void USRPGCombatModel::EvaluateCombatEndState()
{
	/* 이미 중단 */

	if (mCombatPhase == ESRPGCombatRoomPhase::CombatAbort)
	{
		return;
	}

	/* 플레이어가 죽어서 전투가 종료되는가? */

	bool AnyPlayerAlive = false;
	for (const TObjectPtr<UUnitModel>& PlayerUnit : mPlayerUnitModels)
	{
		if (PlayerUnit != nullptr && PlayerUnit->IsDead() == false)
		{
			AnyPlayerAlive = true;
			break;
		}
	}

	if (AnyPlayerAlive == false)
	{
		mCombatResult = ESRPGCombatResult::PlayerLose;
		mCombatPhase = ESRPGCombatRoomPhase::CombatAbort;
		return;
	}

	/* 적군이 모두 죽어 전투가 종료되는가? */

	bool AnyEnemyAlive = false;
	for (const TObjectPtr<UUnitModel>& Unit : mUnitModels)
	{
		if (FGenericTeamId::GetAttitude(EGameTeamType::Adventurer, Unit->GetGenericTeamId()) == ETeamAttitude::Hostile)
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

USRPGTurnContext* USRPGCombatModel::RegisterTurn(UUnitModel* Owner)
{
	/* 턴 생성 */

	USRPGTurnContext* TurnContext = NewObject<USRPGTurnContext>(this);
	TurnContext->InitTurn(this, Owner, mTurnContextMaxIndex++);
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

	/* 턴 등록 */

	mTurnContextOrders.AddTail(TurnContext->GetTurnId());
	mTurnContextMap.Add(TurnContext->GetTurnId(), TurnContext);

	return TurnContext;
}

bool USRPGCombatModel::UnregisterTurn(USRPGTurnContext* TurnContext, bool IgnoreCurTurn)
{
	checkf(TurnContext != nullptr, TEXT("존재하지 않는 턴 제거 요청"));

	return UnregisterTurn(TurnContext->GetOwner(), IgnoreCurTurn);
}

bool USRPGCombatModel::UnregisterTurn(UUnitModel* Owner, bool IgnoreCurTurn)
{
	checkf(Owner != nullptr, TEXT("존재하지 않는 유저의 턴 제거 요청"));

	/* 턴 탐색 */

	if (HasAnyTurnContext() == false)
	{
		return false;
	}

	auto* FoundNode = mTurnContextOrders.GetHead();
	if (IgnoreCurTurn == true)
	{
		FoundNode = FoundNode->GetNextNode();
	}

	while (FoundNode != nullptr)
	{
		USRPGTurnContext* FoundTurnContext = mTurnContextMap[FoundNode->GetValue()];
		if (FoundTurnContext->GetOwner() == Owner)
		{
			FoundTurnContext->OnBeginTurnUI.RemoveAll(this);
			FoundTurnContext->OnEndTurnUI.RemoveAll(this);
			FoundTurnContext->OnBeginAnyActionUI.RemoveAll(this);
			FoundTurnContext->OnEndAnyActionUI.RemoveAll(this);
			break;
		}
		FoundNode = FoundNode->GetNextNode();
	}

	if (FoundNode == nullptr)
	{
		return false;
	}

	/* 턴 제거 */

	mTurnContextMap.Remove(FoundNode->GetValue());
	mTurnContextOrders.RemoveNode(FoundNode);

	return true;
}

void USRPGCombatModel::EvaluateRound()
{
	while (true)
	{
		/* 스피드 포인트 충전 */

		for (const TObjectPtr<UUnitModel>& Unit : mUnitModels)
		{
			Unit->OnPreEvaluateRound();
		}
		for (const TObjectPtr<UBoardActorModel>& Obstacle : mObstacleModels)
		{
			Obstacle->OnPreEvaluateRound();
		}

		/* 현 상태 평가 */

		TArray<FSRPGTurnCandidate> Candidates;
		int32 NextRoundRandomSeed = INDEX_NONE;

		const bool IsValid = CheckOrderedTurnCandidates(OUT Candidates, OUT NextRoundRandomSeed);
		checkf(IsValid == true, TEXT("라운드에 관계없이 영구적으로 턴이 생성될 수 없음"));

		/* 유효 라운드 발견 */

		if (Candidates.IsEmpty() == false)
		{
			ApplyOrderedTurnCandidates(Candidates, NextRoundRandomSeed);
			break;
		}
	}
}

bool USRPGCombatModel::CheckOrderedTurnCandidates(OUT TArray<FSRPGTurnCandidate>& Candidates, OUT int32& NextRoundRandomSeed) const
{
	const UGameBalanceSettings* GameBalanceSettings = GetDefault<UGameBalanceSettings>();
	checkf(GameBalanceSettings != nullptr, TEXT("게임 밸런스 세팅 nullptr"));

	Candidates.Empty();
	NextRoundRandomSeed = INDEX_NONE;

	/* 현재 스피드 수집 */

	TArray<FSRPGTurnCandidate> CurTurnCandidates;
	for (const TObjectPtr<UUnitModel>& UnitModel : mUnitModels)
	{
		const int32 SpeedPoint = StaticCast<int32>(FMath::Floor(
			UnitModel->GetAttributeComponentModel()->GetAttributeCurrentValue(UUnitAttributeSet::GetSpeedPointAttribute())
		));

		FSRPGTurnCandidate CurTurnCandidate;
		CurTurnCandidate.mOwner = UnitModel;
		CurTurnCandidate.mRemainSpeedPoint = SpeedPoint;
		CurTurnCandidate.mRechargedSpeedPoint = StaticCast<int32>(FMath::Floor(
			UnitModel->GetAttributeComponentModel()->GetAttributeCurrentValue(UUnitAttributeSet::GetLastRechargedSpeedPointAttribute())
		));

#if !UE_BUILD_SHIPPING
		/* 영구 라운드 진행 방지 체크 */

		if (CurTurnCandidate.mRechargedSpeedPoint <= 0)
		{
			return false;
		}
#endif

		CurTurnCandidates.Add(CurTurnCandidate);
	}

	/* 후보 추가 */

	const FRandomStream RandomStream(mNextRoundRandomSeed);
	for (FSRPGTurnCandidate& CurTurnCandidate : CurTurnCandidates)
	{
		if (CurTurnCandidate.mRemainSpeedPoint >= GameBalanceSettings->mRequiredSpeedPointForTurn)
		{
			FSRPGTurnCandidate FixedTurnCandidate = CurTurnCandidate;
			FixedTurnCandidate.mRemainSpeedPoint -= GameBalanceSettings->mRequiredSpeedPointForTurn;
			FixedTurnCandidate.mRandomTieBreaker = RandomStream.FRand();

			Candidates.Add(FixedTurnCandidate);
		}
	}

	Candidates.Sort(TGreater<FSRPGTurnCandidate>());
	NextRoundRandomSeed = RandomStream.GetCurrentSeed();

	return true;
}

bool USRPGCombatModel::PredictOrderedTurnCandidatesOnNewRound(int32 RoundOffset, OUT TArray<FSRPGTurnCandidate>& Candidates, OUT int32& NextRoundRandomSeed) const
{
	if (RoundOffset < 0)
	{
		return false;
	}

	const UGameBalanceSettings* GameBalanceSettings = GetDefault<UGameBalanceSettings>();
	checkf(GameBalanceSettings != nullptr, TEXT("게임 밸런스 세팅 nullptr"));

	Candidates.Empty();
	NextRoundRandomSeed = INDEX_NONE;

	/* 현재 스피드 수집 */

	TArray<FSRPGTurnCandidate> CurTurnCandidates;
	for (const TObjectPtr<UUnitModel>& UnitModel : mUnitModels)
	{
		const int32 SpeedPoint = StaticCast<int32>(FMath::Floor(
			UnitModel->GetAttributeComponentModel()->GetAttributeCurrentValue(UUnitAttributeSet::GetSpeedPointAttribute())
		));

		FSRPGTurnCandidate CurTurnCandidate;
		CurTurnCandidate.mOwner = UnitModel;
		CurTurnCandidate.mRemainSpeedPoint = SpeedPoint;
		CurTurnCandidate.mRechargedSpeedPoint = StaticCast<int32>(FMath::Floor(
			UnitModel->GetAttributeComponentModel()->GetAttributeCurrentValue(UUnitAttributeSet::GetLastRechargedSpeedPointAttribute())
		));

#if !UE_BUILD_SHIPPING
		/* 영구 라운드 진행 방지 체크 */

		if (CurTurnCandidate.mRechargedSpeedPoint <= 0)
		{
			return false;
		}
#endif

		CurTurnCandidates.Add(CurTurnCandidate);
	}

	/* 오프셋 만큼 턴 진행 */

	int32 RoundIndex = -1;
	while (true)
	{
		/* 한 라운드 진행 체크 */

		bool IsValidRound = false;
		for (FSRPGTurnCandidate& CurTurnCandidate : CurTurnCandidates)
		{
			if (CurTurnCandidate.mRemainSpeedPoint >= GameBalanceSettings->mRequiredSpeedPointForTurn)
			{
				IsValidRound = true;
				break;
			}
		}

		/* 라운드 발생 감지 */

		if (IsValidRound == true)
		{
			++RoundIndex;
			if (RoundIndex == RoundOffset)
			{
				break;
			}
		}

		/* 라운드 충전 시뮬 */

		for (FSRPGTurnCandidate& CurTurnCandidate : CurTurnCandidates)
		{
			if (CurTurnCandidate.mRemainSpeedPoint >= GameBalanceSettings->mRequiredSpeedPointForTurn)
			{
				CurTurnCandidate.mRemainSpeedPoint -= GameBalanceSettings->mRequiredSpeedPointForTurn;
			}

			CurTurnCandidate.mRemainSpeedPoint += CurTurnCandidate.mRechargedSpeedPoint;
		}
	}

	/* 후보 추가 */

	const FRandomStream RandomStream(mNextRoundRandomSeed);
	for (FSRPGTurnCandidate& CurTurnCandidate : CurTurnCandidates)
	{
		if (CurTurnCandidate.mRemainSpeedPoint >= GameBalanceSettings->mRequiredSpeedPointForTurn)
		{
			/* 후보 추가 */

			FSRPGTurnCandidate FixedTurnCandidate = CurTurnCandidate;
			FixedTurnCandidate.mRemainSpeedPoint -= GameBalanceSettings->mRequiredSpeedPointForTurn;
			FixedTurnCandidate.mRandomTieBreaker = RandomStream.FRand();

			Candidates.Add(FixedTurnCandidate);
		}
	}

	Candidates.Sort(TGreater<FSRPGTurnCandidate>());
	NextRoundRandomSeed = RandomStream.GetCurrentSeed();

	return true;
}

void USRPGCombatModel::ApplyOrderedTurnCandidates(const TArray<FSRPGTurnCandidate>& Candidates, int32 NextRoundRandomSeed)
{
	const UGameBalanceSettings* GameBalanceSettings = GetDefault<UGameBalanceSettings>();
	checkf(GameBalanceSettings != nullptr, TEXT("게임 밸런스 세팅 nullptr"));

	mNextRoundRandomSeed = NextRoundRandomSeed;

	for (const FSRPGTurnCandidate& Candidate : Candidates)
	{
		/* 턴 등록 */

		checkf(Candidate.mOwner != nullptr, TEXT("새로운 턴 오너 nullptr"));
		RegisterTurn(Candidate.mOwner);

		/* 스피드 소모 */

		UAttributeSetComponentModel* AttributeSetCompModel = Candidate.mOwner->GetAttributeComponentModel();
		checkf(AttributeSetCompModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

		UTacticalEffectContext* EffectContext = AttributeSetCompModel->MakeEffectContext();
		TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetCompModel->MakeOutgoingSpec(UTacticalEffect_SpeedPoint::StaticClass(), EffectContext);
		EffectSpec->mDynamicMagnitude = -GameBalanceSettings->mRequiredSpeedPointForTurn;
		AttributeSetCompModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
	}
}

void USRPGCombatModel::RegisterTileMapModel(const FTransform& RoomStartTransform)
{
	checkf(mTileMapModel == nullptr, TEXT("이미 타일맵 존재"));

	/* 스폰 */

	UTileMapModel* TileMapModel = SpawnTileMapModel(RoomStartTransform);
	checkf(TileMapModel != nullptr, TEXT("타일맵 스폰 실패"));

	mTileMapModel = TileMapModel;
}

void USRPGCombatModel::RegisterPlayerUnitModel(UUnitModel* PlayerUnitModel, const FTileTransform& Transform)
{
	checkf(mTileMapModel != nullptr, TEXT("타일맵 미존재"));
	checkf(PlayerUnitModel != nullptr, TEXT("플레이어 유닛 모델 nullptr"));

	/* 등록 */

	mUnitModels.Push(PlayerUnitModel);
	mPlayerUnitModels.Push(PlayerUnitModel);

	/* 배치 */

	PlaceBoardActorModel(PlayerUnitModel, Transform);
	OnRegisterUnitUI.Broadcast(PlayerUnitModel);
}

void USRPGCombatModel::RegisterEnemyUnitModel(FEnemyUnitPlacementData& EnemyPlacementData)
{
	checkf(mTileMapModel != nullptr, TEXT("타일맵 미존재"));

	UStaticEnemyUnitSpawnData* EnemyUnitSpawnData = EnemyPlacementData.mSpawnData.LoadSynchronous();
	checkf(EnemyUnitSpawnData != nullptr, TEXT("적 유닛 스폰 데이터 로드 실패"));

	/* 스폰 */

	UEnemyUnitModel* EnemyUnitModel = Cast<UEnemyUnitModel>(SpawnBoardActorModel(EnemyUnitSpawnData, EnemyPlacementData.mTransform));
	checkf(EnemyUnitModel != nullptr, TEXT("보드액터 스폰 실패"));

	EnemyUnitModel->SetDifficulty(EnemyPlacementData.mDifficulty);

	if (EnemyPlacementData.mDefaultSpeedPoint > 0)
	{
		UTacticalEffectContext* Context = EnemyUnitModel->GetAttributeComponentModel()->MakeEffectContext();
		TSharedPtr<FTacticalEffectSpec> Spec = EnemyUnitModel->GetAttributeComponentModel()->MakeOutgoingSpec(UTacticalEffect_SpeedPoint::StaticClass(), Context);
		Spec->mDynamicMagnitude = StaticCast<float>(EnemyPlacementData.mDefaultSpeedPoint);

		EnemyUnitModel->GetAttributeComponentModel()->ApplyTacticalEffectSpecToSelf(*Spec);
	}

	/* 등록 */

	mUnitModels.Push(EnemyUnitModel);

	/* 배치 */

	PlaceBoardActorModel(EnemyUnitModel, EnemyPlacementData.mTransform);
	OnRegisterUnitUI.Broadcast(EnemyUnitModel);
}

void USRPGCombatModel::RegisterObstacleModel(FObstaclePlacementData& ObstaclePlacementData)
{
	checkf(mTileMapModel != nullptr, TEXT("타일맵 미존재"));

	UStaticObstacleSpawnData* ObstacleSpawnData = ObstaclePlacementData.mSpawnData.LoadSynchronous();
	checkf(ObstacleSpawnData != nullptr, TEXT("장애물 스폰 데이터 로드 실패"));

	/* 스폰 */

	UBoardActorModel* ObstacleModel = SpawnBoardActorModel(ObstacleSpawnData, ObstaclePlacementData.mTransform);
	checkf(ObstacleModel != nullptr, TEXT("보드액터 스폰 실패"));

	/* 등록 */

	UClass* BoardActorModelClass = ObstacleSpawnData->mModelClass.LoadSynchronous();
	checkf(BoardActorModelClass != nullptr, TEXT("보드 액터 Class 문제로 Model 생성 실패"));

	mObstacleModels.Push(ObstacleModel);
	if (BoardActorModelClass->ImplementsInterface(UBoardCombatTarget::StaticClass()) == true)
	{
		mCombatTargetObstacleModels.Push(ObstacleModel);
	}

	/* 배치 */

	PlaceBoardActorModel(ObstacleModel, ObstaclePlacementData.mTransform);
	OnRegisterObstacleUI.Broadcast(ObstacleModel);
}

void USRPGCombatModel::UnregisterUnitModel(UUnitModel* UnitModel)
{
	checkf(mTileMapModel != nullptr, TEXT("타일맵 미존재"));

	/* 턴 제거 */

	UnregisterTurn(UnitModel);

	/* 제거 */

	RemoveBoardActorModel(UnitModel);

	/* 해제 */

	if (UnitModel->IsPlayerUnitModel() == true)
	{
		mPlayerUnitModels.RemoveSingleSwap(UnitModel);
	}
	mUnitModels.RemoveSingleSwap(UnitModel);

	/* 파괴 */

	DestroyBoardActorModel(UnitModel);
	OnUnregisterUnitUI.Broadcast(UnitModel);
}

void USRPGCombatModel::UnregisterObstacleModel(UBoardActorModel* ObstacleModel)
{
	checkf(mTileMapModel != nullptr, TEXT("타일맵 미존재"));

	/* 제거 */

	RemoveBoardActorModel(ObstacleModel);

	/* 해제 */

	if (ObstacleModel->GetClass()->ImplementsInterface(UBoardCombatTarget::StaticClass()) == true)
	{
		mCombatTargetObstacleModels.RemoveSingleSwap(ObstacleModel);
	}
	mObstacleModels.RemoveSingleSwap(ObstacleModel);

	/* 파괴 */

	DestroyBoardActorModel(ObstacleModel);
	OnUnregisterObstacleUI.Broadcast(ObstacleModel);
}

UTileMapModel* USRPGCombatModel::SpawnTileMapModel(const FTransform& RoomStartTransform)
{
	checkf(mTileMapModel == nullptr, TEXT("이미 타일 존재"));

	// 타일맵 스폰
	mTileMapModel = GetWorldModelFactory(this)->NewModel<UTileMapModel>(RoomStartTransform);
	// 모델이 자기 타일 저장소(mTiles)를 직접 빌드
	mTileMapModel->RebuildTiles();

	return mTileMapModel;
}

UBoardActorModel* USRPGCombatModel::SpawnBoardActorModel(UStaticObstacleSpawnData* SpawnData, const FTileTransform& TileTransform)
{
	checkf(SpawnData != nullptr, TEXT("스폰 데이터 nullptr 오류"));
	UClass* BoardActorModelClass = SpawnData->mModelClass.LoadSynchronous();
	checkf(BoardActorModelClass != nullptr, TEXT("보드 액터 Class 문제로 Model 생성 실패"));

	UBoardActorModel* BoardActorModel = GetWorldModelFactory(this)->NewModelDeferred<UBoardActorModel>(BoardActorModelClass);
	BoardActorModel->SetStaticSpawnData(SpawnData);
	BoardActorModel->FinishCreating(mTileMapModel->TileToWorldTransform(TileTransform));

	return BoardActorModel;
}

void USRPGCombatModel::DestroyBoardActorModel(UBoardActorModel* Model)
{
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(OUT Handle, FTimerDelegate::CreateWeakLambda(Model, [Model]() {
		Model->Destroy();
		}), BOARD_ACTOR_DESTROY_DELAY_TIME, false);
}

void USRPGCombatModel::PlaceBoardActorModel(UBoardActorModel* Model, const FTileTransform& TileTransform)
{
	checkf(mTileMapModel->CanPlace(TileTransform.mIndex, Model) == true, TEXT("액터 배치 불가능"));

	mTileMapModel->PlaceActor(TileTransform, Model);
	if (mCombatPhase == ESRPGCombatRoomPhase::CombatPlay)
	{
		Model->OnBeginRoom();
	}
}

void USRPGCombatModel::RemoveBoardActorModel(UBoardActorModel* Model)
{
	if (mCombatPhase == ESRPGCombatRoomPhase::CombatPlay)
	{
		Model->OnEndRoom();
	}
	mTileMapModel->RemoveActor(Model);
}

bool USRPGCombatModel::PushAction(USRPGAction* Action)
{
	checkf(Action != nullptr, TEXT("유효하지 않은 액션"));

	USRPGTurnContext* CurTurnContext = GetCurrentTurnContext();
	if (CurTurnContext == nullptr)
	{
		UE_LOG(LogSRPGCombat, Log, TEXT("현재 등록된 턴 객체가 존재하지 않음"));
		return false;
	}

	CurTurnContext->EnqueueAction(Action);
	return true;
}

void USRPGCombatModel::ForcedAdvanceUntilNextAction(TInstancedStruct<FSRPGCommand> NextCommand, bool NeedEndCurrentAction)
{
	checkf(HasAnyTurnContext() == true, TEXT("현재 진행 중인 턴이 없음"));
	USRPGTurnContext* CurTurnContext = GetCurrentTurnContext();

	if (NeedEndCurrentAction == true)
	{
		CurTurnContext->ForcedClearActions();
	}
	CurTurnContext->ForcedAdvanceUntilNextAction(MoveTemp(NextCommand));
}

void USRPGCombatModel::ForcedAdvanceUntilNextPlayerTurn(bool NeedEndCurrentAction)
{
	checkf(HasAnyTurnContext() == true, TEXT("현재 진행 중인 턴이 없음"));
	USRPGTurnContext* CurTurnContext = GetCurrentTurnContext();

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

bool USRPGCombatModel::HasAnyTurnContext() const
{
	return mTurnContextOrders.IsEmpty() == false;
}

int32 USRPGCombatModel::GetTurnContextCount() const
{
	return mTurnContextOrders.Num();
}

USRPGTurnContext* USRPGCombatModel::GetCurrentTurnContext() const
{
	if (HasAnyTurnContext() == false)
	{
		return nullptr;
	}
	return mTurnContextMap[mTurnContextOrders.GetHead()->GetValue()];
}

USRPGTurnContext* USRPGCombatModel::GetTurnContext(const UUnitModel* Owner) const
{
	for (const int32 TurnId : mTurnContextOrders)
	{
		USRPGTurnContext* FoundTurnContext = mTurnContextMap[TurnId];
		if (FoundTurnContext->GetOwner() == Owner)
		{
			return FoundTurnContext;
		}
	}
	return nullptr;
}

TArray<TObjectPtr<USRPGTurnContext>> USRPGCombatModel::GetOrderedTurnContexts() const
{
	TArray<TObjectPtr<USRPGTurnContext>> Contexts;
	Contexts.Reserve(mTurnContextOrders.Num());

	for (const int32 TurnId : mTurnContextOrders)
	{
		Contexts.Push(mTurnContextMap[TurnId]);
	}

	return Contexts;
}

TArray<FSRPGTurnCandidate> USRPGCombatModel::GetOrderedTurnCandidates(uint32 RoundOffset) const
{
	TArray<FSRPGTurnCandidate> Candidates;
	int32 NextRoundRandomSeed = INDEX_NONE;

	PredictOrderedTurnCandidatesOnNewRound(RoundOffset, OUT Candidates, OUT NextRoundRandomSeed);

	return Candidates;
}

UTileMapModel* USRPGCombatModel::GetTileMap() const
{
	return mTileMapModel;
}

const TArray<TObjectPtr<UUnitModel>>& USRPGCombatModel::GetPlayerUnits() const
{
	return mPlayerUnitModels;
}

const TArray<TObjectPtr<UUnitModel>>& USRPGCombatModel::GetUnits() const
{
	return mUnitModels;
}

const TArray<TObjectPtr<UBoardActorModel>>& USRPGCombatModel::GetObstacles() const
{
	return mObstacleModels;
}

int32 USRPGCombatModel::GetRoundCount() const
{
	return mRoundCount;
}

int32 USRPGCombatModel::GetTurnCount() const
{
	return mTurnCount;
}

