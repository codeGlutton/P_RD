/*****************************************************************//**
 * @file   SRPGEnemyTurnPlanner.cpp
 * @brief  적 한 턴의 행동을 계산하는 모델 레이어 플래너 구현
 * @author 이문환
 * @date   2026-06-30
 *********************************************************************/

#include "SRPGFramework/SRPGEnemyTurnPlanner.h"

#include "SRPGFramework/SRPGMoveAction.h"
#include "Component/BoardMovementComponent/UnitMovementComponentModel.h"
#include "SRPGFramework/SRPGSkillAction.h"
#include "SRPGFramework/SRPGTurnEndAction.h"

#include "Pawn/Enemy/EnemyUnitModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Component/SkillComponent/UnitSkillComponentModel.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Actor/TileMap/TacticalTileTable.h"

#include "Simulation/Logger/EventLogger.h"

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
	 * 계획 헤더(위치/AP/성향) -> 확정 스킬 정보 -> 타겟별 시전불가 사유 순서로 출력.
	 * 타겟별 사유는 확정 스킬로 조준 가능한 타일의 최소 소요 행동력을 근거로 남긴다.
	 */
	void LogNoCastDetails(
		const FString& LogPrefix,
		const FTileIndex& EnemyTile,
		int32 ActionPoint,
		EMoveTendency Tendency,
		int32 ChosenSkillSlot,
		const UStaticUnitSkillData* ChosenSkill,
		const TArray<const UUnitModel*>& TargetModels,
		const TArray<FTileIndex>& TargetTiles,
		const FTacticalTileTable& Table)
	{
		// 계획
		UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 계획: @(%d,%d) AP=%d 성향=%s 타겟수=%d"),
			*LogPrefix, EnemyTile.mX, EnemyTile.mY, ActionPoint,
			*StaticEnum<EMoveTendency>()->GetNameStringByValue(static_cast<int64>(Tendency)),
			TargetTiles.Num());

		// 확정 스킬 정보
		UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 스킬[%d]=%s 비용=%d 사거리=%d 패턴=%s %s"),
			*LogPrefix, ChosenSkillSlot, *ChosenSkill->GetName(), ChosenSkill->mRequiredActionPoint, ChosenSkill->mAimRange,
			*StaticEnum<EAimPattern>()->GetNameStringByValue(static_cast<int64>(ChosenSkill->mAimPattern)),
			(ChosenSkill->mAimBlockerMask == 0) ? TEXT("곡사") : TEXT("직사"));

		// 타겟별 시전불가 사유 탐색 (왜 이 타겟을 때리지 못했나)
		for (int32 TargetIndex = 0; TargetIndex < TargetTiles.Num(); ++TargetIndex)
		{
			const int32 Distance = Table.GetDistanceToTarget(TargetIndex);
			const FString DistanceText = (Distance == MAX_int32) ? FString(TEXT("도달불가")) : FString::FromInt(Distance);
			const FString TargetLabel = MakeUnitLabel(TargetModels[TargetIndex]);

			// 조준가능한 타일 중 최소행동력 탐색 (없으면: 조준이 안 됨, 있으면: 행동력 부족)
			int32 MinNeed = MAX_int32;
			int32 MinMoveCost = 0;
			FTileIndex MinTile = FTileIndex::Invalid;
			for (const FTacticalTileInfo& Tile : Table.GetTacticalTiles())
			{
				if (Table.IsAimable(Tile, ChosenSkillSlot, TargetIndex) == false)
				{
					continue;
				}

				const int32 Need = Tile.mMoveCost + ChosenSkill->mRequiredActionPoint;
				if (Need < MinNeed)
				{
					MinNeed = Need;
					MinMoveCost = Tile.mMoveCost;
					MinTile = Tile.mIndex;
				}
			}

			if (MinNeed == MAX_int32)
			{
				// 도달가능한 어떤 타일에서도 조준 불가
				UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 타겟[%d]=%s@(%d,%d) 경로거리=%s → 시전불가: 조준가능타일 없음(사거리/시야 밖)"),
					*LogPrefix, TargetIndex, *TargetLabel, TargetTiles[TargetIndex].mX, TargetTiles[TargetIndex].mY, *DistanceText);
			}
			else
			{
				// 조준가능하지만 행동력 부족
				UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 타겟[%d]=%s@(%d,%d) 경로거리=%s → 시전불가: 최소소요 = 이동%d(→(%d,%d)) + 시전%d = %d > AP%d"),
					*LogPrefix, TargetIndex, *TargetLabel, TargetTiles[TargetIndex].mX, TargetTiles[TargetIndex].mY, *DistanceText,
					MinMoveCost, MinTile.mX, MinTile.mY, ChosenSkill->mRequiredActionPoint, MinNeed, ActionPoint);
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

	// 스킬 확정: 배치를 보기 전에 우선순위와 사용 가능 여부만으로 이번 턴의 스킬을 정함
	// (이후 배치 판단은 이 스킬 하나로만 하고, 시전이 안 돼도 다른 스킬로 바꾸지 않음)
	const TArray<FSkillEntry>& Skills = SkillComp->GetSkills();
	const int32 ChosenSkillSlot = ChooseSkillByPriority(Enemy, SkillComp, EventStream);
	// 사용 가능한 스킬이 없으면 할 게 없으므로 턴 종료
	if (ChosenSkillSlot == INDEX_NONE)
	{
		UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 결정: 행동없음 — 사용가능 스킬 없음(전부 쿨다운/행동력 부족 또는 미장착)"), *LogPrefix);
		return Commands;
	}
	const UStaticUnitSkillData* ChosenSkill = StaticCast<const UStaticUnitSkillData*>(Skills[ChosenSkillSlot].mData);
	const ESkillPriority ChosenPriority = Enemy->GetSkillPriority(ChosenSkillSlot);
	UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 스킬확정: 스킬[%d]=%s 우선순위=%s"),
		*LogPrefix, ChosenSkillSlot, *ChosenSkill->GetName(),
		*StaticEnum<ESkillPriority>()->GetNameStringByValue(static_cast<int64>(ChosenPriority)));

	// 판단근거 로그: 나머지 슬롯이 밀린 이유 (쿨다운 / 행동력 부족 / 기절 / 우선순위 낮음 / 동순위 추첨 탈락)
	{
		const UUnitSkillComponentModel* UnitSkillComp = Cast<UUnitSkillComponentModel>(SkillComp);
		for (int32 Slot = 0; Slot < Skills.Num(); ++Slot)
		{
			if (Slot == ChosenSkillSlot || Skills[Slot].IsValid() == false)
			{
				continue;
			}

			const TCHAR* Reason = nullptr;
			if (SkillComp->IsCooldown(Slot) == true)
			{
				Reason = TEXT("쿨다운");
			}
			else if (UnitSkillComp != nullptr && UnitSkillComp->HasRequiredActionPoint(Slot) == false)
			{
				Reason = TEXT("행동력 부족");
			}
			else if (SkillComp->CanActiveSkill(Slot) == false)
			{
				Reason = TEXT("기절 등 사용 불가");
			}
			else if (Enemy->GetSkillPriority(Slot) == ChosenPriority)
			{
				Reason = TEXT("동순위 추첨 탈락");
			}
			else
			{
				Reason = TEXT("우선순위 낮음");
			}

			UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 스킬[%d]=%s 우선순위=%s → 제외: %s"),
				*LogPrefix, Slot, *Skills[Slot].mData->GetName(),
				*StaticEnum<ESkillPriority>()->GetNameStringByValue(static_cast<int64>(Enemy->GetSkillPriority(Slot))),
				Reason);
		}
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
		AttributeSetComp->GetAttributeCurrentValue(UUnitAttributeSet::GetActionPointAttribute()),
		0
	);

	const FTileIndex EnemyTile = Enemy->GetTileTransform().mIndex;

	// 속박 등으로 이동 불가면 이동 예산을 0으로 -> 도달 범위가 제자리로 줄어 제자리 스킬만 계획
	// (시전 예산은 ActionPoint를 따로 넘기므로 이동 불가여도 스킬 비용 판정에는 영향 없음)
	const UUnitMovementComponentModel* MovementCompModel = Cast<UUnitMovementComponentModel>(Enemy->GetBoardMovementComponentModel());
	const int32 MoveBudget = (MovementCompModel != nullptr && MovementCompModel->IsMoveable() == false)
		? 0
		: ActionPoint;

	// 전술 타일 테이블 구성: 이후 판단은 전부 테이블 조회로 처리
	// 확정 스킬 하나만 올리되, 슬롯 인덱스를 그대로 쓰기 위해 배열 크기는 슬롯 수 유지
	// 자기 버프는 조준 판단이 없으므로 비워 두고, 시전 비용을 먼저 뗀 예산으로만 이동
	const bool bSpell = (ChosenSkill->mSkillType == ESkillType::Spell);
	TArray<const UStaticUnitSkillData*> SkillDatas;
	SkillDatas.Init(nullptr, Skills.Num());
	if (bSpell == false)
	{
		SkillDatas[ChosenSkillSlot] = ChosenSkill;
	}
	const int32 TableMoveBudget = bSpell
		? FMath::Max(MoveBudget - ChosenSkill->mRequiredActionPoint, 0)
		: MoveBudget;
	FTacticalTileTable Table;
	Table.Build(TileMap, Enemy, EnemyTile, TargetTiles, SkillDatas, TableMoveBudget, ActionPoint);

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
	int32 ChosenTarget = INDEX_NONE;
	FTileIndex Dest = EnemyTile;

	// 이동 판단의 기준 타겟: 전체 타겟 중 최근접
	TArray<int32> AllTargets;
	for (int32 TargetIndex = 0; TargetIndex < TargetTiles.Num(); ++TargetIndex)
	{
		AllTargets.Add(TargetIndex);
	}
	const int32 NearestTarget = ChooseNearestTarget(Table, AllTargets, EventStream);

	// 확정 스킬로 시전 가능한 타겟 후보 수집 (버프는 테이블에 스킬이 없으므로 항상 비어 있음)
	TArray<int32> CastableTargets;
	for (int32 TargetIndex = 0; TargetIndex < TargetTiles.Num(); ++TargetIndex)
	{
		if (Table.CanCastToTarget(TargetIndex))
		{
			CastableTargets.Add(TargetIndex);
		}
	}

	if (bSpell == true)
	{
		//
		// 자기 버프: 남는 예산으로 이동 성향대로 자리를 잡고 거기서 시전 (도달 가능한 모든 타일이 후보)
		//
		Dest = ChooseDestinationByTendency(Table, Enemy->GetMoveTendency(), NearestTarget, EnemyTile,
			[](const FTacticalTileInfo&)
			{
				return true;
			});
		CanCast = true;

		// 판단근거 로그: 버프 결정
		if (Dest != EnemyTile)
		{
			UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 이동+버프: 스킬[%d]=%s 비용%d, 이동 (%d,%d)→(%d,%d) 비용%d (이동예산 %d)"),
				*LogPrefix, ChosenSkillSlot, *ChosenSkill->GetName(), ChosenSkill->mRequiredActionPoint,
				EnemyTile.mX, EnemyTile.mY, Dest.mX, Dest.mY, GetTableMoveCost(Dest), TableMoveBudget);
		}
		else
		{
			UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 제자리버프: 스킬[%d]=%s 비용%d (이동예산 %d)"),
				*LogPrefix, ChosenSkillSlot, *ChosenSkill->GetName(), ChosenSkill->mRequiredActionPoint, TableMoveBudget);
		}
	}
	else if (CastableTargets.IsEmpty() == false)
	{
		//
		// 공격 시전: 시전 가능한 타겟이 있으면 [타겟 -> 목적지] 순서로 확정 (스킬은 이미 확정)
		//
		// 타겟: 시전 가능한 타겟 중 최근접
		ChosenTarget = ChooseNearestTarget(Table, CastableTargets, EventStream);
		// 목적지: 확정 스킬로 그 타겟에게 시전 가능한 타일 중 이동성향대로
		Dest = ChooseDestinationByTendency(Table, Enemy->GetMoveTendency(), ChosenTarget, EnemyTile,
			[&Table, ChosenSkillSlot, ChosenTarget](const FTacticalTileInfo& Tile)
			{
				return Table.IsCastable(Tile, ChosenSkillSlot, ChosenTarget);
			});
		CanCast = true;

		// 판단근거 로그: 시전 턴은 결정 1줄로 요약
		{
			const int32 MoveCost = GetTableMoveCost(Dest);
			const int32 CastCost = ChosenSkill->mRequiredActionPoint;
			const FString TargetLabel = MakeUnitLabel(TargetModels[ChosenTarget]);
			if (Dest != EnemyTile)
			{
				UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 이동+시전: 타겟[%d]=%s@(%d,%d) 스킬[%d]=%s, 이동 (%d,%d)→(%d,%d) 비용%d + 시전%d = %d ≤ AP%d"),
					*LogPrefix, ChosenTarget, *TargetLabel, TargetTiles[ChosenTarget].mX, TargetTiles[ChosenTarget].mY,
					ChosenSkillSlot, *ChosenSkill->GetName(),
					EnemyTile.mX, EnemyTile.mY, Dest.mX, Dest.mY, MoveCost, CastCost, MoveCost + CastCost, ActionPoint);
			}
			else
			{
				UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 제자리시전: 타겟[%d]=%s@(%d,%d) 스킬[%d]=%s, 이동0 + 시전%d = %d ≤ AP%d"),
					*LogPrefix, ChosenTarget, *TargetLabel, TargetTiles[ChosenTarget].mX, TargetTiles[ChosenTarget].mY,
					ChosenSkillSlot, *ChosenSkill->GetName(),
					CastCost, CastCost, ActionPoint);
			}
		}
	}
	else
	{
		//
		// 공격 불가: 다른 스킬로 바꾸지 않고 이동만 (예상 스킬과 실제 스킬 일치 보장)
		//
		// 판단근거 로그: 시전하지 못한 턴은 근거를 상세히 남김
		LogNoCastDetails(LogPrefix, EnemyTile, ActionPoint, Enemy->GetMoveTendency(), ChosenSkillSlot, ChosenSkill, TargetModels, TargetTiles, Table);

		if (Table.HasAnyAimable())
		{
			// 이번 턴에는 못 때리므로, 다음 턴 시전을 노리고 조준 가능한 타일로 이동
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
			// 어디로 가든 조준이 안 되면, 최근접 타겟에게 최대한 접근
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
		// 시전 커맨드에는 조준 타일과 스킬 슬롯만 담는다.
		// 실제 효과 타일은 실행 시점에 스킬 컴포넌트가 조준 타일 기준으로 계산한다.
		// Single 패턴은 조준 타일이 시전자 자기 칸이므로 타겟 칸이 아닌 목적지를 넣는다.
		// 자기 버프는 타겟 없이 시전하므로 역시 목적지를 넣는다.
		const bool bAimSelf = (ChosenSkill->mAimPattern == EAimPattern::Single) || (ChosenTarget == INDEX_NONE);
		TInstancedStruct<FSRPGCommand> Cast;
		Cast.InitializeAs<FSRPGSkillCastCommand>();
		FSRPGSkillCastCommand& CastRef = Cast.GetMutable<FSRPGSkillCastCommand>();
		CastRef.mSkillIndex = ChosenSkillSlot;
		CastRef.mTargetIndex = bAimSelf ? Dest : TargetTiles[ChosenTarget];
		AddAction(MoveTemp(Cast));

		// 로그 작성

		UEventLogger* Logger = GetWorldEventLogger(AttributeSetComp);
		if (Logger != nullptr)
		{
			FSRPGAIPlanLog Log;
			Log.mSkillIndex = ChosenSkillSlot;

			Logger->LogAIPlan(Log);
		}
	}

	return Commands;
}

int32 USRPGEnemyTurnPlanner::ChooseSkillByPriority(
	const UEnemyUnitModel* Enemy,
	const USkillComponentModel* SkillComp,
	const FRandomStream& EventStream)
{
	// 사용 가능한 슬롯 중 최상위 우선순위 후보 수집 (열거값이 작을수록 높음)
	const TArray<FSkillEntry>& Skills = SkillComp->GetSkills();
	ESkillPriority BestPriority = ESkillPriority::Lowest;
	TArray<int32> Candidates;
	for (int32 Slot = 0; Slot < Skills.Num(); ++Slot)
	{
		if (Skills[Slot].IsValid() == false || SkillComp->CanActiveSkill(Slot) == false)
		{
			continue;
		}

		const ESkillPriority Priority = Enemy->GetSkillPriority(Slot);
		if (Priority < BestPriority)
		{
			// 더 높은 우선순위 발견: 후보 교체
			BestPriority = Priority;
			Candidates.Reset();
			Candidates.Add(Slot);
		}
		else if (Priority == BestPriority)
		{
			Candidates.Add(Slot);
		}
	}

	// 후보가 없으면 INDEX_NONE, 동순위가 여럿일 때만 랜덤
	if (Candidates.IsEmpty())
	{
		return INDEX_NONE;
	}
	return (Candidates.Num() == 1)
		? Candidates[0]
		: Candidates[EventStream.RandRange(0, Candidates.Num() - 1)];
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
