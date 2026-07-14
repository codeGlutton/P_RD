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

	// 속성 컴포넌트 확인: 없으면 할 게 없으므로 턴 종료
	UAttributeSetComponentModel* AttributeSetComp = Enemy->GetAttributeComponentModel();
	if (AttributeSetComp == nullptr)
	{
		return Commands;
	}

	// 장착된 스킬 슬롯 인덱스 수집
	// @note 스킬 슬롯은 고정 크기로 미리 확보되므로, 개수가 아니라 슬롯의 데이터 유무로 판단
	const TArray<FSkillEntry>& Skills = SkillComp->GetSkills();
	TArray<int32> EquippedIndexes;
	for (int32 Index = 0; Index < Skills.Num(); ++Index)
	{
		if (Skills[Index].mData != nullptr)
		{
			EquippedIndexes.Add(Index);
		}
	}
	// 모든 슬롯이 비어있으면(스킬 미장착) 할 게 없으므로 턴 종료
	if (EquippedIndexes.Num() == 0)
	{
		return Commands;
	}

	// 사용할 스킬을 랜덤으로 하나 선택. 이후 이동/시전 판단은 이 스킬의 사거리 기준
	// @note 시뮬/라이브 동일 결과 보장을 위해 반드시 룸의 이벤트 스트림에서 뽑아야 함
	const int32 SkillIndex = EquippedIndexes[EventStream.RandRange(0, EquippedIndexes.Num() - 1)];
	const UStaticSkillData* Skill = Skills[SkillIndex].mData;

	// 스킬이 여러 개일 때만 어떤 스킬이 뽑혔는지 확인용 로그
	if (EquippedIndexes.Num() >= 2)
	{
		UE_LOG(LogRD, Log, TEXT("적 AI 스킬 랜덤 선택: %s(ID=%d), 후보 %d개 중 슬롯 %d (%s)"),
			*GetNameSafe(Enemy), Enemy->GetModelId(), EquippedIndexes.Num(), SkillIndex, *GetNameSafe(Skill));
	}

	const FTileIndex Origin = Enemy->GetTileTransform().mIndex;
	const FTileIndex PlayerTile = Player->GetTileTransform().mIndex;

	// 습득한 이동포인트만큼 이동 가능
	const int32 MoveRange = FMath::Max(
		AttributeSetComp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetMovementAttribute()),
		0
	);

	// 주사위 합
	const float DiceSum = AttributeSetComp->GetAttributeCurrentValue(UEnemyUnitAttributeSet::GetRechargeDiceSumAttribute());

	// 조준거리
	// @note 몹은 주사위가 없어 DiceSum=0 → 사거리는 기본값(mAimRangeDefaultValue) 그대로
	const int32 AimRange = Skill->mAimRangeDefaultValue + Skill->mAimRangeRatio * DiceSum;

	// 목적지 결정 (이동 성향 기반)
	bool CanCast = false;
	const FTileIndex Dest = ChooseDestination(Origin, PlayerTile, MoveRange, AimRange, Skill, TileMap, Enemy->GetMoveTendency(), Enemy, OUT CanCast);

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
		// 시전 커맨드에는 타겟 타일과 주사위 합만 담는다.
		// 실제 효과 타일은 실행 시점에 스킬 컴포넌트가 목적지(시전 원점) 기준으로 계산한다.
		TInstancedStruct<FSRPGCommand> Cast;
		Cast.InitializeAs<FSRPGSkillCastCommand>();
		FSRPGSkillCastCommand& CastRef = Cast.GetMutable<FSRPGSkillCastCommand>();
		CastRef.mSkillIndex = SkillIndex;
		CastRef.mTargetIndex = PlayerTile;
		CastRef.mDiceSum = DiceSum;
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
	const UBoardActorModel* Self,
	OUT bool& OutCanCast)
{
	// 도달 가능한 모든 타일 집합 (현재 위치한 타일도 포함)
	TArray<FTileIndex> Candidates = TileMap->GetReachableTiles(Origin, MoveRange);
	Candidates.Add(Origin);

	// 이동 후 플레이어를 조준 가능한 타일만 추림
	// @note 자기 자신(Self)은 이동으로 자리를 비울 예정이므로 시야 차폐에서 제외.
	//       제외하지 않으면 직선 후퇴 타일이 전부 자기 몸에 막혀 제자리 사격이 됨.
	TArray<FTileIndex> Feasible;
	for (const FTileIndex& Candidate : Candidates)
	{
		TArray<FTileIndex> Aimable = TileMap->GetAimableTiles(Candidate, AimRange, Skill->mAimPattern, Skill->mCanAimBoardActor, Skill->mIsIndirect, /*Incoming*/nullptr, /*IgnoreBlocker*/Self);
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
			// '도달' 가능한 타일들 중에서 경로 거리 기준으로 플레이어와 가장 가까워지는 타일 선택
			return PickApproachTile(Candidates, Origin, PlayerTile, TileMap, Self);
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
			// '도달' 가능한 타일들 중에서 경로 거리 기준으로 플레이어와 가장 가까워지는 타일 선택 -> 다음번에 조준 가능성을 높임
			return PickApproachTile(Candidates, Origin, PlayerTile, TileMap, Self);
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
		// 3) 조준할 수 있는 타일이 없다면 -> 도달 가능한 타일들 중에서 경로 거리 기준으로 가장 가까워지는 타일이 최선
		else
		{
			OutCanCast = false;
			return PickApproachTile(Candidates, Origin, PlayerTile, TileMap, Self);
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
