/*****************************************************************//**
 * @file   SRPGEnemyTurnPlanner.cpp
 * @brief  적 한 턴의 행동을 계산하는 모델 레이어 플래너 구현
 * @author 이문환
 * @date   2026-06-30
 *********************************************************************/

#include "SRPGFramework/SRPGEnemyTurnPlanner.h"

#include "SRPGFramework/SRPGMoveAction.h"
#include "SRPGFramework/SRPGSkillAction.h"
#include "SRPGFramework/SRPGTurnEndAction.h"

#include "Pawn/Enemy/EnemyUnitModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "Actor/TileMap/TileMapModel.h"

TArray<TInstancedStruct<FSRPGCommand>> USRPGEnemyTurnPlanner::PlanTurn(
	UEnemyUnitModel* Enemy,
	UUnitModel* Player,
	const UTileMapModel* TileMap)
{
	TArray<TInstancedStruct<FSRPGCommand>> Commands;
	{
		// 종료 커맨드는 무조건 필요하므로 일단 추가하고 시작
		TInstancedStruct<FSRPGCommand> TurnEnd;
		TurnEnd.InitializeAs<FSRPGTurnEndCommand>();
		Commands.Add(MoveTemp(TurnEnd));
	}

	// @note 종료커맨드를 추가하고 시작했으므로 이후 커맨드는 종료커맨드 앞에 넣기 위해 가드 설정.
	//       가드 없으면 실수로 맨 뒤에 넣을 수 있으니까.
	auto AddAction = [&Commands](TInstancedStruct<FSRPGCommand>&& Command)
	{
		Commands.Insert(MoveTemp(Command), Commands.Num() - 1);
	};

	// 가드: 필수 입력이 없으면 행동 없이 턴만 종료
	if (Enemy == nullptr || Player == nullptr || TileMap == nullptr)
	{
		return Commands;
	}

	// 스킬 개수 확인: 0이면 할 게 없으므로 턴 종료
	USkillComponentModel* SkillComp = Enemy->GetSkillComponentModel();
	if (SkillComp == nullptr || SkillComp->GetSkillCount() <= 0)
	{
		return Commands;
	}

	// 사거리를 알기 위해 스킬 정보 조회
	const int32 SkillIndex = 0;
	TSoftObjectPtr<UStaticSkillData> SkillSoft;
	SkillComp->GetSkillData(SkillIndex, OUT SkillSoft);
	const UStaticSkillData* Skill = SkillSoft.Get();
	if (Skill == nullptr)
	{
		return Commands;
	}

	const FTileIndex Origin = Enemy->GetTileTransform().mIndex;
	const FTileIndex PlayerTile = Player->GetTileTransform().mIndex;

	// MovementPoint만큼 이동 가능
	int32 MoveRange = 0;
	if (UAttributeSetComponentModel* AttrComp = Enemy->GetAttributeComponentModel())
	{
		MoveRange = FMath::FloorToInt(AttrComp->GetAttributeCurrentValue(UUnitAttributeSet::GetMovementPointAttribute()));
	}
	MoveRange = FMath::Max(MoveRange, 0);

	// 조준거리
	const int32 AimRange = Skill->mAimDefaultRange;

	// 목적지 결정 (이동 성향 기반)
	bool CanCast = false;
	const FTileIndex Dest = ChooseDestination(Origin, PlayerTile, MoveRange, AimRange, Skill, TileMap, Enemy->GetMoveTendency(), OUT CanCast);

	/**
	 * @brief 이동커맨드 생성 여부 판단 및 생성
	 * @details
	 * 현재위치와 목적지가 다를때만 이동커맨드 생성
	 */
	if (Dest != Origin)
	{
		TArray<FTileIndex> Path = TileMap->FindPath(Origin, Dest);
		// 경로는 출발지와 목적지가 포함되므로 2 이상이어야 실제 이동 가능.
		if (Path.Num() >= 2)
		{
			// 이동커맨드 생성
			TInstancedStruct<FSRPGCommand> Move;
			Move.InitializeAs<FSRPGMoveCommand>();
			Move.GetMutable<FSRPGMoveCommand>().mPathTileIndexes = MoveTemp(Path);
			AddAction(MoveTemp(Move));
		}
	}

	/**
	 * @brief 스킬커맨드 생성 여부 판단 및 생성
	 * @details
	 * 스킬 사용이 가능할 경우에만 생성
	 * 목적지 타일에 도착해서 시전한다고 가정
	 */ 
	if (CanCast == true)
	{
		// 시전 원점은 이동 후 목적지(Dest)
		const int32 EffectArea = Skill->mEffectDefaultArea;
		TArray<FTileIndex> EffectTiles = TileMap->GetEffectTiles(
			Dest,
			PlayerTile,
			Skill->mEffectPattern,
			EffectArea,
			Skill->mIsPenetration);

		TInstancedStruct<FSRPGCommand> Cast;
		Cast.InitializeAs<FSRPGSkillCastCommand>();
		FSRPGSkillCastCommand& CastRef = Cast.GetMutable<FSRPGSkillCastCommand>();
		CastRef.mSkillIndex = SkillIndex;
		CastRef.mEffectTileIndexes = MoveTemp(EffectTiles);
		// 몹은 주사위 포인트 없다고 가정
		CastRef.mDicePoint = 0.0f;
		AddAction(MoveTemp(Cast));
	}

	return Commands;
}

FTileIndex USRPGEnemyTurnPlanner::ChooseDestination(
	const FTileIndex& Origin,
	const FTileIndex& PlayerTile,
	int32 MoveRange,
	int32 AimRange,
	const UStaticSkillData* Skill,
	const UTileMapModel* TileMap,
	EMoveTendency Tendency,
	OUT bool& OutCanCast)
{
	// 도달 가능한 모든 타일 집합 (현재 위치한 타일도 포함)
	TArray<FTileIndex> Candidates = TileMap->GetReachableTiles(Origin, MoveRange);
	Candidates.Add(Origin);

	// 이동 후 플레이어를 조준 가능한 타일만 추림
	TArray<FTileIndex> Feasible;
	for (const FTileIndex& Candidate : Candidates)
	{
		TArray<FTileIndex> Aimable = TileMap->GetAimableTiles(Candidate, AimRange, Skill->mAimPattern, Skill->mCanAimObstacle, Skill->mIsIndirect);
		if (Aimable.Contains(PlayerTile) == true)
		{
			Feasible.Add(Candidate);
		}
	}

	// 이동 성향에 따라 타일 탐색
	switch (Tendency)
	{
	case EMoveTendency::MoveClose:
		// 1) 조준 가능한 타일이 있다면
		if (Feasible.Num() > 0)
		{
			// 조준 가능한 타일이 있으니까 스킬 시전도 가능하게 설정
			OutCanCast = true;
			// 조준 가능한 타일들 중에서 플레이어와 가장 가까운 타일 선택
			return PickByPlayerDistance(Feasible, Origin, PlayerTile, /*Closest*/true);
		}
		// 2) 조준 가능한 타일이 없다면
		else
		{
			// 조준 가능한 타일이 없음 -> 어느 타일에서도 플레이어를 조준할 수 없음 -> 스킬 시전 불가능 설정
			OutCanCast = false;
			// '도달' 가능한 타일들 중에서 플레이어와 가장 가까운 타일 선택
			return PickByPlayerDistance(Candidates, Origin, PlayerTile, /*Closest*/true);
		}
	case EMoveTendency::MoveAway:
		// 1) 조준 가능한 타일이 있다면
		if (Feasible.Num() > 0)
		{
			// 조준 가능한 타일이 있으니까 스킬 시전도 가능하게 설정
			OutCanCast = true;
			// 조준 가능한 타일들 중에서 플레이어와 가장 먼 타일 선택
			return PickByPlayerDistance(Feasible, Origin, PlayerTile, /*Closest*/false);
		}
		else
		{
			// 조준 가능한 타일이 없음 -> 어느 타일에서도 플레이어를 조준할 수 없음 -> 스킬 시전 불가능 설정
			OutCanCast = false;
			// '도달' 가능한 타일들 중에서 플레이어와 가장 가까운 타일 선택 -> 다음번에 조준 가능성을 높임
			return PickByPlayerDistance(Candidates, Origin, PlayerTile, /*Closest*/true);
		}
	case EMoveTendency::HoldRange:
	default:
		// 1) 이미 조준할 수 있다면 제자리 유지
		if (Feasible.Contains(Origin) == true)
		{
			OutCanCast = true;
			return Origin;
		}
		// 2) 제자리에서 조준할 수 없다면 -> 이동거리가 가장 작은 조준 가능한 타일이 최선
		if (Feasible.Num() > 0)
		{
			OutCanCast = true;
			return PickByMoveCost(Feasible, Origin);
		}
		// 3) 조준할 수 있는 타일이 없다면 -> 도달 가능한 타일들 중에서 플레이어와 가장 가까운 타일이 최선
		else
		{
			OutCanCast = false;
			return PickByPlayerDistance(Candidates, Origin, PlayerTile, /*Closest*/true);
		}
	}
}

FTileIndex USRPGEnemyTurnPlanner::PickByPlayerDistance(
	const TArray<FTileIndex>& Tiles,
	const FTileIndex& Origin,
	const FTileIndex& PlayerTile,
	bool Closest)
{
	FTileIndex Best = Origin;
	bool HasBest = false;
	int32 BestPlayerDist = 0;
	int32 BestMoveCost = 0;
	for (const FTileIndex& Tile : Tiles)
	{
		// 해당 타일과 플레이어 사이의 거리
		const int32 PlayerDist = TileDistance(Tile, PlayerTile);
		// 해당 타일과 적 사이의 거리
		const int32 MoveCost = TileDistance(Tile, Origin);

		bool Better = false;

		/**
		 * 최선 타일 선택
		 */
		// 1) 아직 최선 타일이 없으면 이 타일이 바로 최선!
		if (HasBest == false)
		{
			Better = true;
		}
		// 2) 최선 타일이 있다면, 1순위 비교
		else if (PlayerDist != BestPlayerDist)
		{
			Better = Closest
				// 근거리를 좋아하면, 플레이어와의 거리가 가까우면 최선
				? (PlayerDist < BestPlayerDist)
				// 원거리를 좋아하면, 플레이어와의 거리가 멀어지면 최선
				: (PlayerDist > BestPlayerDist);
		}
		// 3) 1순위가 같다면, 2순위 비교
		else
		{
			// 2순위는 이동거리가 작으면 최선
			Better = (MoveCost < BestMoveCost);
		}

		// 신규 최선 타일이 있다면 그걸 최선으로 설정해서 리턴할 때 사용
		if (Better == true)
		{
			Best = Tile;
			BestPlayerDist = PlayerDist;
			BestMoveCost = MoveCost;
			HasBest = true;
		}
	}
	return Best;
}

FTileIndex USRPGEnemyTurnPlanner::PickByMoveCost(
	const TArray<FTileIndex>& Tiles,
	const FTileIndex& Origin)
{
	FTileIndex Best = Origin;
	bool HasBest = false;
	int32 BestMoveCost = 0;
	for (const FTileIndex& Tile : Tiles)
	{
		const int32 MoveCost = TileDistance(Tile, Origin);
		// 최선 타일이 없으면 이게 최선!
		// 최선 타일이 있으면 -> 이동비용이 작으면 최선
		if (HasBest == false || MoveCost < BestMoveCost)
		{
			Best = Tile;
			BestMoveCost = MoveCost;
			HasBest = true;
		}
	}
	return Best;
}

int32 USRPGEnemyTurnPlanner::TileDistance(const FTileIndex& A, const FTileIndex& B)
{
	return FMath::Abs(A.mX - B.mX) + FMath::Abs(A.mY - B.mY);
}
