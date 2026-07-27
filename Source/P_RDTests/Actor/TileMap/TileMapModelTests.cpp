/*****************************************************************//**
 * @file   TileMapModelTests.cpp
 * @brief  UTileMapModel 효과범위(GetEffectTiles)/조준가능 질의(CanAim) 유닛테스트
 * @details
 * 빔(Beam) 패턴의 점유 칸 처리 검증 3케이스.
 * 비관통 빔이 점유 칸을 직접 조준하면 그 칸에서 멈추는 회귀 케이스 포함.
 * CanAim은 GetAimableTiles와 판정이 일치해야 하므로 패턴/사거리/점유/곡사 조합 전수 교차검증.
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileMapModelCanAimTests,
	"P_RD.TileMap.CanAim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FTileMapModelCanAimTests::RunTest(const FString& Parameters)
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

	/**
	 * CanAim은 GetAimableTiles의 목록 생성과 별개 구현(패턴 기하 산술)이므로
	 * 두 구현이 어긋나면 AI 계획과 실제 조준 판정이 달라진다.
	 * 패턴/사거리/점유/곡사 전 조합에 대해, 맵의 모든 타일에서
	 * CanAim(원점, 타일) == GetAimableTiles(원점).Contains(타일)을 전수 확인한다.
	 */
	// 맵 (7x5): 원점 O=(1,2), 시야/점유 변화를 만들 유닛 U1=(3,2), U2=(2,1)
	UTileMapModel* TileMap = NewObject<UTileMapModel>(World);
	TileMap->SetDimensions(7, 5);

	UMockPlayerUnitModel* Unit1 = NewObject<UMockPlayerUnitModel>(World);
	TileMap->PlaceActor(FTileTransform(FTileIndex(3, 2)), Unit1);
	UMockPlayerUnitModel* Unit2 = NewObject<UMockPlayerUnitModel>(World);
	TileMap->PlaceActor(FTileTransform(FTileIndex(2, 1)), Unit2);

	const FTileIndex Origin(1, 2);

	// 조합 축: 패턴 4종 × 사거리 {0,1,3} × 점유허용 {true,false} × 곡사 {false,true}
	const EAimPattern Patterns[] = { EAimPattern::Single, EAimPattern::Cross, EAimPattern::Star, EAimPattern::Square };
	const int32 Ranges[] = { 0, 1, 3 };
	const bool Bools[] = { false, true };

	for (const EAimPattern Pattern : Patterns)
	{
		for (const int32 Range : Ranges)
		{
			for (const bool bIncludeOccupied : Bools)
			{
				for (const bool bIndirect : Bools)
				{
					// 기준: 목록 버전이 계산한 조준 가능 타일 집합
					const TArray<FTileIndex> Aimables = TileMap->GetAimableTiles(Origin, Range, Pattern, bIncludeOccupied, bIndirect);

					// 맵의 모든 타일에 대해 판정 일치 확인
					int32 MismatchCount = 0;
					for (int32 Y = 0; Y < TileMap->GetHeight(); ++Y)
					{
						for (int32 X = 0; X < TileMap->GetWidth(); ++X)
						{
							const FTileIndex Target(X, Y);
							const bool bCanAim = TileMap->CanAim(Origin, Target, Range, Pattern, bIncludeOccupied, bIndirect);
							if (bCanAim != Aimables.Contains(Target))
							{
								++MismatchCount;
								AddError(FString::Printf(TEXT("판정 불일치: 타일(%d,%d) 패턴=%d 사거리=%d 점유허용=%d 곡사=%d CanAim=%d"),
									X, Y, static_cast<int32>(Pattern), Range, bIncludeOccupied, bIndirect, bCanAim));
							}
						}
					}
					TestEqual(FString::Printf(TEXT("패턴=%d 사거리=%d 점유허용=%d 곡사=%d 전 타일 판정 일치"),
						static_cast<int32>(Pattern), Range, bIncludeOccupied, bIndirect), MismatchCount, 0);
				}
			}
		}
	}

	return true;
}
