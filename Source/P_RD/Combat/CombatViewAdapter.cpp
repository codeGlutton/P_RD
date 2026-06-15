#include "Combat/CombatViewAdapter.h"

#include "Actor/TileMap/TileMap.h"
#include "Pawn/Unit.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "UI/Combat/CombatViewModel.h"
#include "UI/Combat/CombatViewTypes.h"

namespace
{
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
	mViewModel = InViewModel;
	PushAll();
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
