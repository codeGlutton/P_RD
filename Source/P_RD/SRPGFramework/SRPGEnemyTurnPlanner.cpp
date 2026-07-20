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
	const FRandomStream& EventStream,
	int32 MoveRangeOverride,
	int32 SkillIndexOverride,
	int32* OutPlannedSkillIndex,
	const TArray<FTileIndex>* ReservedDestinations,
	FTileIndex PreviousDestination,
	bool bWasDisplaced,
	FTileIndex DisplacedFrom)
{
	if (OutPlannedSkillIndex != nullptr)
	{
		*OutPlannedSkillIndex = INDEX_NONE;
	}
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

	// 최초 계획은 랜덤으로 스킬을 고른다. 플레이어 행동 뒤 갱신되는 계획은 공개했던 전술 정체성을
	// 유지하기 위해 SkillIndexOverride로 같은 스킬을 다시 사용한다.
	// @note 시뮬/라이브 동일 결과 보장을 위해 반드시 룸의 이벤트 스트림에서 뽑아야 함
	const int32 SkillIndex = EquippedIndexes.Contains(SkillIndexOverride)
		? SkillIndexOverride
		: EquippedIndexes[EventStream.RandRange(0, EquippedIndexes.Num() - 1)];
	if (OutPlannedSkillIndex != nullptr)
	{
		*OutPlannedSkillIndex = SkillIndex;
	}
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
	const int32 RequestedMoveRange = MoveRangeOverride != INDEX_NONE
		? FMath::Max(MoveRangeOverride, 0)
		: FMath::Max(
			AttributeSetComp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetMovementAttribute()),
			0
		);
	// 적도 한 번의 대응에서는 한 칸만 이동한다. 공격 사거리 안이라면 이동 대신 공격 하나를 실행한다.
	const int32 MoveRange = FMath::Min(RequestedMoveRange, 1);

	// 주사위 합
	const float DiceSum = AttributeSetComp->GetAttributeCurrentValue(UEnemyUnitAttributeSet::GetRechargeDiceSumAttribute());

	// 조준거리
	// @note 몹은 주사위가 없어 DiceSum=0 → 사거리는 기본값(mAimRangeDefaultValue) 그대로
	const int32 AimRange = Skill->mAimRangeDefaultValue + Skill->mAimRangeRatio * DiceSum;

	// 목적지 결정 (이동 성향 기반)
	bool CanCast = false;
	const FTileIndex Dest = ChooseDestination(
		Origin,
		PlayerTile,
		MoveRange,
		AimRange,
		Skill,
		TileMap,
		Enemy->GetMoveTendency(),
		Enemy->GetMovementRole(),
		Enemy,
		ReservedDestinations,
		PreviousDestination,
		bWasDisplaced,
		DisplacedFrom,
		OUT CanCast);

	/**
	 * @brief 이동커맨드 생성 여부 판단 및 생성
	 * @details
	 * 현재위치와 목적지가 다를때만 이동커맨드 생성
	 */
	const bool bPlanElasticCharge = (Enemy->GetMovementRole() == ESRPGEnemyMovementRole::Slider
		|| Enemy->GetMovementRole() == ESRPGEnemyMovementRole::Lancer)
		&& Origin != PlayerTile
		&& MoveRange > 0;
	if (Dest != Origin || bPlanElasticCharge)
	{
		const bool bElasticCharge = bPlanElasticCharge;
		TArray<FTileIndex> Path;
		if (bElasticCharge && MoveRange > 0)
		{
			// 슬라임은 안전한 우회 경로를 찾지 않는다. 플레이어 방향으로 몸을 튕기며,
			// 첫 점유 칸까지도 공개 경로에 포함해 실행 시 밀침/충돌 판정이 실제로 발생한다.
			FTileIndex Cursor = Origin;
			Path.Add(Cursor);
			for (int32 Distance = 0; Distance < MoveRange && Cursor != PlayerTile; ++Distance)
			{
				const int32 DeltaX = PlayerTile.mX - Cursor.mX;
				const int32 DeltaY = PlayerTile.mY - Cursor.mY;
				const int32 AbsX = FMath::Abs(DeltaX);
				const int32 AbsY = FMath::Abs(DeltaY);
				const FTileIndex Step(
					AbsX >= AbsY ? FMath::Sign(DeltaX) : 0,
					AbsY >= AbsX ? FMath::Sign(DeltaY) : 0);
				const FTileIndex Next(Cursor.mX + Step.mX, Cursor.mY + Step.mY);
				if (TileMap->IsValidIndex(Next) == false)
				{
					break;
				}
				Path.Add(Next);
				if (Next == PlayerTile || TileMap->CanPlace(Next, Enemy) == false)
				{
					break;
				}
				Cursor = Next;
			}
			if (Path.Num() >= 2)
			{
				// 탄성 돌진 턴은 이동 그 자체가 공격이다. 다른 위치를 전제로 계산한 원본
				// 스킬을 뒤이어 시전하지 않아 예고와 실제 판정을 일치시킨다.
				CanCast = false;
			}
		}
		else
		{
			Path = TileMap->FindPath(Origin, Dest);
		}
		// 경로는 출발지와 목적지가 포함되므로 2 이상이어야 실제 이동 가능.
		if (Path.Num() >= 2)
		{
			// 이동커맨드 생성
			TInstancedStruct<FSRPGCommand> Move;
			Move.InitializeAs<FSRPGMoveCommand>();
			FSRPGMoveCommand& MoveRef = Move.GetMutable<FSRPGMoveCommand>();
			MoveRef.mPathTileIndexes = MoveTemp(Path);
			MoveRef.mIsElasticCharge = bElasticCharge;
			AddAction(MoveTemp(Move));
			// 한 번의 대응은 이동 또는 공격 하나다. 이동 뒤 같은 턴의 공격을 계획/UI에 남기지 않는다.
			CanCast = false;
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
		// 고정 의도 실행에서는 목적지 기준 효과 타일을 이 스냅샷 그대로 사용한다.
		CastRef.mFixedEffectTileIndexes = TileMap->GetEffectTiles(
			Dest,
			PlayerTile,
			Skill->mEffectPattern,
			FMath::Max(static_cast<int32>(Skill->mEffectAreaDefaultValue + Skill->mEffectAreaRatio * DiceSum), 0),
			Skill->mIsPenetration);
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
	ESRPGEnemyMovementRole MovementRole,
	const UBoardActorModel* Self,
	const TArray<FTileIndex>* ReservedDestinations,
	const FTileIndex& PreviousDestination,
	bool bWasDisplaced,
	const FTileIndex& DisplacedFrom,
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

	if (MovementRole != ESRPGEnemyMovementRole::Standard)
	{
		return ChooseRoleDestination(
			Candidates,
			Feasible,
			Origin,
			PlayerTile,
			MovementRole,
			ReservedDestinations,
			PreviousDestination,
			bWasDisplaced,
			DisplacedFrom,
			OUT OutCanCast);
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

FTileIndex USRPGEnemyTurnPlanner::ChooseRoleDestination(
	const TArray<FTileIndex>& Candidates,
	const TArray<FTileIndex>& Feasible,
	const FTileIndex& Origin,
	const FTileIndex& PlayerTile,
	ESRPGEnemyMovementRole MovementRole,
	const TArray<FTileIndex>* ReservedDestinations,
	const FTileIndex& PreviousDestination,
	bool bWasDisplaced,
	const FTileIndex& DisplacedFrom,
	OUT bool& OutCanCast)
{
	OutCanCast = Feasible.IsEmpty() == false;
	const TArray<FTileIndex>& Pool = OutCanCast ? Feasible : Candidates;
	if (Pool.IsEmpty())
	{
		return Origin;
	}

	const FTileIndex MomentumStep(
		FMath::Sign(Origin.mX - DisplacedFrom.mX),
		FMath::Sign(Origin.mY - DisplacedFrom.mY));
	FTileIndex Best = Origin;
	int32 BestScore = TNumericLimits<int32>::Lowest();
	int32 BestMoveCost = TNumericLimits<int32>::Max();
	for (const FTileIndex& Tile : Pool)
	{
		const int32 DeltaX = Tile.mX - PlayerTile.mX;
		const int32 DeltaY = Tile.mY - PlayerTile.mY;
		const int32 PlayerManhattan = FMath::Abs(DeltaX) + FMath::Abs(DeltaY);
		const int32 PlayerChebyshev = FMath::Max(FMath::Abs(DeltaX), FMath::Abs(DeltaY));
		const int32 MoveDeltaX = Tile.mX - Origin.mX;
		const int32 MoveDeltaY = Tile.mY - Origin.mY;
		const int32 MoveCost = FMath::Abs(MoveDeltaX) + FMath::Abs(MoveDeltaY);
		const bool bDiagonalToPlayer = DeltaX != 0 && DeltaY != 0;
		const bool bCardinalToPlayer = DeltaX == 0 || DeltaY == 0;
		const bool bStraightMove = MoveDeltaX == 0 || MoveDeltaY == 0;

		// 공격 가능성이 먼저다. 아직 사거리 밖이면 역할을 유지하면서도 전투에서 이탈하지 않게 접근한다.
		int32 Score = OutCanCast ? 500 : -PlayerManhattan * 18;
		if (ReservedDestinations != nullptr)
		{
			for (const FTileIndex& Reserved : *ReservedDestinations)
			{
				if (Reserved == FTileIndex::Invalid)
				{
					continue;
				}
				const int32 Separation = FMath::Max(
					FMath::Abs(Tile.mX - Reserved.mX),
					FMath::Abs(Tile.mY - Reserved.mY));
				Score -= Separation == 0 ? 10000 : (Separation == 1 ? 34 : 0);
			}
		}

		if (bWasDisplaced)
		{
			// 밀려난 뒤 원래 목적지로 즉시 되감는 선택은 크게 손해다. 현재 착지점을 새 전술 원점으로 사용한다.
			if (Tile == PreviousDestination)
			{
				Score -= 120;
			}
			if (PreviousDestination != FTileIndex::Invalid)
			{
				const int32 PreviousDistanceAtOrigin = TileDistance(Origin, PreviousDestination);
				const int32 PreviousDistanceAtCandidate = TileDistance(Tile, PreviousDestination);
				Score -= FMath::Max(PreviousDistanceAtOrigin - PreviousDistanceAtCandidate, 0) * 22;
			}
			if (DisplacedFrom != FTileIndex::Invalid && MoveCost > 0)
			{
				const int32 MomentumDot = MomentumStep.mX * FMath::Sign(MoveDeltaX)
					+ MomentumStep.mY * FMath::Sign(MoveDeltaY);
				Score += MomentumDot > 0 ? 18 : (MomentumDot < 0 ? -30 : 0);
			}
		}

		switch (MovementRole)
		{
		case ESRPGEnemyMovementRole::Anchor:
			// 버섯은 2칸 전후에서 길목을 차단하며, 이미 좋은 자리라면 불필요하게 왕복하지 않는다.
			Score -= FMath::Abs(PlayerChebyshev - 2) * 24;
			Score += bCardinalToPlayer ? 18 : 0;
			Score -= MoveCost * 7;
			Score += Tile == Origin && OutCanCast ? 24 : 0;
			break;
		case ESRPGEnemyMovementRole::Flanker:
			// 거미는 플레이어와 대각 관계인 측면 슬롯을 우선하고 같은 행/열 정면 대치는 피한다.
			Score -= FMath::Abs(PlayerChebyshev - 2) * 14;
			Score += bDiagonalToPlayer ? 44 : -14;
			Score -= MoveCost * 2;
			Score += Tile != Origin ? 6 : 0;
			break;
		case ESRPGEnemyMovementRole::Slider:
			// 슬라임은 같은 행/열의 돌진 축과 직선 이동을 선호해 다음 충돌각을 만든다.
			Score -= FMath::Abs(PlayerChebyshev - 2) * 15;
			Score += bCardinalToPlayer ? 42 : -20;
			Score += bStraightMove ? 22 : -8;
			Score += FMath::Min(MoveCost, 3) * 3;
			break;
		case ESRPGEnemyMovementRole::Bulwark:
			Score -= FMath::Abs(PlayerChebyshev - 1) * 22;
			Score += bCardinalToPlayer ? 28 : 0;
			Score -= MoveCost * 5;
			break;
		case ESRPGEnemyMovementRole::Lancer:
			Score -= FMath::Abs(PlayerChebyshev - 2) * 12;
			Score += bCardinalToPlayer ? 52 : -22;
			Score += bStraightMove ? 34 : -12;
			break;
		case ESRPGEnemyMovementRole::Bomber:
			Score -= FMath::Abs(PlayerChebyshev - 3) * 18;
			Score += bDiagonalToPlayer ? 18 : 0;
			Score -= MoveCost * 3;
			break;
		case ESRPGEnemyMovementRole::Standard:
		default:
			break;
		}

		const bool bBetter = Score > BestScore
			|| (Score == BestScore && MoveCost < BestMoveCost)
			|| (Score == BestScore && MoveCost == BestMoveCost
				&& (Tile.mX < Best.mX || (Tile.mX == Best.mX && Tile.mY < Best.mY)));
		if (bBetter)
		{
			Best = Tile;
			BestScore = Score;
			BestMoveCost = MoveCost;
		}
	}
	return Best;
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
