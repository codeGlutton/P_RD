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
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "Actor/TileMap/TileMapModel.h"

TArray<TInstancedStruct<FSRPGCommand>> USRPGEnemyTurnPlanner::PlanTurn(
	UEnemyUnitModel* Enemy,
	UUnitModel* Player,
	const UTileMapModel* TileMap,
	const FRandomStream& EventStream)
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

	// 스킬 컴포넌트 확인: 없으면 할 게 없으므로 턴 종료
	USkillComponentModel* SkillComp = Enemy->GetSkillComponentModel();
	if (SkillComp == nullptr)
	{
		return Commands;
	}
	
	// 스킬 슬롯에 유효한 스킬이 있는 지 확인: 스킬이 없으면 딱히 할 게 없으므로 턴 종료
	TArray<int32> ValidSkillIndexes;
	const TArray<FSkillEntry>& Skills = SkillComp->GetSkills();
	for (int32 Index = 0; Index < Skills.Num(); ++Index)
	{
		if (Skills[Index].IsValid())
			ValidSkillIndexes.Add(Index);
	}
	if (ValidSkillIndexes.IsEmpty())
		return Commands;

	// 속성 컴포넌트 확인: 없으면 할 게 없으므로 턴 종료
	UAttributeSetComponentModel* AttributeSetComp = Enemy->GetAttributeComponentModel();
	if (AttributeSetComp == nullptr)
	{
		return Commands;
	}

	// 사용 가능한 액션포인트
	const int32 ActionPoint = FMath::Max(
		AttributeSetComp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetMovementAttribute()),
		0
	);
	
	const FTileIndex EnemyTile = Enemy->GetTileTransform().mIndex;
	const FTileIndex PlayerTile = Player->GetTileTransform().mIndex;
	
	// 적이 이동할 수 있는 모든 타일들 수집
	TArray<FTileIndex> ReachableTiles = TileMap->GetReachableTiles(EnemyTile, ActionPoint);
	ReachableTiles.Add(EnemyTile);
	
	// 적이 이동할 수 있는 모든 타일들에 대해서 이동거리를 미리 계산
	// 그래야 어느 타일로 갈 때 얼마나 드는 지 금방 비교할 수 있음
	const TArray<int32> DistanceField = TileMap->GetDistanceField(EnemyTile, Enemy);
	
	// 스킬별 조준가능/시전가능 타일들
	TArray<TArray<FTileIndex>> AimableTiles;
	TArray<TArray<FTileIndex>> CastableTiles;
	AimableTiles.SetNum(Skills.Num());
	CastableTiles.SetNum(Skills.Num());
	
	/**
	 * @brief 스킬별 조준가능/시전가능 타일들 수집
	 */
	for (const int32 SkillIndex : ValidSkillIndexes)
	{
		const UStaticSkillData* Skill = Skills[SkillIndex].mData;
		
		// 도달 가능한 모든 타일에 대해서 탐색
		for (const FTileIndex Tile : ReachableTiles)
		{
			// 해당타일에서 조준가능한 타일들 수집
			// @note 아직 플레이어를 조준할 수 있는지는 모름
			TArray<FTileIndex> Aimables = TileMap->GetAimableTiles(
				Tile,
				Skill->mAimRange,
				Skill->mAimPattern,
				Skill->mCanAimBoardActor,
				Skill->mIsIndirect,
				nullptr,
				Enemy);
			
			if (Aimables.Contains(PlayerTile) == false)
				continue;
			
			// 조준가능한 타일들중에 플레이어 타일이 있으면 최종조준가능한 타일
			// @note 아직 플레이어에게 시전할 수 있는지는 모름
			AimableTiles[SkillIndex].Add(Tile);
			
			const int32 LinearIndex = TileMap->TileIndexToLinearIndex(Tile);
			// 조준가능한 타일까지 이동하는 비용
			const int32 MoveDistance = DistanceField[LinearIndex];
			if (MoveDistance + Skill->mRequiredMovement <= ActionPoint)
			{
				// 현재 액션포인트로 이동포인트와 스킬사용포인트까지 감당할 수 있으면 시전가능한 타일
				CastableTiles[SkillIndex].Add(Tile);
			}
		}
	}
	
	// 이번 턴에 시전가능한 스킬들 수집
	TArray<int32> CastableSkillIndexes;
	for (const int32 SkillIndex : ValidSkillIndexes)
	{
		if (CastableTiles[SkillIndex].IsEmpty() == false)
			CastableSkillIndexes.Add(SkillIndex);
	}
	
	bool CanCast = false;
	int32 ChosenSkillIndex = INDEX_NONE;
	FTileIndex Dest = EnemyTile;
	
	//
	// 1단계: 이동+시전으로 플레이어를 때릴 수 있으면 시전을 계획
	//
	if (CastableSkillIndexes.IsEmpty() == false)
	{
		// 시전 가능한 스킬 중에서 랜덤 선택
		const int32 Pick = EventStream.RandRange(0, CastableSkillIndexes.Num() - 1);
		
		// 시전할 스킬 인덱스, 시전 계획, 목표 타일 등을 설정
		ChosenSkillIndex = CastableSkillIndexes[Pick];
		CanCast = true;
		Dest = PickByTendency(CastableTiles[ChosenSkillIndex], EnemyTile, PlayerTile, Enemy->GetMoveTendency());
	}
	else
	{
		//
		// 2단계: 이번 턴에는 못 때리므로 다음 턴에 때릴 수 있는 타일로 이동
		//
		// 조준타일들의 합집합이 대상
		TArray<FTileIndex> PositioningTiles;
		for (const int32 SkillIndex : ValidSkillIndexes)
		{
			for (const FTileIndex Tile : AimableTiles[SkillIndex])
			{
				// 중복할 필요가 없으므로 유니크한 인덱스만 수집
				PositioningTiles.AddUnique(Tile);
			}
		}
		
		if (PositioningTiles.IsEmpty() == false)
		{
			// 조준가능한 타일이 있으면, 일단 그 타일에 가 있으면 다음 턴에서 플레이어를 때릴 수 있음
			Dest = PickByTendency(PositioningTiles, EnemyTile, PlayerTile, Enemy->GetMoveTendency());
		}
		else
		{
			//
			// 3단계: 어디로 가든, 다음 턴에서 플레이어를 때릴 수 없다면, 플레이어에게 최대한 접근
			//
			Dest = PickApproachTile(ReachableTiles, EnemyTile, PlayerTile, TileMap, Enemy);
		}
	}

	/**
	 * @brief 이동커맨드 생성 여부 판단 및 생성
	 * @details
	 * 현재위치와 목적지가 다를때만 이동커맨드 생성
	 */
	if (Dest != EnemyTile)
	{
		TArray<FTileIndex> Path = TileMap->FindPath(EnemyTile, Dest);
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
		// 시전 커맨드에는 타겟 타일과 주사위 합만 담는다.
		// 실제 효과 타일은 실행 시점에 스킬 컴포넌트가 목적지(시전 원점) 기준으로 계산한다.
		TInstancedStruct<FSRPGCommand> Cast;
		Cast.InitializeAs<FSRPGSkillCastCommand>();
		FSRPGSkillCastCommand& CastRef = Cast.GetMutable<FSRPGSkillCastCommand>();
		CastRef.mSkillIndex = ChosenSkillIndex;
		CastRef.mTargetIndex = PlayerTile;
		AddAction(MoveTemp(Cast));
	}

	return Commands;
}

FTileIndex USRPGEnemyTurnPlanner::PickByTendency(
	const TArray<FTileIndex>& Tiles,
	const FTileIndex& EnemyTile,
	const FTileIndex& PlayerTile,
	EMoveTendency Tendency)
{
	// 이동성향에 따라 타일 목록에서 최선 타일 선택
	switch (Tendency)
	{
	case EMoveTendency::MoveClose:
		// 근접 성향: 플레이어와 가장 가까운 타일
		return PickByPlayerDistance(Tiles, EnemyTile, PlayerTile, /*Closest*/true);
	case EMoveTendency::MoveAway:
		// 원거리 성향: 플레이어와 가장 먼 타일
		return PickByPlayerDistance(Tiles, EnemyTile, PlayerTile, /*Closest*/false);
	case EMoveTendency::HoldRange:
	default:
		// 거리유지 성향: 제자리에서 가능하면 제자리, 아니면 이동거리가 가장 작은 타일
		return (Tiles.Contains(EnemyTile) == true) ? EnemyTile : PickByMoveCost(Tiles, EnemyTile);
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

FTileIndex USRPGEnemyTurnPlanner::PickApproachTile(
	const TArray<FTileIndex>& Tiles,
	const FTileIndex& Origin,
	const FTileIndex& PlayerTile,
	const UTileMapModel* TileMap,
	const UBoardActorModel* Self)
{
	// 플레이어 기준 경로 거리 표 (자기 자신은 자리를 비울 예정이므로 통과 판정에서 제외)
	const TArray<int32> DistanceField = TileMap->GetDistanceField(PlayerTile, Self);

	FTileIndex Best = Origin;
	bool HasBest = false;
	int32 BestPathDist = 0;
	int32 BestMoveCost = 0;
	for (const FTileIndex& Tile : Tiles)
	{
		// 해당 타일과 플레이어 사이의 경로 거리
		const int32 Linear = TileMap->TileIndexToLinearIndex(Tile);
		const int32 RawDist = (Linear != INDEX_NONE) ? DistanceField[Linear] : INDEX_NONE;
		const int32 PathDist = (RawDist >= 0) ? RawDist : TNumericLimits<int32>::Max();
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
		// 2) 최선 타일이 있다면, 1순위 비교: 경로 거리가 가까우면 최선
		else if (PathDist != BestPathDist)
		{
			Better = (PathDist < BestPathDist);
		}
		// 3) 1순위가 같다면, 2순위 비교
		else
		{
			// 2순위는 이동거리가 작으면 최선 (동률이면 제자리 우선 -> 의미 없는 이동 방지)
			Better = (MoveCost < BestMoveCost);
		}

		// 신규 최선 타일이 있다면 그걸 최선으로 설정해서 리턴할 때 사용
		if (Better == true)
		{
			Best = Tile;
			BestPathDist = PathDist;
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
