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
#include "Component/SkillComponent/SkillComponentModel.h"
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
		Enemy->SetMovePoint(MoveRange);

		// 스킬 추가: 일반공격 계열
		UStaticAttackSkillData* Skill = NewObject<UStaticAttackSkillData>(World);
		Skill->mAimPattern = EAimPattern::Square;
		Skill->mAimRangeDefaultValue = AimRange;
		Skill->mCanAimBoardActor = true;
		Skill->mIsIndirect = false;
		Skill->mEffectPattern = EEffectPattern::Single;
		Skill->mEffectAreaDefaultValue = 0;
		Skill->mIsPenetration = false;
		Skill->AddToRoot();
		KeepAlive.Add(Skill);
		Enemy->GetSkillComponentModel()->SetSkill(0, Skill);

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

	// @brief 이동해서도 플레이어를 조준 불가능한 케이스 검증
	void CheckApproachNoCast(
		FAutomationTestBase& Test,
		const TArray<TInstancedStruct<FSRPGCommand>>& Commands,
		const TCHAR* Label,
		FTileIndex ExpectedDest)
	{
		CheckTail(Test, Commands, Label);

		// 이동커맨드가 있고 경로가 2칸 이상이어야 목적지 검증이 의미 있음
		const FSRPGMoveCommand* Move = FindMoveCommand(Commands);
		if (Test.TestTrue(FString::Printf(TEXT("[%s] 이동커맨드 존재"), Label), Move != nullptr) &&
			Test.TestTrue(FString::Printf(TEXT("[%s] 경로는 2칸 이상"), Label), Move->mPathTileIndexes.Num() >= 2))
		{
			// 목적지 = 경로 마지막 (FindPath가 goal을 마지막 원소로 넣음)
			const FTileIndex Dest = Move->mPathTileIndexes.Last();
			Test.TestTrue(FString::Printf(TEXT("[%s] 목적지=(%d,%d)"), Label, ExpectedDest.mX, ExpectedDest.mY),
				Dest == ExpectedDest);
		}

		// 도착해서도 조준범위 밖이라 스킬캐스팅 없어야 함
		Test.TestTrue(FString::Printf(TEXT("[%s] 스킬커맨드 없음(이동 후에도 조준 불가능)"), Label), FindCast(Commands) == nullptr);
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

	/**
	 * 케이스 레이블: CaseX-Y
	 *   X = 이동성향  (1:원거리/MoveAway, 2:등거리/HoldRange, 3:근거리/MoveClose)
	 *   Y = 조준 여부 (1:가능, 2:불가)
	 * 조준 불가는 "이동 후에도 조준범위 밖"인 경우.
	 */

	/**
	 * Case1-1: 원거리(MoveAway) / 조준 가능
	 *   -> 최대조준거리로 이동 후 스킬 사용
	 * 맵 (8x2): E(2,1) P(1,0)
	 *   -> 최대 원거리 타격가능지점인 (4,1)로 이동
	 */
	AddInfo(TEXT("=== Case1-1: 원거리(MoveAway), 조준 가능 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::MoveAway,
			6, 3, 8, 2,
			FTileIndex(2, 1),
			FTileIndex(1, 0));
		CheckTail(*this, Commands, TEXT("Case1-1"));
		const FSRPGMoveCommand* Move = FindMoveCommand(Commands);
		if (TestTrue(TEXT("[Case1-1] 이동커맨드 존재"), Move != nullptr) &&
			TestTrue(TEXT("[Case1-1] 경로 2칸 이상"), Move->mPathTileIndexes.Num() >= 2))
		{
			// 이동 경로 확인
			const FTileIndex Dest = Move->mPathTileIndexes.Last();
			TestTrue(TEXT("[Case1-1] 출발지=(2,1)"), Move->mPathTileIndexes[0] == FTileIndex(2, 1));
			TestTrue(TEXT("[Case1-1] 목적지=(4,1)"), Dest == FTileIndex(4, 1));
		}
		TestTrue(TEXT("[Case1-1] 스킬커맨드 존재"), FindCast(Commands) != nullptr);
	}

	/**
	 * Case1-2: 원거리(MoveAway) / 이동 후에도 조준 불가능
	 *   -> 플레이어에게 최대한 접근
	 * 맵 (10x3): E(0,1) P(9,1)
	 *   -> 이동포인트를 최대한 사용해서 (4,1)로 접근
	 */
	AddInfo(TEXT("=== Case1-2: 원거리(MoveAway) / 조준 불가능 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::MoveAway,
			4, 3, 10, 3,
			FTileIndex(0, 1),
			FTileIndex(9, 1));
		CheckApproachNoCast(*this, Commands, TEXT("Case1-2"), FTileIndex(4, 1));
	}

	/**
	 * Case2-1: 등거리(HoldRange) / 조준 가능
	 *   -> 현재위치에서 조준되므로 이동없음 + 스킬사용
	 * 맵 (6x3): E(2,1) P(3,1)
	 *   -> 이동커맨드 없고, 바로 스킬 사용
	 */
	AddInfo(TEXT("=== Case2-1: 등거리(HoldRange) / 조준 가능 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::HoldRange,
			3, 1, 6, 3,
			FTileIndex(2, 1),
			FTileIndex(3, 1));
		CheckTail(*this, Commands, TEXT("Case2-1"));
		TestTrue(TEXT("[Case2-1] 이동커맨드 없음(현재위치에서 스킬 사용이 가능하니까)"), FindMoveCommand(Commands) == nullptr);
		TestTrue(TEXT("[Case2-1] 스킬커맨드 존재"), FindCast(Commands) != nullptr);
	}

	/**
	 * Case2-2: 등거리(HoldRange) / 이동 후에도 조준 불가능
	 *   -> 플레이어에게 최대한 접근
	 * 맵과 결과는 Case1-2와 동일
	 */
	AddInfo(TEXT("=== Case2-2: 등거리(HoldRange) / 조준 불가능 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::HoldRange,
			4, 1, 10, 3,
			FTileIndex(0, 1),
			FTileIndex(9, 1));
		CheckApproachNoCast(*this, Commands, TEXT("Case2-2"), FTileIndex(4, 1));
	}

	/**
	 * Case3-1: 근거리(MoveClose) / 조준 가능
	 *   -> 현재위치에서 조준되므로 이동없음 + 스킬사용
	 * 맵 (6x3): E(2,1) P(3,1)
	 *   -> 이동커맨드 없고, 바로 스킬 사용
	 */
	AddInfo(TEXT("=== Case3-1: 근거리(MoveClose) / 조준 가능 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::MoveClose,
			3, 1, 6, 3,
			FTileIndex(2, 1),
			FTileIndex(3, 1));
		CheckTail(*this, Commands, TEXT("Case3-1"));
		TestTrue(TEXT("[Case3-1] 이동커맨드 없음(현재위치에서 스킬 사용 가능)"), FindMoveCommand(Commands) == nullptr);
		const FSRPGSkillCastCommand* Cast = FindCast(Commands);
		if (TestTrue(TEXT("[Case3-1] 스킬커맨드 존재"), Cast != nullptr))
		{
			TestEqual(TEXT("[Case3-1] 스킬 인덱스 0"), Cast->mSkillIndex, 0);
			TestTrue(TEXT("[Case3-1] 타겟이 플레이어 타일"), Cast->mTargetIndex == FTileIndex(3, 1));
		}
	}

	/**
	 * Case3-2: 근거리(MoveClose) / 이동 후에도 조준 불가능
	 *   -> 플레이어에게 최대한 접근
	 * 맵과 결과는 Case1-2와 동일
	 */
	AddInfo(TEXT("=== Case3-2: 근거리(MoveClose) / 이동 후에도 조준 불가능 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::MoveClose,
			4, 1, 10, 3,
			FTileIndex(0, 1),
			FTileIndex(9, 1));
		CheckApproachNoCast(*this, Commands, TEXT("Case3-2"), FTileIndex(4, 1));
	}

	// GC 안당하려고 KeepAlive에 마달아놨던 SkillComponent 연결 해제 -> GC 대상
	for (UObject* Object : KeepAlive)
		if (IsValid(Object))
			Object->RemoveFromRoot();

	return true;
}
