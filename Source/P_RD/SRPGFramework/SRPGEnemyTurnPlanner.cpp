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
	// Evaluate actual effect footprints, including empty aimed tiles and each phase's team filter.
	// This is geometric coverage, not damage simulation; it consumes no gameplay randomness.
	int32 CountCoveredTargets(const UTileMapModel* Map, const UEnemyUnitModel* Enemy,
		const UStaticUnitSkillData* Skill, const FTileIndex& From, const FTileIndex& Aim,
		const TArray<const UUnitModel*>& Targets)
	{
		TArray<FTileIndex> EffectTiles;
		for (const FTileIndex& Center : Map->GetTargetTiles(From, Aim, Skill->mTargetPattern))
		{
			for (const FTileIndex& Tile : Map->GetEffectTiles(Center, Skill->mEffectPattern,
				Skill->mEffectArea, static_cast<ETileLayerFlag>(Skill->mEffectBlockerMask), Enemy))
			{
				EffectTiles.AddUnique(Tile);
			}
		}
		TSet<const UUnitModel*> Covered;
		for (const FSkillPhaseLayer& Phase : Skill->mSkillPhaseLayers)
		{
			const auto Tiles = Phase.FilterTileIndexes(From, EffectTiles);
			const auto CombatTargets = Phase.FilterCombatTargets(Map, Enemy, Tiles);
			for (const UUnitModel* Target : Targets)
			{
				if (CombatTargets.Contains(const_cast<UUnitModel*>(Target))) Covered.Add(Target);
			}
		}
		return Covered.Num();
	}

	bool ChooseMostTargets(const FTacticalTileTable& Table, const UTileMapModel* Map,
		const UEnemyUnitModel* Enemy, const UStaticUnitSkillData* Skill,
		const TArray<const UUnitModel*>& Targets, int32 ActionPoint,
		TMap<FTileIndex, FTileIndex>& BestAims)
	{
		int32 BestCount = 0;
		for (const FTacticalTileInfo& Tile : Table.GetTacticalTiles())
		{
			if (Tile.mMoveCost + Skill->mRequiredActionPoint > ActionPoint) continue;
			const auto AimTiles = Map->GetAimableTiles(Tile.mIndex, Skill->mAimRange,
				Skill->mAimPattern, Skill->mCanAimBoardActor,
				static_cast<ETileLayerFlag>(Skill->mAimBlockerMask), nullptr, Enemy);
			for (const FTileIndex& Aim : AimTiles)
			{
				const int32 Count = CountCoveredTargets(Map, Enemy, Skill, Tile.mIndex, Aim, Targets);
				if (Count == 0 || Count < BestCount) continue;
				if (Count > BestCount)
				{
					BestCount = Count;
					BestAims.Reset();
				}
				// Stable first aim per destination; movement tendency breaks destination ties.
				if (!BestAims.Contains(Tile.mIndex)) BestAims.Add(Tile.mIndex, Aim);
			}
		}
		return BestCount > 0;
	}

	/**
	 * @brief 이동 스킬의 착지 타일 선택 결과
	 */
	struct FMoveAimChoice
	{
		// @brief 착지 타일 (후보가 없으면 Invalid)
		FTileIndex mTile = FTileIndex::Invalid;
		// @brief 착지 타일에서 기준 타겟 타일까지 경로 거리 (경로 없음은 MAX_int32)
		int32 mPathDistance = MAX_int32;
		// @brief 착지 타일에서 기준 타겟 타일까지 맨해튼 거리
		int32 mManhattanDistance = MAX_int32;
		// @brief 조준 가능한 빈 타일 수
		int32 mCandidateCount = 0;
	};

	/**
	 * @brief 이동 스킬의 착지 타일 선택
	 * @details
	 * 현재 타일에서 조준 가능한 타일 중 기준 타겟 타일에 가장 가까운 타일 선택.
	 * 1순위 경로 거리, 2순위 맨해튼 거리, 완전 동률은 먼저 나온 타일 (난수 소모 없음).
	 * 조준 집합은 실행 직전 검증(SRPGSkillAction)과 같은 스킬 컴포넌트 함수로 구해서 결과 일치 보장.
	 */
	FMoveAimChoice ChooseMoveAimTile(const UTileMapModel* Map, const UEnemyUnitModel* Enemy,
		const USkillComponentModel* SkillComp, int32 SkillSlot, const FTileIndex& ReferenceTile)
	{
		FMoveAimChoice Choice;

		// 현재 타일 기준 조준 가능 타일 (DA의 사거리/패턴/빈 타일 조건/차폐 적용)
		const TArray<FTileIndex> AimTiles = SkillComp->GetAimableTiles(Map, SkillSlot);
		Choice.mCandidateCount = AimTiles.Num();

		// 기준 타겟 타일에서 각 타일까지의 경로 거리장 (자기 자신은 자리를 비울 예정이므로 차단에서 제외)
		const TArray<int32> DistanceField = Map->GetDistanceField(ReferenceTile, Enemy);

		for (const FTileIndex& Tile : AimTiles)
		{
			// 경로 없음(-1)은 MAX_int32로 통일 (전술 타일 테이블과 같은 규칙)
			const int32 RawDistance = DistanceField[Map->TileIndexToLinearIndex(Tile)];
			const int32 PathDistance = (RawDistance >= 0) ? RawDistance : MAX_int32;
			const int32 ManhattanDistance = FTileIndex::ManhattanDistance(Tile, ReferenceTile);

			// 1순위 경로 거리, 2순위 맨해튼 거리. 완전 동률이면 먼저 나온 타일 유지
			const bool bBetter = (PathDistance != Choice.mPathDistance)
				? (PathDistance < Choice.mPathDistance)
				: (ManhattanDistance < Choice.mManhattanDistance);
			if (bBetter == true)
			{
				Choice.mTile = Tile;
				Choice.mPathDistance = PathDistance;
				Choice.mManhattanDistance = ManhattanDistance;
			}
		}
		return Choice;
	}

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
			else if (MinNeed <= ActionPoint)
			{
				UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 타겟[%d]=%s → 조준/AP 가능하지만 대상 정책 또는 효과 범위 조건으로 시전하지 않음"),
					*LogPrefix, TargetIndex, *TargetLabel);
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
		if (Player != nullptr &&
			(!Player->GetAttributeComponentModel() || !Player->GetAttributeComponentModel()->HasMatchingGameplayTag(
				EffectTags::GameplayEffect_ActorState_Dead)))
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

	// 로그 작성

	UEventLogger* Logger = GetWorldEventLogger(SkillComp);
	if (Logger != nullptr)
	{
		FSRPGAIPlanLog Log;
		Log.mSkillIndex = ChosenSkillSlot;

		Logger->LogAIPlan(Log);
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
	const int32 MoveBudget = (MovementCompModel != nullptr && MovementCompModel->CanSelfMove() == false)
		? 0
		: ActionPoint;

	// 전술 타일 테이블 구성: 이후 판단은 전부 테이블 조회로 처리
	// 확정 스킬 하나만 올리되, 슬롯 인덱스를 그대로 쓰기 위해 배열 크기는 슬롯 수 유지
	// 자기 버프는 조준 판단이 없으므로 비워 두고, 시전 비용을 먼저 뗀 예산으로만 이동
	// 이동 스킬은 빈 타일을 조준하므로 타겟 타일 기준인 테이블 조준 판정에서 제외 (제자리 시전이라 이동 예산은 그대로)
	const bool bSpell = (ChosenSkill->mSkillType == ESkillType::Spell);
	const bool bMove = (ChosenSkill->mSkillType == ESkillType::Move);
	TArray<const UStaticUnitSkillData*> SkillDatas;
	SkillDatas.Init(nullptr, Skills.Num());
	if (bSpell == false && bMove == false)
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
	FTileIndex AreaAim = FTileIndex::Invalid;

	const bool bTargetPolicyOverrided = Enemy->GetTargetPolicyOverride(ChosenSkillSlot).mIsOverrided;
	const FEnemyTargetPolicy& ChosenTargetPolicy =
		bSpell == true ?
		FEnemyTargetPolicy::Default :
		(bTargetPolicyOverrided == true ? Enemy->GetTargetPolicyOverride(ChosenSkillSlot).mTargetPolicy : ChosenSkill->mTargetPolicy);

	const bool bChasePreferred = !bSpell && ChosenTargetPolicy.mPriority != EEnemyTargetPriority::MostTargets &&
		ChosenTargetPolicy.mUnreachableBehavior == EEnemyUnreachableTargetBehavior::ChasePreferred;

	// 이동 판단의 기준 타겟도 DA 우선순위를 사용한다. 자기 버프는 기존 거리 기준 유지.
	TArray<int32> AllTargets;
	for (int32 TargetIndex = 0; TargetIndex < TargetTiles.Num(); ++TargetIndex)
	{
		AllTargets.Add(TargetIndex);
	}
	const int32 ReferenceTarget = ChooseTargetByPriority(
		bSpell ? FEnemyTargetPolicy() : ChosenTargetPolicy,
		Table, TargetModels, AllTargets, EventStream);

	// 확정 스킬로 시전 가능한 타겟 후보 수집 (버프와 이동 스킬은 테이블에 스킬이 없으므로 항상 비어 있음)
	TArray<int32> CastableTargets;
	for (int32 TargetIndex = 0; TargetIndex < TargetTiles.Num(); ++TargetIndex)
	{
		if (Table.CanCastToTarget(TargetIndex))
		{
			CastableTargets.Add(TargetIndex);
		}
	}

	TMap<FTileIndex, FTileIndex> BestAreaAims;
	const bool bAreaCast = !bSpell && ChosenTargetPolicy.mPriority == EEnemyTargetPriority::MostTargets &&
		ChooseMostTargets(Table, TileMap, Enemy, ChosenSkill, TargetModels, ActionPoint, BestAreaAims);
	if (bChasePreferred)
	{
		// Rank all living targets first, then only permit a cast at that target.
		CastableTargets.RemoveAll([ReferenceTarget](int32 Index) { return Index != ReferenceTarget; });
	}

	// 이동 스킬: 기준 타겟 타일에 가장 가까운 빈 타일을 착지 타일로 선택 (후보가 없으면 Invalid)
	const FMoveAimChoice MoveAim = bMove
		? ChooseMoveAimTile(TileMap, Enemy, SkillComp, ChosenSkillSlot, TargetTiles[ReferenceTarget])
		: FMoveAimChoice();

	if (bMove == true && MoveAim.mTile != FTileIndex::Invalid)
	{
		//
		// 이동 스킬: 걷지 않고 현재 타일에서 착지 타일을 조준해 시전 (후보가 없으면 아래 시전 불가 분기로)
		//
		AreaAim = MoveAim.mTile;
		CanCast = true;

		// 판단근거 로그: 누구를 기준으로 어느 타일을 골랐는지
		const FString PathDistanceText = (MoveAim.mPathDistance == MAX_int32) ? FString(TEXT("도달불가")) : FString::FromInt(MoveAim.mPathDistance);
		UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 이동스킬: 기준타겟[%d]=%s@(%d,%d) 대상우선순위=%s, 스킬[%d]=%s (%d,%d)→(%d,%d) 경로거리=%s 맨해튼=%d 후보=%d, 시전%d ≤ AP%d"),
			*LogPrefix, ReferenceTarget, *MakeUnitLabel(TargetModels[ReferenceTarget]),
			TargetTiles[ReferenceTarget].mX, TargetTiles[ReferenceTarget].mY,
			*StaticEnum<EEnemyTargetPriority>()->GetNameStringByValue(static_cast<int64>(ChosenTargetPolicy.mPriority)),
			ChosenSkillSlot, *ChosenSkill->GetName(),
			EnemyTile.mX, EnemyTile.mY, MoveAim.mTile.mX, MoveAim.mTile.mY,
			*PathDistanceText, MoveAim.mManhattanDistance, MoveAim.mCandidateCount,
			ChosenSkill->mRequiredActionPoint, ActionPoint);
	}
	else if (bSpell == true)
	{
		//
		// 자기 버프: 남는 예산으로 이동 성향대로 자리를 잡고 거기서 시전 (도달 가능한 모든 타일이 후보)
		//
		Dest = ChooseDestinationByTendency(Table, Enemy->GetMoveTendency(), ReferenceTarget, EnemyTile,
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
	else if (bAreaCast)
	{
		Dest = ChooseDestinationByTendency(Table, Enemy->GetMoveTendency(), ReferenceTarget, EnemyTile,
			[&BestAreaAims](const FTacticalTileInfo& Tile) { return BestAreaAims.Contains(Tile.mIndex); });
		AreaAim = BestAreaAims.FindChecked(Dest);
		CanCast = true;
		UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 밀집공격: 이동(%d,%d) 조준(%d,%d)"),
			*LogPrefix, Dest.mX, Dest.mY, AreaAim.mX, AreaAim.mY);
	}
	else if (CastableTargets.IsEmpty() == false && ChosenTargetPolicy.mPriority != EEnemyTargetPriority::MostTargets)
	{
		//
		// 공격 시전: 시전 가능한 타겟이 있으면 [타겟 -> 목적지] 순서로 확정 (스킬은 이미 확정)
		//
		// 높은 우선순위 대상이 사거리/AP 조건 밖이어도 공격 가능한 다른 대상에게 시전한다.
		ChosenTarget = bChasePreferred ? ReferenceTarget :
			ChooseTargetByPriority(ChosenTargetPolicy, Table, TargetModels, CastableTargets, EventStream);
		UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 대상우선순위=%s 선택=%s"), *LogPrefix,
			*StaticEnum<EEnemyTargetPriority>()->GetNameStringByValue(static_cast<int64>(ChosenTargetPolicy.mPriority)),
			*MakeUnitLabel(TargetModels[ChosenTarget]));
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
		if (bMove == true)
		{
			// 이동 스킬은 타겟 타일을 조준하지 않으므로 타겟별 조준 사유 대신 빈 타일 후보가 없었다는 사실만 남김
			UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 이동스킬 시전불가: 스킬[%d]=%s @(%d,%d) AP=%d 사거리=%d 패턴=%s → 조준 범위 안에 빈 타일 없음"),
				*LogPrefix, ChosenSkillSlot, *ChosenSkill->GetName(), EnemyTile.mX, EnemyTile.mY, ActionPoint, ChosenSkill->mAimRange,
				*StaticEnum<EAimPattern>()->GetNameStringByValue(static_cast<int64>(ChosenSkill->mAimPattern)));
		}
		else
		{
			LogNoCastDetails(LogPrefix, EnemyTile, ActionPoint, Enemy->GetMoveTendency(), ChosenSkillSlot, ChosenSkill, TargetModels, TargetTiles, Table);
		}

		// Keep legacy Nearest/AttackAvailable movement and RNG behaviour exactly intact.
		const bool bLegacyMovement = ChosenTargetPolicy.mPriority == EEnemyTargetPriority::Nearest && !bChasePreferred;
		const auto CanPrepareCast = [&Table, ChosenSkillSlot, ReferenceTarget, bLegacyMovement](const FTacticalTileInfo& Tile)
		{
			return bLegacyMovement ? Tile.mAimableFlags.Contains(true) :
				Table.IsAimable(Tile, ChosenSkillSlot, ReferenceTarget);
		};
		if (Table.GetTacticalTiles().ContainsByPredicate(CanPrepareCast))
		{
			// 이번 턴에는 못 때리므로, 다음 턴 시전을 노리고 조준 가능한 타일로 이동
			Dest = ChooseDestinationByTendency(Table, Enemy->GetMoveTendency(), ReferenceTarget, EnemyTile,
				CanPrepareCast);

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
			// 어디로 가든 조준이 안 되면, DA로 선택한 타겟에게 최대한 접근
			Dest = ChooseApproachDestination(Table, ReferenceTarget, EnemyTile);

			// 판단근거 로그: 접근이동 결정
			if (Dest != EnemyTile)
			{
				UE_LOG(LogSRPGEnemyPlanner, Log, TEXT("%s 결정: 접근이동 → 타겟[%d]=%s 방향 (%d,%d) 이동%d/AP%d"),
					*LogPrefix, ReferenceTarget, *MakeUnitLabel(TargetModels[ReferenceTarget]), Dest.mX, Dest.mY, GetTableMoveCost(Dest), ActionPoint);
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
		CastRef.mTargetIndex = AreaAim != FTileIndex::Invalid ? AreaAim :
			(bAimSelf ? Dest : TargetTiles[ChosenTarget]);
		AddAction(MoveTemp(Cast));
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

int32 USRPGEnemyTurnPlanner::ChooseTargetByPriority(
	const FEnemyTargetPolicy& Policy,
	const FTacticalTileTable& Table,
	const TArray<const UUnitModel*>& Targets,
	const TArray<int32>& Candidates,
	const FRandomStream& EventStream)
{
	const EEnemyTargetPriority Priority = Policy.mPriority;
	if (Candidates.IsEmpty()) return INDEX_NONE;
	if (Priority == EEnemyTargetPriority::Random)
		return Candidates.Num() == 1 ? Candidates[0] : Candidates[EventStream.RandRange(0, Candidates.Num() - 1)];
	if (Priority == EEnemyTargetPriority::Nearest || Priority == EEnemyTargetPriority::MostTargets ||
		(Policy.IsStatusPolicy() && !Policy.mStatusTag.IsValid()))
		return ChooseNearestTarget(Table, Candidates, EventStream);

	double BestScore = TNumericLimits<double>::Max();
	TArray<int32> BestTargets;
	for (int32 Index : Candidates)
	{
		const auto* Attributes = Targets[Index]->GetAttributeComponentModel();
		if (!Attributes && Priority != EEnemyTargetPriority::Farthest) continue;
		bool HasValues = true;
		const auto Read = [Attributes, &HasValues](const FTacticalAttribute& Attribute)
		{
			bool Found = false;
			const float Value = Attributes->GetAttributeCurrentValue(Attribute, Found);
			HasValues &= Found;
			return Value;
		};
		double Score = 0.;
		switch (Priority)
		{
		case EEnemyTargetPriority::LowestHP:
			Score = Read(UCombatTargetAttributeSet::GetHPAttribute());
			break;
		case EEnemyTargetPriority::LowestHPPercent:
		{
			const double MaxHP = Read(UCombatTargetAttributeSet::GetMaxHPAttribute());
			if (MaxHP <= 0.) continue;
			Score = Read(UCombatTargetAttributeSet::GetHPAttribute()) / MaxHP;
			break;
		}
		case EEnemyTargetPriority::HighestMaxHP:
			Score = -Read(UCombatTargetAttributeSet::GetMaxHPAttribute());
			break;
		case EEnemyTargetPriority::Farthest:
			if (Table.GetDistanceToTarget(Index) == MAX_int32) continue;
			Score = -static_cast<double>(Table.GetDistanceToTarget(Index));
			break;
		case EEnemyTargetPriority::WithStatus:
			Score = Attributes->HasMatchingGameplayTag(Policy.mStatusTag) ? 0. : 1.;
			break;
		case EEnemyTargetPriority::WithoutStatus:
			Score = Attributes->HasMatchingGameplayTag(Policy.mStatusTag) ? 1. : 0.;
			break;
		default:
			return ChooseNearestTarget(Table, Candidates, EventStream);
		}
		if (!HasValues || !FMath::IsFinite(Score)) continue;
		if (Score < BestScore)
		{
			BestScore = Score;
			BestTargets.Reset();
			BestTargets.Add(Index);
		}
		else if (Score == BestScore)
		{
			BestTargets.Add(Index);
		}
	}
	// Missing attributes must not stop the turn. Preserve the established seeded distance tie-break.
	return ChooseNearestTarget(Table, BestTargets.IsEmpty() ? Candidates : BestTargets, EventStream);
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
