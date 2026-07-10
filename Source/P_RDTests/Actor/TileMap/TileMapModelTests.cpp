/*****************************************************************//**
 * @file   TileMapModelTests.cpp
 * @brief  UTileMapModel 효과범위(GetEffectTiles) 유닛테스트
 * @details
 * 빔(Beam) 패턴의 점유 칸 처리 검증 3케이스.
 * 비관통 빔이 점유 칸을 직접 조준하면 그 칸에서 멈추는 회귀 케이스 포함.
 * @author 이문환
 * @date   2026-07-10
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "SRPGFramework/EnemyTurnPlannerTestsHelper.h" // UMockPlayerUnitModel (점유 판정용 유닛 Mock)
#include "SRPGFramework/SRPGFrameworkType.h"           // FTileIndex, EEffectPattern

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

	// 맵 (8x1): 시전자 C=(0,0), 점유 유닛 U=(3,0)
	UTileMapModel* TileMap = NewObject<UTileMapModel>(World);
	TileMap->SetDimensions(8, 1);

	// 점유 유닛 배치 (Unit 레이어 판정만 필요하니 플레이어 Mock 재사용)
	UMockPlayerUnitModel* Blocker = NewObject<UMockPlayerUnitModel>(World);
	TileMap->PlaceActor(FTileTransform(FTileIndex(3, 0)), Blocker);

	const FTileIndex Caster(0, 0);

	/**
	 * Case1: 비관통 빔 / 빈 타일 (1,0) 조준, 길이 4
	 *   -> 빔이 뻗다가 점유 칸(3,0)을 맞고 멈춤 (점유 칸까지는 포함)
	 *   -> 기대: {(1,0), (2,0), (3,0)}
	 */
	AddInfo(TEXT("=== Case1: 비관통 빔 / 빈 타일 조준 -> 점유 칸에서 정지 ==="));
	{
		const TArray<FTileIndex> Tiles = TileMap->GetEffectTiles(Caster, FTileIndex(1, 0), EEffectPattern::Beam, 4, /*bPenetrate*/false);
		TestEqual(TEXT("[Case1] 효과 타일 수 3"), Tiles.Num(), 3);
		TestTrue(TEXT("[Case1] (1,0) 포함"), Tiles.Contains(FTileIndex(1, 0)));
		TestTrue(TEXT("[Case1] (2,0) 포함"), Tiles.Contains(FTileIndex(2, 0)));
		TestTrue(TEXT("[Case1] (3,0) 포함(맞고 멈춤)"), Tiles.Contains(FTileIndex(3, 0)));
		TestFalse(TEXT("[Case1] (4,0) 미포함(점유 칸 너머 진행 금지)"), Tiles.Contains(FTileIndex(4, 0)));
	}

	/**
	 * Case2: 비관통 빔 / 점유 칸 (3,0) 직접 조준, 길이 4 (회귀 케이스)
	 *   -> 조준 칸 자체가 점유라 거기서 맞고 멈춤 — 너머로 뻗으면 관통 버그
	 *   -> 기대: {(3,0)}
	 */
	AddInfo(TEXT("=== Case2: 비관통 빔 / 점유 칸 직접 조준 -> 관통 금지 (회귀) ==="));
	{
		const TArray<FTileIndex> Tiles = TileMap->GetEffectTiles(Caster, FTileIndex(3, 0), EEffectPattern::Beam, 4, /*bPenetrate*/false);
		TestEqual(TEXT("[Case2] 효과 타일 수 1"), Tiles.Num(), 1);
		TestTrue(TEXT("[Case2] (3,0) 포함"), Tiles.Contains(FTileIndex(3, 0)));
		TestFalse(TEXT("[Case2] (4,0) 미포함(점유 칸 너머 진행 금지)"), Tiles.Contains(FTileIndex(4, 0)));
	}

	/**
	 * Case3: 관통 빔 / 점유 칸 (3,0) 직접 조준, 길이 3
	 *   -> 관통이면 점유 칸 너머로도 계속 진행
	 *   -> 기대: {(3,0), (4,0), (5,0)}
	 */
	AddInfo(TEXT("=== Case3: 관통 빔 / 점유 칸 직접 조준 -> 통과 유지 ==="));
	{
		const TArray<FTileIndex> Tiles = TileMap->GetEffectTiles(Caster, FTileIndex(3, 0), EEffectPattern::Beam, 3, /*bPenetrate*/true);
		TestEqual(TEXT("[Case3] 효과 타일 수 3"), Tiles.Num(), 3);
		TestTrue(TEXT("[Case3] (3,0) 포함"), Tiles.Contains(FTileIndex(3, 0)));
		TestTrue(TEXT("[Case3] (4,0) 포함(관통 진행)"), Tiles.Contains(FTileIndex(4, 0)));
		TestTrue(TEXT("[Case3] (5,0) 포함(관통 진행)"), Tiles.Contains(FTileIndex(5, 0)));
	}

	return true;
}
