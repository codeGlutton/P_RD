/*****************************************************************//**
 * @file   EnemyTurnPlannerTests.cpp
 * @brief  USRPGEnemyTurnPlanner 유닛테스트
 * @details
 * 이동성향 3종(MoveClose/HoldRange/MoveAway) × 조준 가능/불가 2가지 엮어서 6가지 케이스 테스트.
 * @note
 * 장애물은 아직 구현체가 없어서 제외. 나중에 구현체가 나오면 유닛테스트에 추가 필요
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "SRPGFramework/EnemyTurnPlannerTestsHelper.h"
#include "SRPGFramework/SRPGEnemyTurnPlanner.h"
#include "SRPGFramework/SRPGMoveAction.h"      // FSRPGMoveCommand
#include "SRPGFramework/SRPGSkillAction.h"     // FSRPGSkillCastCommand
#include "SRPGFramework/SRPGTurnEndAction.h"   // FSRPGTurnEndCommand
#include "SRPGFramework/SRPGFrameworkType.h"   // FTileIndex, EAimPattern, EEffectPattern

#include "Actor/TileMap/TileMapModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "DataAsset/SkillData/StaticAttackSkillData.h"

#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	// 월드
	UWorld* GetAnyGameWorld()
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

	// @brief 커맨드 목록에서 첫 이동 커맨드를 타입으로 찾아 반환 (없으면 nullptr)
	const FSRPGMoveCommand* FindMoveCommand(const TArray<TInstancedStruct<FSRPGCommand>>& Commands)
	{
		for (const TInstancedStruct<FSRPGCommand>& Command : Commands)
		{
			if (Command.Get().GetCommandType() == ESRPGCommandType::MoveCast)
			{
				return &Command.Get<FSRPGMoveCommand>();
			}
		}
		return nullptr;
	}

	// @brief 커맨드 목록에서 첫 스킬시전 커맨드를 타입으로 찾아 반환 (없으면 nullptr)
	const FSRPGSkillCastCommand* FindCast(const TArray<TInstancedStruct<FSRPGCommand>>& Commands)
	{
		for (const TInstancedStruct<FSRPGCommand>& Command : Commands)
		{
			if (Command.Get().GetCommandType() == ESRPGCommandType::SkillCast)
			{
				return &Command.Get<FSRPGSkillCastCommand>();
			}
		}
		return nullptr;
	}

	// @brief 적 유닛의 한 턴을 계획해 커맨드 목록 반환
	// @param KeepAlive SkillComponent가 약참조라 GC 안당하도록 붙잡아두는 장치
	TArray<TInstancedStruct<FSRPGCommand>> Plan(
		UWorld* World,
		TArray<UObject*>& KeepAlive,
		EMoveTendency Tendency,
		int32 MoveRange,
		int32 AimRange,
		int32 TileMapWidth,
		int32 TileMapHeight,
		FTileIndex EnemyIndex,
		FTileIndex PlayerIndex)
	{
		// 타일맵 생성
		UTileMapModel* TileMap = NewObject<UTileMapModel>(World);
		TileMap->SetDimensions(TileMapWidth, TileMapHeight);

		// 플레이어유닛 생성
		UMockPlayerUnitModel* Player = NewObject<UMockPlayerUnitModel>(World);

		// 적유닛 생성
		UMockEnemyUnitModel* Enemy = NewObject<UMockEnemyUnitModel>(World);
		Enemy->Initialize();
		Enemy->BeginPlay();
		Enemy->SetMoveTendency(Tendency);
		Enemy->GetAttributeComponentModel()->SetAttributeBaseValue(
			UUnitAttributeSet::GetMovementPointAttribute(), static_cast<float>(MoveRange));

		// 스킬 추가: 일반공격 계열
		UStaticAttackSkillData* Skill = NewObject<UStaticAttackSkillData>(World);
		Skill->mAimPattern = EAimPattern::Square;
		Skill->mAimDefaultRange = AimRange;
		Skill->mCanAimObstacle = true;
		Skill->mIsIndirect = false;
		Skill->mEffectPattern = EEffectPattern::Single;
		Skill->mEffectDefaultArea = 0;
		Skill->mIsPenetration = false;
		Skill->AddToRoot();
		KeepAlive.Add(Skill);
		Enemy->GetSkillComponentModel()->AddSkillData(TSoftObjectPtr<UStaticSkillData>(Skill));

		// 플레이어유닛과 적유닛 배치
		TileMap->PlaceActor(FTileTransform(PlayerIndex), Player);
		TileMap->PlaceActor(FTileTransform(EnemyIndex), Enemy);

		// AI 돌려서(ㅋㅋ) 적유닛의 예상커맨드 획득
		return USRPGEnemyTurnPlanner::PlanTurn(Enemy, Player, TileMap);
	}

	// @brief 커맨드가 비어있지 않고, 마지막은 항상 턴 종료인 지 검증
	void CheckTail(FAutomationTestBase& Test, const TArray<TInstancedStruct<FSRPGCommand>>& Commands, const TCHAR* Label)
	{
		if (Test.TestTrue(FString::Printf(TEXT("[%s] 커맨드 존재"), Label),
			Commands.Num() > 0))
		{
			Test.TestTrue(FString::Printf(TEXT("[%s] 마지막은 턴 종료"), Label),
				Commands.Last().Get().GetCommandType() == ESRPGCommandType::TurnEnd);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyTurnPlannerTests,
	"P_RD.SRPG.EnemyTurnPlanner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FEnemyTurnPlannerTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorld();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	// 유닛테스트 동안 SkillComponent가 GC 당하지 않도록 걸어두는 장치
	TArray<UObject*> KeepAlive;

	// ============================================================
	// 시나리오 A: 현재 위치(또는 사거리 내)에서 조준 가능
	// 맵 α (6x3): E(2,1) P(3,1) 인접
	// ============================================================

	// --- Case1: MoveClose + 일반공격(r1) → 이미 인접이라 제자리 시전 ---
	AddInfo(TEXT("=== Case1: MoveClose / 인접 조준 → 제자리+시전 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands =
			Plan(World, KeepAlive, EMoveTendency::MoveClose, 3, 1, 6, 3, FTileIndex(2, 1), FTileIndex(3, 1));
		CheckTail(*this, Commands, TEXT("Case1"));
		TestTrue(TEXT("[Case1] 이동커맨드 없음(제자리)"), FindMoveCommand(Commands) == nullptr);
		const FSRPGSkillCastCommand* Cast = FindCast(Commands);
		if (TestTrue(TEXT("[Case1] 시전커맨드 존재"), Cast != nullptr))
		{
			TestEqual(TEXT("[Case1] 스킬 인덱스 0"), Cast->mSkillIndex, 0);
			TestTrue(TEXT("[Case1] 효과 타일에 플레이어 포함"), Cast->mEffectTileIndexes.Contains(FTileIndex(3, 1)));
		}
	}

	// --- Case3: HoldRange + 일반공격(r1) → 제자리에서 조준되므로 정지+시전 ---
	AddInfo(TEXT("=== Case3: HoldRange / 제자리 조준 → 제자리+시전 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands =
			Plan(World, KeepAlive, EMoveTendency::HoldRange, 3, 1, 6, 3, FTileIndex(2, 1), FTileIndex(3, 1));
		CheckTail(*this, Commands, TEXT("Case3"));
		TestTrue(TEXT("[Case3] 이동커맨드 없음(제자리)"), FindMoveCommand(Commands) == nullptr);
		TestTrue(TEXT("[Case3] 시전커맨드 존재"), FindCast(Commands) != nullptr);
	}

	// --- Case2: MoveAway + 원거리(r3) → 근접했으니 최대사거리로 후퇴 후 시전 ---
	// 맵 β (8x2): E(2,1) P(1,0). cheby3 내 최원거리 유일 타일 (4,1)로 후퇴
	AddInfo(TEXT("=== Case2: MoveAway / 조준 가능하나 근접 → 최원거리 후퇴+시전 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands =
			Plan(World, KeepAlive, EMoveTendency::MoveAway, 6, 3, 8, 2, FTileIndex(2, 1), FTileIndex(1, 0));
		CheckTail(*this, Commands, TEXT("Case2"));
		const FSRPGMoveCommand* Move = FindMoveCommand(Commands);
		if (TestTrue(TEXT("[Case2] 이동커맨드 존재"), Move != nullptr)
			&& TestTrue(TEXT("[Case2] 경로 2칸 이상"), Move->mPathTileIndexes.Num() >= 2))
		{
			// 목적지 = 경로 마지막 (MoveCommand엔 목적지 필드가 없고, FindPath가 goal을 마지막 원소로 넣음)
			const FTileIndex Dest = Move->mPathTileIndexes.Last();
			TestTrue(TEXT("[Case2] 경로 시작=origin(2,1)"), Move->mPathTileIndexes[0] == FTileIndex(2, 1));
			TestTrue(TEXT("[Case2] 목적지=(4,1) 최원거리 후퇴"), Dest == FTileIndex(4, 1));
		}
		TestTrue(TEXT("[Case2] 시전커맨드 존재"), FindCast(Commands) != nullptr);
	}

	// ============================================================
	// 시나리오 B: 이동해도 조준 불가 (사거리 밖) → 접근만, 시전 없음
	// 맵 γ (10x3): E(0,1) P(9,1). 도달 최대 x=4 < 조준 필요 x → 3성향 모두 (4,1)로 접근
	// ============================================================
	AddInfo(TEXT("=== Case4~6: 조준 불가(사거리 밖) → 접근+시전없음 (성향 무관 동일) ==="));
	{
		struct FCaseB { EMoveTendency Tendency; int32 AimRange; const TCHAR* Name; };
		const FCaseB Cases[] =
		{
			{ EMoveTendency::MoveClose, 1, TEXT("Case4-MoveClose") },
			{ EMoveTendency::MoveAway,  3, TEXT("Case5-MoveAway")  },
			{ EMoveTendency::HoldRange, 1, TEXT("Case6-HoldRange") },
		};

		for (const FCaseB& C : Cases)
		{
			const TArray<TInstancedStruct<FSRPGCommand>> Commands =
				Plan(World, KeepAlive, C.Tendency, 4, C.AimRange, 10, 3, FTileIndex(0, 1), FTileIndex(9, 1));
			CheckTail(*this, Commands, C.Name);

			const FSRPGMoveCommand* Move = FindMoveCommand(Commands);
			if (TestTrue(FString::Printf(TEXT("[%s] 이동커맨드 존재"), C.Name), Move != nullptr)
				&& TestTrue(FString::Printf(TEXT("[%s] 경로 2칸 이상"), C.Name), Move->mPathTileIndexes.Num() >= 2))
			{
				// 목적지 = 경로 마지막
				const FTileIndex Dest = Move->mPathTileIndexes.Last();
				TestTrue(FString::Printf(TEXT("[%s] 접근 목적지=(4,1)"), C.Name), Dest == FTileIndex(4, 1));
			}
			TestTrue(FString::Printf(TEXT("[%s] 시전커맨드 없음(조준 불가)"), C.Name), FindCast(Commands) == nullptr);
		}
	}

	// 루트 해제
	for (UObject* Object : KeepAlive)
	{
		if (IsValid(Object))
		{
			Object->RemoveFromRoot();
		}
	}

	return true;
}
