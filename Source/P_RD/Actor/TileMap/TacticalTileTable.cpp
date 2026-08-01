/*****************************************************************//**
 * @file   TacticalTileTable.cpp
 * @brief  턴 계획용 전술 타일 테이블 구현
 * @author 이문환
 * @date   2026-07-25
 *********************************************************************/

#include "Actor/TileMap/TacticalTileTable.h"

#include "Actor/TileMap/TileMapModel.h"
#include "DataAsset/SkillData/StaticSkillData.h"

void FTacticalTileTable::Build(
	const UTileMapModel* TileMap,
	const UBoardActorModel* Self,
	const FTileIndex& Origin,
	const TArray<FTileIndex>& TargetTiles,
	const TArray<const UStaticSkillData*>& Skills,
	int32 ActionPoint)
{
	// 재사용 대비 초기화
	mTacticalTiles.Reset();
	mOriginTargetDistances.Reset();
	mSkillSlotCount = Skills.Num();
	mTargetCount = TargetTiles.Num();

	// 타일맵 또는 타겟이 없으면 계산이 무의미하므로 바로 리턴
	if (TileMap == nullptr || mTargetCount == 0)
	{
		return;
	}

	// 이동가능한 타일 수집 (어쨌거나 이동가능한 타일에서 공격을 시작하니까)
	TArray<FTileIndex> ReachableTiles = TileMap->GetReachableTiles(Origin, ActionPoint, Self);
	ReachableTiles.Add(Origin);

	// 원점에서 타일까지 이동거리장 (타일까지 이동하는데 소모하는 행동력)
	const TArray<int32> MoveCostField = TileMap->GetDistanceField(Origin, Self);

	// 타일에서 타겟까지 이동거리장 (이동성향에 따라 분기할 때 참조)
	TArray<TArray<int32>> TargetDistanceFields;
	TargetDistanceFields.Reserve(mTargetCount);
	for (const FTileIndex& TargetTile : TargetTiles)
	{
		TargetDistanceFields.Add(TileMap->GetDistanceField(TargetTile, Self));
	}

	// 원점에서 타겟까지의 경로 거리 (타겟 선택할 때 '최근접' 비교값)
	for (const FTileIndex& TargetTile : TargetTiles)
	{
		// 타겟이 서 있는 칸까지 들어선다고 칠 때의 걸음 수 = 경로 길이 - 1 (경로가 없으면 도달 불가)
		const TArray<FTileIndex> Path = TileMap->FindPath(Origin, TargetTile, Self, /*bAllowOccupiedGoal*/true);
		mOriginTargetDistances.Add(Path.IsEmpty() ? MAX_int32 : Path.Num() - 1);
	}

	// 타일마다 전술 정보 채움
	mTacticalTiles.Reserve(ReachableTiles.Num());
	for (const FTileIndex& Tile : ReachableTiles)
	{
		const int32 Linear = TileMap->TileIndexToLinearIndex(Tile);

		// 이동비용 (원점 자신은 거리장에서 0)
		const int32 MoveCost = MoveCostField[Linear];
		checkf(MoveCost >= 0, TEXT("타일(%d,%d)이 이동가능 목록에는 있는데 거리장에서는 도달불가(%d) — GetReachableTiles()와 GetDistanceField()의 통과 판정이 어긋남"), Tile.mX, Tile.mY, MoveCost);

		FTacticalTileInfo& Info = mTacticalTiles.AddDefaulted_GetRef();
		Info.mIndex = Tile;
		Info.mMoveCost = MoveCost;

		// 타일에서 타겟까지의 경로 거리 (도달 불가는 MAX_int32로 통일)
		Info.mTargetDistances.Reserve(mTargetCount);
		for (int32 TargetIndex = 0; TargetIndex < mTargetCount; ++TargetIndex)
		{
			const int32 RawDistance = TargetDistanceFields[TargetIndex][Linear];
			Info.mTargetDistances.Add(RawDistance >= 0 ? RawDistance : MAX_int32);
		}

		// 조준/시전 플래그 채우기
		Info.mAimableFlags.Init(false, mSkillSlotCount * mTargetCount);
		Info.mCastableFlags.Init(false, mSkillSlotCount * mTargetCount);
		for (int32 SkillSlot = 0; SkillSlot < mSkillSlotCount; ++SkillSlot)
		{
			const UStaticSkillData* Skill = Skills[SkillSlot];
			// 빈 슬롯은 전부 false 유지
			if (Skill == nullptr)
			{
				continue;
			}

			// 예산 판정: 이 타일까지 이동한 뒤 남는 행동력으로 시전비용을 감당할 수 있는가
			const bool bBudgetOk = (Info.mMoveCost + Skill->mRequiredMovement <= ActionPoint);

			for (int32 TargetIndex = 0; TargetIndex < mTargetCount; ++TargetIndex)
			{
				// 조준 판정: 자기 자신은 이동으로 자리를 비울 예정이므로 시야 차폐에서 제외
				const bool bAimable = TileMap->CanAim(
					Tile,
					TargetTiles[TargetIndex],
					Skill->mAimRange,
					Skill->mAimPattern,
					Skill->mCanAimBoardActor,
					static_cast<ETileLayerFlag>(Skill->mAimBlockerMask),
					/*Incoming*/nullptr,
					/*IgnoreBlocker*/Self);

				const int32 Flag = FlagIndex(SkillSlot, TargetIndex);
				Info.mAimableFlags[Flag] = bAimable;
				Info.mCastableFlags[Flag] = bAimable && bBudgetOk;
			}
		}
	}
}

bool FTacticalTileTable::IsAimable(const FTacticalTileInfo& Tile, int32 SkillSlot, int32 TargetIndex) const
{
	return Tile.mAimableFlags[FlagIndex(SkillSlot, TargetIndex)];
}

bool FTacticalTileTable::IsCastable(const FTacticalTileInfo& Tile, int32 SkillSlot, int32 TargetIndex) const
{
	return Tile.mCastableFlags[FlagIndex(SkillSlot, TargetIndex)];
}

bool FTacticalTileTable::CanCastToTarget(int32 TargetIndex) const
{
	// 어느 타일에서든, 어떤 스킬로든 시전 가능하면 참
	for (const FTacticalTileInfo& Tile : mTacticalTiles)
	{
		for (int32 SkillSlot = 0; SkillSlot < mSkillSlotCount; ++SkillSlot)
		{
			if (IsCastable(Tile, SkillSlot, TargetIndex))
			{
				return true;
			}
		}
	}
	return false;
}

TArray<int32> FTacticalTileTable::GetCastableSkillSlots(int32 TargetIndex) const
{
	TArray<int32> Result;
	for (int32 SkillSlot = 0; SkillSlot < mSkillSlotCount; ++SkillSlot)
	{
		// 어느 타일에서든 이 슬롯으로 해당 타겟에게 시전 가능하면 포함
		for (const FTacticalTileInfo& Tile : mTacticalTiles)
		{
			if (IsCastable(Tile, SkillSlot, TargetIndex))
			{
				Result.Add(SkillSlot);
				break;
			}
		}
	}
	return Result;
}

bool FTacticalTileTable::HasAnyAimable() const
{
	// 조준 가능한 {타일, 스킬, 타겟} 조합이 하나라도 있으면 참
	for (const FTacticalTileInfo& Tile : mTacticalTiles)
	{
		if (Tile.mAimableFlags.Contains(true))
		{
			return true;
		}
	}
	return false;
}

int32 FTacticalTileTable::GetDistanceToTarget(int32 TargetIndex) const
{
	return mOriginTargetDistances[TargetIndex];
}

int32 FTacticalTileTable::GetNearestTargetDistance(const FTacticalTileInfo& Tile) const
{
	// 가장 가까운 타겟까지의 거리 = 타겟별 거리의 최솟값
	int32 Result = MAX_int32;
	for (const int32 Distance : Tile.mTargetDistances)
	{
		Result = FMath::Min(Result, Distance);
	}
	return Result;
}

FTileIndex FTacticalTileTable::PickTile(
	TFunctionRef<bool(const FTacticalTileInfo&)> Filter,
	TFunctionRef<int64(const FTacticalTileInfo&)> PrimaryAxis,
	TFunctionRef<int64(const FTacticalTileInfo&)> SecondaryAxis,
	const FTileIndex& Fallback) const
{
	FTileIndex Best = Fallback;
	bool HasBest = false;
	int64 BestPrimary = 0;
	int64 BestSecondary = 0;

	for (const FTacticalTileInfo& Tile : mTacticalTiles)
	{
		// 필터 탈락 타일은 후보가 아님
		if (Filter(Tile) == false)
		{
			continue;
		}

		const int64 Primary = PrimaryAxis(Tile);
		const int64 Secondary = SecondaryAxis(Tile);

		bool Better = false;
		// 1) 첫 후보면 무조건 최선
		if (HasBest == false)
		{
			Better = true;
		}
		// 2) 1순위 축이 다르면 작은 쪽이 최선
		else if (Primary != BestPrimary)
		{
			Better = (Primary < BestPrimary);
		}
		// 3) 1순위가 같으면 2순위 축이 작은 쪽이 최선
		// 완전 동률이면 먼저 나온 타일 유지
		else
		{
			Better = (Secondary < BestSecondary);
		}

		if (Better == true)
		{
			Best = Tile.mIndex;
			BestPrimary = Primary;
			BestSecondary = Secondary;
			HasBest = true;
		}
	}
	return Best;
}

int32 FTacticalTileTable::FlagIndex(int32 SkillSlot, int32 TargetIndex) const
{
	return SkillSlot * mTargetCount + TargetIndex;
}
