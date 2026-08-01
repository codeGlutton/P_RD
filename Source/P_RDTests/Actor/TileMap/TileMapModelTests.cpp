/*****************************************************************//**
 * @file   TileMapModelTests.cpp
 * @brief  UTileMapModel 타겟범위(GetTargetTiles)와 영향범위(GetEffectTiles) 유닛테스트
 * @details
 * 타겟범위: TargetOnly/LineToTarget 경로 수집, 가까운 순 정렬, 차단 무관 검증.
 * 영향범위: 차단 레이어 마스크에 따른 확산 멈춤/관통 검증.
 * @author 이문환
 * @date   2026-08-01
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "SRPGFramework/EnemyTurnPlannerTestsHelper.h" // UMockPlayerUnitModel (점유 판정용 유닛 Mock)
#include "SRPGFramework/SRPGFrameworkType.h"           // FTileIndex, ETargetPattern, EEffectPattern

#include "Actor/TileMap/TileMapModel.h"

#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	// @brief 테스트용 월드 획득 (PIE/Game 우선)
	UWorld* GetAnyGameWorldForTileMapTests()
	{
		if (GEngine == nullptr)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if ((Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) && Context.World() != nullptr)
			{
				return Context.World();
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileMapModelTargetTilesTests,
	"P_RD.TileMap.TargetTiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FTileMapModelTargetTilesTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForTileMapTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	// 맵 (8x8): 시전자 C=(0,0)
	UTileMapModel* TileMap = NewObject<UTileMapModel>(World);
	TileMap->SetDimensions(8, 8);

	const FTileIndex Caster(0, 0);

	/**
	 * Case1: TargetOnly / (3,0) 조준
	 *   -> 조준 타일 한 칸만 타겟
	 *   -> 기대: {(3,0)}
	 */
	AddInfo(TEXT("=== Case1: TargetOnly -> 조준 타일 한 칸 ==="));
	{
		const TArray<FTileIndex> Tiles = TileMap->GetTargetTiles(Caster, FTileIndex(3, 0), ETargetPattern::TargetOnly);
		TestEqual(TEXT("[Case1] 타겟 타일 수 1"), Tiles.Num(), 1);
		TestTrue(TEXT("[Case1] (3,0) 포함"), Tiles.Contains(FTileIndex(3, 0)));
	}

	/**
	 * Case2: LineToTarget / 직선 (3,0) 조준
	 *   -> 시전자 제외, 시전자에서 가까운 순으로 경로 수집
	 *   -> 기대: {(1,0), (2,0), (3,0)} 순서 보장
	 */
	AddInfo(TEXT("=== Case2: LineToTarget 직선 -> 가까운 순 경로 ==="));
	{
		const TArray<FTileIndex> Tiles = TileMap->GetTargetTiles(Caster, FTileIndex(3, 0), ETargetPattern::LineToTarget);
		TestEqual(TEXT("[Case2] 타겟 타일 수 3"), Tiles.Num(), 3);
		if (Tiles.Num() == 3)
		{
			TestTrue(TEXT("[Case2] 첫째 (1,0)"), Tiles[0] == FTileIndex(1, 0));
			TestTrue(TEXT("[Case2] 둘째 (2,0)"), Tiles[1] == FTileIndex(2, 0));
			TestTrue(TEXT("[Case2] 셋째 (3,0)"), Tiles[2] == FTileIndex(3, 0));
		}
	}

	/**
	 * Case3: LineToTarget / 경로 위 점유 유닛 (2,0)
	 *   -> 타겟 수집은 차단과 무관하게 경로 전체 유지 (차단은 조준/영향 단계 소관)
	 *   -> 기대: {(1,0), (2,0), (3,0)}
	 */
	AddInfo(TEXT("=== Case3: LineToTarget / 경로 위 점유 -> 경로 불변 ==="));
	{
		UMockPlayerUnitModel* Blocker = NewObject<UMockPlayerUnitModel>(World);
		TileMap->PlaceActor(FTileTransform(FTileIndex(2, 0)), Blocker);

		const TArray<FTileIndex> Tiles = TileMap->GetTargetTiles(Caster, FTileIndex(3, 0), ETargetPattern::LineToTarget);
		TestEqual(TEXT("[Case3] 타겟 타일 수 3"), Tiles.Num(), 3);
		TestTrue(TEXT("[Case3] 점유 칸 (2,0) 포함"), Tiles.Contains(FTileIndex(2, 0)));
	}

	/**
	 * Case4: LineToTarget / 시전자 타일 조준
	 *   -> 조준 타일은 어떤 패턴에서든 항상 포함
	 *   -> 기대: {(0,0)}
	 */
	AddInfo(TEXT("=== Case4: LineToTarget / 자기 타일 조준 -> 조준 타일만 ==="));
	{
		const TArray<FTileIndex> Tiles = TileMap->GetTargetTiles(Caster, Caster, ETargetPattern::LineToTarget);
		TestEqual(TEXT("[Case4] 타겟 타일 수 1"), Tiles.Num(), 1);
		TestTrue(TEXT("[Case4] (0,0) 포함"), Tiles.Contains(FTileIndex(0, 0)));
	}

	/**
	 * Case5: LineToTarget / 대각 (3,3) 조준
	 *   -> 대각 경로도 가까운 순으로 수집
	 *   -> 기대: {(1,1), (2,2), (3,3)} 순서 보장
	 */
	AddInfo(TEXT("=== Case5: LineToTarget 대각 -> 가까운 순 경로 ==="));
	{
		const TArray<FTileIndex> Tiles = TileMap->GetTargetTiles(Caster, FTileIndex(3, 3), ETargetPattern::LineToTarget);
		TestEqual(TEXT("[Case5] 타겟 타일 수 3"), Tiles.Num(), 3);
		if (Tiles.Num() == 3)
		{
			TestTrue(TEXT("[Case5] 첫째 (1,1)"), Tiles[0] == FTileIndex(1, 1));
			TestTrue(TEXT("[Case5] 둘째 (2,2)"), Tiles[1] == FTileIndex(2, 2));
			TestTrue(TEXT("[Case5] 셋째 (3,3)"), Tiles[2] == FTileIndex(3, 3));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileMapModelEffectTilesTests,
	"P_RD.TileMap.EffectTiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FTileMapModelEffectTilesTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForTileMapTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	// 맵 (8x1): 점유 유닛 U=(3,0)
	UTileMapModel* TileMap = NewObject<UTileMapModel>(World);
	TileMap->SetDimensions(8, 1);

	// 점유 유닛 배치 (Unit 레이어 판정만 필요하니 플레이어 Mock 재사용)
	UMockPlayerUnitModel* Blocker = NewObject<UMockPlayerUnitModel>(World);
	TileMap->PlaceActor(FTileTransform(FTileIndex(3, 0)), Blocker);

	/**
	 * Case1: Cross / 중심 (1,0), 크기 4, 차단 마스크 Unit+Obstacle
	 *   -> 오른쪽 확산이 점유 칸(3,0)을 맞고 멈춤 (점유 칸까지는 포함)
	 *   -> 기대: {(1,0), (0,0), (2,0), (3,0)}
	 */
	AddInfo(TEXT("=== Case1: 차단 마스크 확산 -> 점유 칸에서 정지 ==="));
	{
		const TArray<FTileIndex> Tiles = TileMap->GetEffectTiles(FTileIndex(1, 0), EEffectPattern::Cross, 4, ETileLayerFlag::Obstacle | ETileLayerFlag::Unit);
		TestEqual(TEXT("[Case1] 효과 타일 수 4"), Tiles.Num(), 4);
		TestTrue(TEXT("[Case1] (3,0) 포함(맞고 멈춤)"), Tiles.Contains(FTileIndex(3, 0)));
		TestFalse(TEXT("[Case1] (4,0) 미포함(점유 칸 너머 진행 금지)"), Tiles.Contains(FTileIndex(4, 0)));
	}

	/**
	 * Case2: Cross / 중심 (1,0), 크기 4, 차단 마스크 None
	 *   -> 아무것도 확산을 막지 않으므로 점유 칸 너머로 계속 진행
	 *   -> 기대: {(1,0), (0,0), (2,0), (3,0), (4,0), (5,0)}
	 */
	AddInfo(TEXT("=== Case2: None 마스크 -> 관통 진행 ==="));
	{
		const TArray<FTileIndex> Tiles = TileMap->GetEffectTiles(FTileIndex(1, 0), EEffectPattern::Cross, 4, ETileLayerFlag::None);
		TestEqual(TEXT("[Case2] 효과 타일 수 6"), Tiles.Num(), 6);
		TestTrue(TEXT("[Case2] (4,0) 포함(관통 진행)"), Tiles.Contains(FTileIndex(4, 0)));
		TestTrue(TEXT("[Case2] (5,0) 포함(관통 진행)"), Tiles.Contains(FTileIndex(5, 0)));
	}

	/**
	 * Case3: Cross / 점유 칸 (3,0)을 중심으로, 크기 2, 차단 마스크 Unit+Obstacle
	 *   -> 중심 점유 여부는 확산에 영향 없음 (중심은 항상 포함, 확산은 계속)
	 *   -> 기대: {(3,0), (2,0), (1,0), (4,0), (5,0)}
	 */
	AddInfo(TEXT("=== Case3: 점유 칸 중심 -> 확산 계속 ==="));
	{
		const TArray<FTileIndex> Tiles = TileMap->GetEffectTiles(FTileIndex(3, 0), EEffectPattern::Cross, 2, ETileLayerFlag::Obstacle | ETileLayerFlag::Unit);
		TestEqual(TEXT("[Case3] 효과 타일 수 5"), Tiles.Num(), 5);
		TestTrue(TEXT("[Case3] (2,0) 포함"), Tiles.Contains(FTileIndex(2, 0)));
		TestTrue(TEXT("[Case3] (4,0) 포함"), Tiles.Contains(FTileIndex(4, 0)));
	}

	return true;
}
