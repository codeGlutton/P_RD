/*****************************************************************//**
 * @file   TileMapModelTests.cpp
 * @brief  UTileMapModel 타겟범위/영향범위/조준가능(CanAim)/위협범위(GetThreatRanges) 유닛테스트
 * @details
 * 타겟범위: TargetOnly/LineToTarget 경로 수집, 가까운 순 정렬, 차단 무관 검증.
 * 영향범위: 차단 레이어 마스크에 따른 확산 멈춤/관통 검증.
 * CanAim은 GetAimableTiles와 판정이 일치해야 하므로 패턴/사거리/점유/차단 조합 전수 교차검증.
 * GetThreatRanges는 시전 예산(제자리 시전/이동 후 시전)과 유닛 차단 검증 3케이스.
 * @author 이문환
 * @date   2026-08-01
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "SRPGFramework/EnemyTurnPlannerTestsHelper.h" // UMockPlayerUnitModel (점유 판정용 유닛 Mock)
#include "SRPGFramework/SRPGFrameworkType.h"           // FTileIndex, ETargetPattern, EEffectPattern

#include "Actor/TileMap/TileMapModel.h"
#include "DataAsset/SkillData/StaticSkillData.h"

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

	// @brief 위협 범위 테스트용 스킬 생성 (KeepAlive에 등록해 GC 방지)
	UStaticSkillData* MakeThreatSkill(UWorld* World, TArray<UObject*>& KeepAlive, EAimPattern AimPattern, int32 AimRange, int32 RequiredMovement)
	{
		UStaticSkillData* Skill = NewObject<UStaticSkillData>(World);
		Skill->mAimPattern = AimPattern;
		Skill->mAimRange = AimRange;
		Skill->mCanAimBoardActor = true;
		Skill->mAimBlockerMask = static_cast<int32>(ETileLayerFlag::Obstacle | ETileLayerFlag::Unit);
		Skill->mRequiredMovement = RequiredMovement;
		Skill->AddToRoot();
		KeepAlive.Add(Skill);
		return Skill;
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
					// 곡사 여부를 차단 레이어로 변환 (전 조합 유지)
					const ETileLayerFlag BlockerLayers = bIndirect ? ETileLayerFlag::None : (ETileLayerFlag::Obstacle | ETileLayerFlag::Unit);

					// 기준: 목록 버전이 계산한 조준 가능 타일 집합
					const TArray<FTileIndex> Aimables = TileMap->GetAimableTiles(Origin, Range, Pattern, bIncludeOccupied, BlockerLayers);

					// 맵의 모든 타일에 대해 판정 일치 확인
					int32 MismatchCount = 0;
					for (int32 Y = 0; Y < TileMap->GetHeight(); ++Y)
					{
						for (int32 X = 0; X < TileMap->GetWidth(); ++X)
						{
							const FTileIndex Target(X, Y);
							const bool bCanAim = TileMap->CanAim(Origin, Target, Range, Pattern, bIncludeOccupied, BlockerLayers);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileMapModelThreatRangesTests,
	"P_RD.TileMap.ThreatRanges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FTileMapModelThreatRangesTests::RunTest(const FString& Parameters)
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

	// 유닛테스트 동안 스킬 데이터가 GC 당하지 않도록 걸어두는 장치
	TArray<UObject*> KeepAlive;

	/**
	 * Case1: 시전비용 = 행동력 (기획안 예시 A)
	 *   -> 이동은 3칸까지 가능하지만, 이동하면 시전 예산이 남지 않아 공격은 제자리에서만
	 * 맵 (5x5): 적 E=(2,2), AP 3, 스킬 {비용 3, Cross 1}
	 *   -> 이동범위: 이동거리 3 이내 20칸 + 원점 = 21칸
	 *   -> 공격범위: 원점 Cross1 = 4칸
	 */
	AddInfo(TEXT("=== Case1: 시전비용 = 행동력 -> 제자리 시전만 ==="));
	{
		UTileMapModel* TileMap = NewObject<UTileMapModel>(World);
		TileMap->SetDimensions(5, 5);

		// 적 유닛 배치 (점유/차폐 제외 판정용 — 레이어만 필요하니 플레이어 Mock 재사용)
		UMockPlayerUnitModel* Enemy = NewObject<UMockPlayerUnitModel>(World);
		TileMap->PlaceActor(FTileTransform(FTileIndex(2, 2)), Enemy);

		// 빈 슬롯(nullptr)이 무시되는지 함께 검증
		const TArray<const UStaticSkillData*> Skills = { nullptr, MakeThreatSkill(World, KeepAlive, EAimPattern::Cross, 1, 3) };

		TArray<FTileIndex> MoveTiles;
		TArray<FTileIndex> AttackTiles;
		TileMap->GetThreatRanges(FTileIndex(2, 2), 3, Skills, Enemy, OUT MoveTiles, OUT AttackTiles);

		TestEqual(TEXT("[Case1] 이동범위 21칸 (거리3 이내 + 원점)"), MoveTiles.Num(), 21);
		TestTrue(TEXT("[Case1] 이동범위에 원점 포함"), MoveTiles.Contains(FTileIndex(2, 2)));
		TestEqual(TEXT("[Case1] 공격범위 4칸 (제자리 Cross1)"), AttackTiles.Num(), 4);
		TestTrue(TEXT("[Case1] (1,2) 포함"), AttackTiles.Contains(FTileIndex(1, 2)));
		TestTrue(TEXT("[Case1] (3,2) 포함"), AttackTiles.Contains(FTileIndex(3, 2)));
		TestTrue(TEXT("[Case1] (2,1) 포함"), AttackTiles.Contains(FTileIndex(2, 1)));
		TestTrue(TEXT("[Case1] (2,3) 포함"), AttackTiles.Contains(FTileIndex(2, 3)));
	}

	/**
	 * Case2: 시전비용 1 (기획안 예시 B)
	 *   -> 이동비용 2 이하인 타일에서 시전 가능, 공격범위가 이동만큼 넓어짐
	 * 맵 (5x5): 적 E=(2,2), AP 3, 스킬 {비용 1, Cross 2}
	 *   -> 이동범위: Case1과 동일 21칸
	 *   -> 공격범위: 이동비용 2 이내 타일들의 Cross2 합집합 = 보드 전체 25칸
	 */
	AddInfo(TEXT("=== Case2: 시전비용 1 -> 이동 후 시전으로 공격범위 확장 ==="));
	{
		UTileMapModel* TileMap = NewObject<UTileMapModel>(World);
		TileMap->SetDimensions(5, 5);

		// 적 유닛 배치 (점유/차폐 제외 판정용 — 레이어만 필요하니 플레이어 Mock 재사용)
		UMockPlayerUnitModel* Enemy = NewObject<UMockPlayerUnitModel>(World);
		TileMap->PlaceActor(FTileTransform(FTileIndex(2, 2)), Enemy);

		const TArray<const UStaticSkillData*> Skills = { MakeThreatSkill(World, KeepAlive, EAimPattern::Cross, 2, 1) };

		TArray<FTileIndex> MoveTiles;
		TArray<FTileIndex> AttackTiles;
		TileMap->GetThreatRanges(FTileIndex(2, 2), 3, Skills, Enemy, OUT MoveTiles, OUT AttackTiles);

		TestEqual(TEXT("[Case2] 이동범위 21칸 (Case1과 동일)"), MoveTiles.Num(), 21);
		TestEqual(TEXT("[Case2] 공격범위 25칸 (보드 전체)"), AttackTiles.Num(), 25);
		TestTrue(TEXT("[Case2] 코너 (0,0) 포함 (이동2 + Cross2)"), AttackTiles.Contains(FTileIndex(0, 0)));
		TestTrue(TEXT("[Case2] 코너 (4,4) 포함 (이동2 + Cross2)"), AttackTiles.Contains(FTileIndex(4, 4)));
	}

	/**
	 * Case3: 차단 유닛 (통과 불가 + 시야 차폐)
	 * 맵 (6x1): 적 E=(0,0), 차단 유닛 U=(2,0), AP 3, 스킬 {비용 1, Cross 2, 직사}
	 *   -> 이동범위: {(0,0), (1,0)} — 유닛을 통과할 수 없어 뒤쪽 도달 불가
	 *   -> 공격범위: {(0,0), (1,0), (2,0)} — 점유 칸은 조준 가능, 그 너머 (3,0)은 차폐로 미포함
	 */
	AddInfo(TEXT("=== Case3: 차단 유닛 -> 통과 불가 + 차폐 ==="));
	{
		UTileMapModel* TileMap = NewObject<UTileMapModel>(World);
		TileMap->SetDimensions(6, 1);

		// 적 유닛과 차단 유닛 배치 (레이어만 필요하니 플레이어 Mock 재사용)
		UMockPlayerUnitModel* Enemy = NewObject<UMockPlayerUnitModel>(World);
		TileMap->PlaceActor(FTileTransform(FTileIndex(0, 0)), Enemy);
		UMockPlayerUnitModel* Blocker = NewObject<UMockPlayerUnitModel>(World);
		TileMap->PlaceActor(FTileTransform(FTileIndex(2, 0)), Blocker);

		const TArray<const UStaticSkillData*> Skills = { MakeThreatSkill(World, KeepAlive, EAimPattern::Cross, 2, 1) };

		TArray<FTileIndex> MoveTiles;
		TArray<FTileIndex> AttackTiles;
		TileMap->GetThreatRanges(FTileIndex(0, 0), 3, Skills, Enemy, OUT MoveTiles, OUT AttackTiles);

		TestEqual(TEXT("[Case3] 이동범위 2칸 (차단 유닛에 막힘)"), MoveTiles.Num(), 2);
		TestTrue(TEXT("[Case3] 이동범위에 (1,0) 포함"), MoveTiles.Contains(FTileIndex(1, 0)));
		TestFalse(TEXT("[Case3] 이동범위에 (2,0) 미포함 (점유 칸)"), MoveTiles.Contains(FTileIndex(2, 0)));
		TestEqual(TEXT("[Case3] 공격범위 3칸"), AttackTiles.Num(), 3);
		TestTrue(TEXT("[Case3] (2,0) 포함 (점유 칸 조준 가능)"), AttackTiles.Contains(FTileIndex(2, 0)));
		TestFalse(TEXT("[Case3] (3,0) 미포함 (차단 유닛 너머 차폐)"), AttackTiles.Contains(FTileIndex(3, 0)));
	}

	// GC 안당하려고 KeepAlive에 매달아놨던 스킬 데이터 연결 해제 -> GC 대상
	for (UObject* Object : KeepAlive)
		if (IsValid(Object))
			Object->RemoveFromRoot();

	return true;
}
