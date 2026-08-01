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
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Actor/TileMap/TacticalTileTable.h"

DEFINE_LOG_CATEGORY(LogSRPGEnemyPlanner);

namespace
{
	// @brief 유닛 로그 라벨 (키이름#모델ID)
	FString MakeUnitLabel(const UBoardActorModel* Model)
	{
		return FString::Printf(TEXT("%s#%d"), *Model->GetBoardActorKeyName().ToString(), Model->GetModelId());
	}

	/**
	 * @brief 시전하지 못한 턴의 판단근거 상세 로그
	 * @details
	 * 계획 헤더(위치/AP/성향) -> 스킬 슬롯별 상태 -> 타겟별 시전불가 사유 순서로 출력.
	 * 타겟별 사유는 조준 가능한 {타일,스킬} 조합의 최소 소요 행동력을 근거로 남긴다.
	 */
	void LogNoCastDetails(
		const FString& LogPrefix,
		const FTileIndex& EnemyTile,
		int32 ActionPoint,
		EMoveTendency Tendency,
		const USkillComponentModel* SkillComp,
		const TArray<const UStaticSkillData*>& SkillDatas,
		const TArray<const UUnitModel*>& TargetModels,
		const TArray<FTileIndex>& TargetTiles,
		const FTacticalTileTable& Table)
	{
		// 계획
		UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 계획: @(%d,%d) AP=%d 성향=%s 타겟수=%d"),
			*LogPrefix, EnemyTile.mX, EnemyTile.mY, ActionPoint,
			*StaticEnum<EMoveTendency>()->GetNameStringByValue(static_cast<int64>(Tendency)),
			TargetTiles.Num());

		// 스킬 정보 (비어있지 않은 슬롯별로 나열)
		const TArray<FSkillEntry>& Skills = SkillComp->GetSkills();
		for (int32 Slot = 0; Slot < Skills.Num(); ++Slot)
		{
			if (SkillDatas.IsValidIndex(Slot) && SkillDatas[Slot] != nullptr)
			{
				const UStaticSkillData* Skill = SkillDatas[Slot];
				UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 스킬[%d]=%s 비용=%d 사거리=%d 패턴=%s %s"),
					*LogPrefix, Slot, *Skill->GetName(), Skill->mRequiredActionPoint, Skill->mAimRange,
					*StaticEnum<EAimPattern>()->GetNameStringByValue(static_cast<int64>(Skill->mAimPattern)),
					Skill->mIsIndirect ? TEXT("곡사") : TEXT("직사"));
			}
			else if (Skills[Slot].IsValid() == true)
			{
				UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 스킬[%d]=%s 쿨다운 → 제외"),
					*LogPrefix, Slot, *Skills[Slot].mData->GetName());
			}
		}

		// 타겟별 시전불가 사유 탐색 (왜 이 타겟을 때리지 못했나)
		for (int32 TargetIndex = 0; TargetIndex < TargetTiles.Num(); ++TargetIndex)
		{
			const int32 Distance = Table.GetDistanceToTarget(TargetIndex);
			const FString DistanceText = (Distance == MAX_int32) ? FString(TEXT("도달불가")) : FString::FromInt(Distance);
			const FString TargetLabel = MakeUnitLabel(TargetModels[TargetIndex]);

			// 조준가능한 조합 중 최소행동력 탐색 (없으면: 조준이 안 됨, 있으면: 행동력 부족)
			int32 MinNeed = MAX_int32;
			int32 MinSlot = INDEX_NONE;
			int32 MinMoveCost = 0;
			FTileIndex MinTile = FTileIndex::Invalid;
			for (const FTacticalTileInfo& Tile : Table.GetTacticalTiles())
			{
				for (int32 Slot = 0; Slot < SkillDatas.Num(); ++Slot)
				{
					if (SkillDatas[Slot] == nullptr || Table.IsAimable(Tile, Slot, TargetIndex) == false)
					{
						continue;
					}
					const int32 Need = Tile.mMoveCost + SkillDatas[Slot]->mRequiredActionPoint;
					if (Need < MinNeed)
					{
						MinNeed = Need;
						MinSlot = Slot;
						MinMoveCost = Tile.mMoveCost;
						MinTile = Tile.mIndex;
					}
				}
			}

			if (MinSlot == INDEX_NONE)
			{
				// 도달가능한 어떤 타일에서도 조준 불가
				UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 타겟[%d]=%s@(%d,%d) 경로거리=%s → 시전불가: 조준가능타일 없음(사거리/시야 밖)"),
					*LogPrefix, TargetIndex, *TargetLabel, TargetTiles[TargetIndex].mX, TargetTiles[TargetIndex].mY, *DistanceText);
			}
			else
			{
				// 조준가능하지만 행동력 부족
				UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 타겟[%d]=%s@(%d,%d) 경로거리=%s → 시전불가: 최소소요 = 이동%d(→(%d,%d)) + 시전%d(스킬[%d]) = %d > AP%d"),
					*LogPrefix, TargetIndex, *TargetLabel, TargetTiles[TargetIndex].mX, TargetTiles[TargetIndex].mY, *DistanceText,
					MinMoveCost, MinTile.mX, MinTile.mY, SkillDatas[MinSlot]->mRequiredActionPoint, MinSlot, MinNeed, ActionPoint);
			}
		}
	}
}

TArray<TInstancedStruct<FSRPGCommand>> USRPGEnemyTurnPlanner::PlanTurn(
	UEnemyUnitModel* Enemy,
	const TArray<UUnitModel*>& Players,
	const UTileMapModel* TileMap,
	const FRandomStream& EventStream,
	const FString& LogTag)
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
	if (Enemy == nullptr || TileMap == nullptr)
	{
		UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("[%s] 결정: 행동없음 — 적 또는 타일맵 없음"), *LogTag);
		return Commands;
	}

	// 로그 헤더 문자열: [라운드/턴][유닛키#모델ID]
	const FString LogPrefix = FString::Printf(TEXT("%s[%s]"),
		LogTag.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("[%s]"), *LogTag),
		*MakeUnitLabel(Enemy));

	// 타겟 타일 수집
	TArray<FTileIndex> TargetTiles;
	TArray<const UUnitModel*> TargetModels;
	for (const UUnitModel* Player : Players)
	{
		if (Player != nullptr)
		{
			TargetTiles.Add(Player->GetTileTransform().mIndex);
			TargetModels.Add(Player);
		}
	}
	// 타겟이 하나도 없으면 할 게 없으므로 턴 종료
	if (TargetTiles.IsEmpty())
	{
		UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 결정: 행동없음 — 타겟 없음"), *LogPrefix);
		return Commands;
	}

	// 스킬 컴포넌트 확인: 없으면 할 게 없으므로 턴 종료
	USkillComponentModel* SkillComp = Enemy->GetSkillComponentModel();
	if (SkillComp == nullptr)
	{
		UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 결정: 행동없음 — 스킬 컴포넌트 없음"), *LogPrefix);
		return Commands;
	}

	// 사용 가능한 스킬 수집: 장착돼 있고 쿨다운이 아닌 슬롯만 데이터 채움 (사용불가 슬롯은 nullptr 유지)
	const TArray<FSkillEntry>& Skills = SkillComp->GetSkills();
	TArray<const UStaticSkillData*> SkillDatas;
	SkillDatas.Init(nullptr, Skills.Num());
	bool HasUsableSkill = false;
	for (int32 Index = 0; Index < Skills.Num(); ++Index)
	{
		if (Skills[Index].IsValid() && SkillComp->IsCooldown(Index) == false)
		{
			SkillDatas[Index] = Skills[Index].mData;
			HasUsableSkill = true;
		}
	}
	// 사용 가능한 스킬이 없으면 할 게 없으므로 턴 종료
	if (HasUsableSkill == false)
	{
		UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 결정: 행동없음 — 사용가능 스킬 없음(전부 쿨다운 또는 미장착)"), *LogPrefix);
		return Commands;
	}

	// 속성 컴포넌트 확인: 없으면 할 게 없으므로 턴 종료
	UAttributeSetComponentModel* AttributeSetComp = Enemy->GetAttributeComponentModel();
	if (AttributeSetComp == nullptr)
	{
		UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 결정: 행동없음 — 속성 컴포넌트 없음"), *LogPrefix);
		return Commands;
	}

	// 사용 가능한 액션포인트
	const int32 ActionPoint = FMath::Max(
		AttributeSetComp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetActionPointAttribute()),
		0
	);

	const FTileIndex EnemyTile = Enemy->GetTileTransform().mIndex;

	// 전술 타일 테이블 구성: 이후 판단은 전부 테이블 조회로 처리
	FTacticalTileTable Table;
	Table.Build(TileMap, Enemy, EnemyTile, TargetTiles, SkillDatas, ActionPoint);

	// 목적지 이동비용 조회 (판단근거 로그용)
	auto GetTableMoveCost = [&Table](const FTileIndex& Tile)
	{
		for (const FTacticalTileInfo& Info : Table.GetTacticalTiles())
		{
			if (Info.mIndex == Tile)
			{
				return Info.mMoveCost;
			}
		}
		return 0;
	};

	bool CanCast = false;
	int32 ChosenSkillSlot = INDEX_NONE;
	int32 ChosenTarget = INDEX_NONE;
	FTileIndex Dest = EnemyTile;

	// 시전 가능한 타겟 후보 수집
	TArray<int32> CastableTargets;
	for (int32 TargetIndex = 0; TargetIndex < TargetTiles.Num(); ++TargetIndex)
	{
		if (Table.CanCastToTarget(TargetIndex))
		{
			CastableTargets.Add(TargetIndex);
		}
	}

	if (CastableTargets.IsEmpty() == false)
	{
		//
		// 1단계: 시전 가능한 타겟이 있으면 [타겟 -> 스킬 -> 목적지] 순서로 확정
		//
		// 타겟: 시전 가능한 타겟 중 최근접
		ChosenTarget = ChooseNearestTarget(Table, CastableTargets, EventStream);
		// 스킬: 그 타겟에게 시전 가능한 슬롯 중 랜덤
		const TArray<int32> CastableSlots = Table.GetCastableSkillSlots(ChosenTarget);
		ChosenSkillSlot = CastableSlots[EventStream.RandRange(0, CastableSlots.Num() - 1)];
		// 목적지: 확정된 스킬·타겟을 시전 가능한 타일 중 이동성향대로
		Dest = ChooseDestinationByTendency(Table, Enemy->GetMoveTendency(), ChosenTarget, EnemyTile,
			[&Table, ChosenSkillSlot, ChosenTarget](const FTacticalTileInfo& Tile)
			{
				return Table.IsCastable(Tile, ChosenSkillSlot, ChosenTarget);
			});
		CanCast = true;

		// 판단근거 로그: 시전 턴은 결정 1줄로 요약
		{
			const int32 MoveCost = GetTableMoveCost(Dest);
			const int32 CastCost = SkillDatas[ChosenSkillSlot]->mRequiredActionPoint;
			const FString TargetLabel = MakeUnitLabel(TargetModels[ChosenTarget]);
			if (Dest != EnemyTile)
			{
				UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 이동+시전: 타겟[%d]=%s@(%d,%d) 스킬[%d]=%s, 이동 (%d,%d)→(%d,%d) 비용%d + 시전%d = %d ≤ AP%d"),
					*LogPrefix, ChosenTarget, *TargetLabel, TargetTiles[ChosenTarget].mX, TargetTiles[ChosenTarget].mY,
					ChosenSkillSlot, *SkillDatas[ChosenSkillSlot]->GetName(),
					EnemyTile.mX, EnemyTile.mY, Dest.mX, Dest.mY, MoveCost, CastCost, MoveCost + CastCost, ActionPoint);
			}
			else
			{
				UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 제자리시전: 타겟[%d]=%s@(%d,%d) 스킬[%d]=%s, 이동0 + 시전%d = %d ≤ AP%d"),
					*LogPrefix, ChosenTarget, *TargetLabel, TargetTiles[ChosenTarget].mX, TargetTiles[ChosenTarget].mY,
					ChosenSkillSlot, *SkillDatas[ChosenSkillSlot]->GetName(),
					CastCost, CastCost, ActionPoint);
			}
		}
	}
	else
	{
		// 판단근거 로그: 시전하지 못한 턴은 근거를 상세히 남김
		LogNoCastDetails(LogPrefix, EnemyTile, ActionPoint, Enemy->GetMoveTendency(), SkillComp, SkillDatas, TargetModels, TargetTiles, Table);

		// 폴백의 기준 타겟: 전체 타겟 중 최근접
		TArray<int32> AllTargets;
		for (int32 TargetIndex = 0; TargetIndex < TargetTiles.Num(); ++TargetIndex)
		{
			AllTargets.Add(TargetIndex);
		}
		const int32 NearestTarget = ChooseNearestTarget(Table, AllTargets, EventStream);

		if (Table.HasAnyAimable())
		{
			//
			// 2단계: 이번 턴에는 못 때리므로, 다음 턴 시전을 노리고 조준 가능한 타일로 이동
			//
			Dest = ChooseDestinationByTendency(Table, Enemy->GetMoveTendency(), NearestTarget, EnemyTile,
				[](const FTacticalTileInfo& Tile)
				{
					return Tile.mAimableFlags.Contains(true);
				});

			// 판단근거 로그: 선점이동 결정
			if (Dest != EnemyTile)
			{
				UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 결정: 선점이동 → (%d,%d) 이동%d/AP%d, 다음턴 시전 노림"),
					*LogPrefix, Dest.mX, Dest.mY, GetTableMoveCost(Dest), ActionPoint);
			}
			else
			{
				UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 결정: 제자리대기 — 이동해도 개선 없음"), *LogPrefix);
			}
		}
		else
		{
			//
			// 3단계: 어디로 가든 조준이 안 되면, 최근접 타겟에게 최대한 접근
			//
			Dest = ChooseApproachDestination(Table, NearestTarget, EnemyTile);

			// 판단근거 로그: 접근이동 결정
			if (Dest != EnemyTile)
			{
				UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 결정: 접근이동 → 타겟[%d]=%s 방향 (%d,%d) 이동%d/AP%d"),
					*LogPrefix, NearestTarget, *MakeUnitLabel(TargetModels[NearestTarget]), Dest.mX, Dest.mY, GetTableMoveCost(Dest), ActionPoint);
			}
			else
			{
				UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 결정: 제자리대기 — 접근 경로 없음"), *LogPrefix);
			}
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
		CastRef.mSkillIndex = ChosenSkillSlot;
		CastRef.mTargetIndex = TargetTiles[ChosenTarget];
		AddAction(MoveTemp(Cast));
	}

	return Commands;
}

int32 USRPGEnemyTurnPlanner::ChooseNearestTarget(
	const FTacticalTileTable& Table,
	const TArray<int32>& CandidateTargets,
	const FRandomStream& EventStream)
{
	// 후보가 없으면 선택할 것도 없음
	if (CandidateTargets.IsEmpty())
	{
		return INDEX_NONE;
	}

	// 후보 중 가장 짧은 경로 거리 탐색
	int32 BestDistance = MAX_int32;
	for (const int32 TargetIndex : CandidateTargets)
	{
		BestDistance = FMath::Min(BestDistance, Table.GetDistanceToTarget(TargetIndex));
	}

	// 최근접 동률 후보 수집
	TArray<int32> NearestTargets;
	for (const int32 TargetIndex : CandidateTargets)
	{
		if (Table.GetDistanceToTarget(TargetIndex) == BestDistance)
		{
			NearestTargets.Add(TargetIndex);
		}
	}

	// 동률이 있을때만 랜덤 돌린다.
	return (NearestTargets.Num() == 1)
		? NearestTargets[0]
		: NearestTargets[EventStream.RandRange(0, NearestTargets.Num() - 1)];
}

FTileIndex USRPGEnemyTurnPlanner::ChooseDestinationByTendency(
	const FTacticalTileTable& Table,
	EMoveTendency Tendency,
	int32 ReferenceTarget,
	const FTileIndex& Origin,
	TFunctionRef<bool(const FTacticalTileInfo&)> Filter)
{
	switch (Tendency)
	{
	case EMoveTendency::MoveClose:
		// 근접 성향: 기준 타겟과의 거리 최소 -> 이동비용 최소
		return Table.PickTile(
			Filter,
			[ReferenceTarget](const FTacticalTileInfo& Tile)
			{
				return static_cast<int64>(Tile.mTargetDistances[ReferenceTarget]);
			},
			[](const FTacticalTileInfo& Tile)
			{
				return static_cast<int64>(Tile.mMoveCost);
			},
			Origin);

	case EMoveTendency::MoveAway:
		// 원거리 성향: 무조건 먼 게 좋은 게 아니라, '최근접' 타겟이 '최대한' 멀어야 함
		return Table.PickTile(
			Filter,
			[&Table](const FTacticalTileInfo& Tile)
			{
				return -static_cast<int64>(Table.GetNearestTargetDistance(Tile));
			},
			[](const FTacticalTileInfo& Tile)
			{
				return static_cast<int64>(Tile.mMoveCost);
			},
			Origin);

	case EMoveTendency::HoldRange:
	default:
		// 등거리 성향: 최대한 제자리를 유지하려고 하지만, 조준이 안된다면 최근접 타겟과의 거리를 최대로 하는 지점으로 이동
		return Table.PickTile(
			Filter,
			[](const FTacticalTileInfo& Tile)
			{
				return static_cast<int64>(Tile.mMoveCost);
			},
			[&Table](const FTacticalTileInfo& Tile)
			{
				return -static_cast<int64>(Table.GetNearestTargetDistance(Tile));
			},
			Origin);
	}
}

FTileIndex USRPGEnemyTurnPlanner::ChooseApproachDestination(
	const FTacticalTileTable& Table,
	int32 ReferenceTarget,
	const FTileIndex& Origin)
{
	// 기준 타겟과의 거리 최소 -> 이동비용 최소 (도달 가능한 모든 타일이 후보라 필터 없음)
	return Table.PickTile(
		[](const FTacticalTileInfo&)
		{
			return true;
		},
		[ReferenceTarget](const FTacticalTileInfo& Tile)
		{
			return static_cast<int64>(Tile.mTargetDistances[ReferenceTarget]);
		},
		[](const FTacticalTileInfo& Tile)
		{
			return static_cast<int64>(Tile.mMoveCost);
		},
		Origin);
}
