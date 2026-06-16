#include "Combat/CombatViewAdapter.h"

#include "Actor/TileMap/TileMap.h"
#include "GameFramework/PlayerController.h"
#include "Pawn/Unit.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "UI/Combat/CombatViewModel.h"
#include "UI/Combat/CombatViewTypes.h"

namespace
{
	// 스킬 레일 index (UI와 합의): BASIC=0, STEP=5.
	constexpr int32 SkillIndexBasic = 0;
	constexpr int32 SkillIndexStep = 5;

	// 플레이스홀더 시작값(맞추면 됨). 실제 값은 UUnitData 연결 시 교체.
	constexpr float PlayerStartHP = 100.0f;
	constexpr float EnemyStartHP = 30.0f;
	constexpr int32 PlayerStartMovePoint = 0;

	// 가상 적 시작 타일(9x9 보드, 플레이어 (0,0) 코너 기준 유효 좌표).
	const FTileIndex VirtualEnemyTiles[] = {
		FTileIndex(4, 4),
		FTileIndex(6, 6),
		FTileIndex(2, 5),
	};
}

void UCombatViewAdapter::Build(USRPGCombatSubsystem* InCombat, const URunPersistData* InRun)
{
	mCombat = InCombat;
	mPlayerLevel = InRun != nullptr ? InRun->GetPlayerLevel() : 1;
	mUnitStates.Reset();
	mNextUnitId = 0;

	// 플레이어: 실제 스폰된 유닛에서 타일/액터를 가져온다(HP는 플레이스홀더).
	if (mCombat != nullptr)
	{
		for (const TObjectPtr<AUnit>& Unit : mCombat->GetUnits())
		{
			if (Unit == nullptr || Unit->IsPlayerUnit() == false)
			{
				continue;
			}
			FCombatUnitState State;
			State.mId = mNextUnitId++;
			State.mIsPlayer = true;
			State.mTile = Unit->GetTileTransform().mIndex;
			State.mMaxHP = PlayerStartHP;
			State.mHP = PlayerStartHP;
			State.mMovePoint = PlayerStartMovePoint;
			State.mActor = Unit;
			mUnitStates.Add(State);
		}
	}

	// 적: 액터 스폰 대신 가상 유닛으로 타일 위에 둔다(스폰 크래시 회피).
	for (const FTileIndex& Tile : VirtualEnemyTiles)
	{
		FCombatUnitState State;
		State.mId = mNextUnitId++;
		State.mIsPlayer = false;
		State.mTile = Tile;
		State.mMaxHP = EnemyStartHP;
		State.mHP = EnemyStartHP;
		mUnitStates.Add(State);
	}

	PushAll();
}

void UCombatViewAdapter::BindViewModel(UCombatViewModel* InViewModel)
{
	if (mViewModel != nullptr)
	{
		mViewModel->OnCombatCommand.RemoveDynamic(this, &UCombatViewAdapter::HandleCombatCommand);
		mViewModel->OnCombatWorldTouch.RemoveDynamic(this, &UCombatViewAdapter::HandleWorldTouch);
	}

	mViewModel = InViewModel;

	if (mViewModel != nullptr)
	{
		mViewModel->OnCombatCommand.AddUniqueDynamic(this, &UCombatViewAdapter::HandleCombatCommand);
		mViewModel->OnCombatWorldTouch.AddUniqueDynamic(this, &UCombatViewAdapter::HandleWorldTouch);
	}

	PushAll();
}

void UCombatViewAdapter::HandleCombatCommand(ECombatInputType Type, int32 IntPayload)
{
	switch (Type)
	{
	case ECombatInputType::SelectSkill:
		mSelectedSkillIndex = IntPayload;
		mPendingAttackDamage = -1;
		mMovePending = false;
		break;

	case ECombatInputType::ToggleDice:
	{
		const int32 DiceValue = GetRolledDiceValue(IntPayload);
		if (DiceValue <= 0)
		{
			break;
		}
		if (mSelectedSkillIndex == SkillIndexStep)
		{
			// STEP: 주사위 배치 → 본인 타일을 회색(Aim)으로. 탭마다 노랑→빨강(확정).
			mPendingStepValue = DiceValue;
			mStepStage = 0;
			mStepTile = GetPlayerTile();
			SetSingleTileHighlight(mStepTile, ETileHighlightFlag::Aim);
		}
		else if (mSelectedSkillIndex == SkillIndexBasic)
		{
			// BASIC: 주사위 배치 → 적 탭을 기다린다.
			mPendingAttackDamage = DiceValue;
		}
		break;
	}

	case ECombatInputType::Move:
		// MOVE 모드: 타일 탭으로 이동.
		mMovePending = true;
		mSelectedSkillIndex = INDEX_NONE;
		mPendingAttackDamage = -1;
		break;

	case ECombatInputType::Cancel:
		ClearPendingAction();
		break;

	default:
		break;
	}
}

void UCombatViewAdapter::HandleWorldTouch(FVector2D ScreenPosition, bool bLongPress)
{
	FTileIndex Tile;
	if (ResolveTileFromScreen(ScreenPosition, Tile) == false)
	{
		// 타일맵 밖 탭 = 진행 중 스킬/이동 취소.
		ClearPendingAction();
		return;
	}

	// STEP 단계 확정: 본인 타일을 탭할 때마다 회색→노랑→빨강(확정).
	if (mPendingStepValue >= 0)
	{
		if (Tile.mX == mStepTile.mX && Tile.mY == mStepTile.mY)
		{
			++mStepStage;
			if (mStepStage == 1)
			{
				SetSingleTileHighlight(mStepTile, ETileHighlightFlag::Select);   // 노랑
			}
			else if (mStepStage >= 2)
			{
				SetSingleTileHighlight(mStepTile, ETileHighlightFlag::Effect);   // 빨강 = 확정
				ApplyStep(mPendingStepValue);                                    // 이동력 += 주사위값
				ClearPendingAction();
			}
		}
		else
		{
			// 다른 곳 탭 = 취소.
			ClearPendingAction();
		}
		return;
	}

	// BASIC 평타: 적이 있는 타일을 탭했으면 데미지.
	if (mPendingAttackDamage >= 0)
	{
		const int32 TargetId = FindUnitIdAtTile(Tile);
		FCombatUnitState* Target = FindStateById(TargetId);
		if (Target != nullptr && Target->mIsPlayer == false)
		{
			ApplyBasicAttack(TargetId, mPendingAttackDamage);
		}
		ClearPendingAction();
		return;
	}

	// MOVE: 빈 타일로 이동(이동력 소모). 남으면 모드 유지.
	if (mMovePending)
	{
		TryMovePlayer(Tile);
		if (GetPlayerMovePoint() <= 0)
		{
			ClearPendingAction();
		}
		return;
	}
}

void UCombatViewAdapter::SetSingleTileHighlight(const FTileIndex& Tile, ETileHighlightFlag Flag) const
{
	ATileMap* TileMap = mCombat != nullptr ? mCombat->GetTileMap() : nullptr;
	if (TileMap == nullptr)
	{
		return;
	}
	// 세 상태 모두 끄고 원하는 한 가지만 켠다(단계 전환).
	TileMap->ClearTileHighlight(ETileHighlightFlag::Aim);
	TileMap->ClearTileHighlight(ETileHighlightFlag::Select);
	TileMap->ClearTileHighlight(ETileHighlightFlag::Effect);
	TArray<FTileIndex> Tiles;
	Tiles.Add(Tile);
	TileMap->SetTileHighlight(Tiles, Flag);
}

void UCombatViewAdapter::ClearAllHighlight() const
{
	ATileMap* TileMap = mCombat != nullptr ? mCombat->GetTileMap() : nullptr;
	if (TileMap == nullptr)
	{
		return;
	}
	TileMap->ClearTileHighlight(ETileHighlightFlag::Aim);
	TileMap->ClearTileHighlight(ETileHighlightFlag::Select);
	TileMap->ClearTileHighlight(ETileHighlightFlag::Effect);
}

int32 UCombatViewAdapter::GetRolledDiceValue(int32 DiceIndex) const
{
	if (mViewModel == nullptr)
	{
		return 0;
	}
	const TArray<FDiceSlotView>& Dice = mViewModel->GetDiceViews();
	if (Dice.IsValidIndex(DiceIndex) == false)
	{
		return 0;
	}
	return Dice[DiceIndex].mIsRolled ? Dice[DiceIndex].mResultValue : 0;
}

void UCombatViewAdapter::ClearPendingAction()
{
	mSelectedSkillIndex = INDEX_NONE;
	mPendingAttackDamage = -1;
	mMovePending = false;
	mPendingStepValue = -1;
	mStepStage = 0;
	ClearAllHighlight();

	// UI에 스킬/주사위 선택 강조를 풀라고 알린다(확정/취소 공통).
	if (mViewModel != nullptr)
	{
		mViewModel->NotifyActionResolved();
	}
}

bool UCombatViewAdapter::ResolveTileFromScreen(const FVector2D& ScreenPosition, FTileIndex& OutTile) const
{
	ATileMap* TileMap = mCombat != nullptr ? mCombat->GetTileMap() : nullptr;
	UWorld* World = mCombat != nullptr ? mCombat->GetWorld() : nullptr;
	if (TileMap == nullptr || World == nullptr)
	{
		return false;
	}
	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (PlayerController == nullptr)
	{
		return false;
	}

	// 화면 좌표 → 월드 광선.
	FVector WorldOrigin;
	FVector WorldDirection;
	if (PlayerController->DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, WorldOrigin, WorldDirection) == false)
	{
		return false;
	}

	// 타일맵 평면(z = 타일맵 액터 높이)과 광선 교차.
	const float PlaneZ = TileMap->GetActorLocation().Z;
	if (FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}
	const float RayT = (PlaneZ - WorldOrigin.Z) / WorldDirection.Z;
	if (RayT < 0.0f)
	{
		return false;
	}
	const FVector HitPoint = WorldOrigin + WorldDirection * RayT;

	const FTileIndex Tile = TileMap->WorldToTileIndex(HitPoint);
	if (TileMap->IsValidIndex(Tile) == false)
	{
		return false;   // 타일맵 밖.
	}
	OutTile = Tile;
	return true;
}

FVector UCombatViewAdapter::TileToWorld(const FTileIndex& Tile) const
{
	const ATileMap* TileMap = mCombat != nullptr ? mCombat->GetTileMap() : nullptr;
	return TileMap != nullptr ? TileMap->TileToWorldLocation(Tile) : FVector::ZeroVector;
}

void UCombatViewAdapter::PushAll()
{
	if (mViewModel == nullptr)
	{
		return;
	}

	TArray<FUnitView> UnitViews;
	UnitViews.Reserve(mUnitStates.Num());
	int32 PlayerUnitId = INDEX_NONE;
	for (const FCombatUnitState& State : mUnitStates)
	{
		FUnitView View;
		View.mUnitId = State.mId;
		View.mIsPlayer = State.mIsPlayer;
		View.mHP = State.mHP;
		View.mMaxHP = State.mMaxHP;
		View.mMovementPoint = State.mMovePoint;
		View.mMaxMovementPoint = State.mMaxMovePoint;
		View.mTile = State.mTile;
		// 실제 액터가 있으면 그 위치, 가상이면 타일→월드 변환.
		View.mWorldLocation = State.mActor.IsValid()
			? State.mActor->GetActorLocation()
			: TileToWorld(State.mTile);

		if (State.mIsPlayer)
		{
			PlayerUnitId = State.mId;
		}
		UnitViews.Add(View);
	}
	mViewModel->SetUnitViews(UnitViews);

	FPlayerMetaView Meta;
	Meta.mLevel = mPlayerLevel;
	Meta.mGold = mPlayerGold;
	mViewModel->SetPlayerMeta(Meta);

	FTurnView Turn;
	Turn.mRound = 1;
	Turn.mCurrentUnitId = PlayerUnitId;
	mViewModel->SetTurnView(Turn);
}

const FCombatUnitState* UCombatViewAdapter::FindPlayerState() const
{
	return mUnitStates.FindByPredicate([](const FCombatUnitState& State) { return State.mIsPlayer; });
}

FCombatUnitState* UCombatViewAdapter::FindStateById(int32 UnitId)
{
	return mUnitStates.FindByPredicate([UnitId](const FCombatUnitState& State) { return State.mId == UnitId; });
}

void UCombatViewAdapter::ApplyBasicAttack(int32 TargetUnitId, int32 Amount)
{
	FCombatUnitState* Target = FindStateById(TargetUnitId);
	if (Target == nullptr || Target->mIsPlayer)
	{
		return;
	}

	Target->mHP = FMath::Max(0.0f, Target->mHP - static_cast<float>(Amount));
	if (Target->mHP <= 0.0f)
	{
		// 가상 적 사망: 상태에서 제거.
		const int32 RemoveId = Target->mId;
		mUnitStates.RemoveAll([RemoveId](const FCombatUnitState& State) { return State.mId == RemoveId; });
	}
	PushAll();
}

void UCombatViewAdapter::ApplyStep(int32 Amount)
{
	FCombatUnitState* Player = mUnitStates.FindByPredicate([](const FCombatUnitState& State) { return State.mIsPlayer; });
	if (Player == nullptr)
	{
		return;
	}
	Player->mMovePoint += Amount;
	Player->mMaxMovePoint = Player->mMovePoint;   // STEP 직후 현재=최대(6/6).
	PushAll();
}

bool UCombatViewAdapter::TryMovePlayer(const FTileIndex& TargetTile)
{
	FCombatUnitState* Player = mUnitStates.FindByPredicate([](const FCombatUnitState& State) { return State.mIsPlayer; });
	if (Player == nullptr || Player->mMovePoint <= 0)
	{
		return false;
	}

	// 가상 적이 있는 타일로는 이동 불가.
	if (FindUnitIdAtTile(TargetTile) != INDEX_NONE)
	{
		return false;
	}

	Player->mTile = TargetTile;
	Player->mMovePoint -= 1;

	// 실제 플레이어 액터도 타일맵 위에서 옮긴다.
	if (Player->mActor.IsValid())
	{
		if (ATileMap* TileMap = mCombat != nullptr ? mCombat->GetTileMap() : nullptr)
		{
			TileMap->StartActorMovement(FTileTransform(TargetTile), Player->mActor.Get());
			TileMap->CompleteActorMovement(Player->mActor.Get());
		}
	}

	PushAll();
	return true;
}

int32 UCombatViewAdapter::FindUnitIdAtTile(const FTileIndex& Tile) const
{
	const FCombatUnitState* Found = mUnitStates.FindByPredicate([&Tile](const FCombatUnitState& State)
	{
		return State.mTile.mX == Tile.mX && State.mTile.mY == Tile.mY;
	});
	return Found != nullptr ? Found->mId : INDEX_NONE;
}

FTileIndex UCombatViewAdapter::GetPlayerTile() const
{
	const FCombatUnitState* Player = FindPlayerState();
	return Player != nullptr ? Player->mTile : FTileIndex::Invalid;
}

int32 UCombatViewAdapter::GetPlayerMovePoint() const
{
	const FCombatUnitState* Player = FindPlayerState();
	return Player != nullptr ? Player->mMovePoint : 0;
}
