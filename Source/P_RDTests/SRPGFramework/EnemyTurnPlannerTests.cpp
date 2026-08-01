/*****************************************************************//**
 * @file   EnemyTurnPlannerTests.cpp
 * @brief  USRPGEnemyTurnPlanner 유닛테스트
 * @details
 * 이동성향 3종(MoveClose/HoldRange/MoveAway) × 조준 가능/불가 2가지 엮어서 6가지 케이스 테스트.
 * 추가로 직사 스킬 일직선 후퇴 시 자기 차폐로 제자리 사격하던 회귀 케이스(Case1-3),
 * 다른 몹에 막혀 조준 불가일 때 우회 접근하지 않고 제자리에 서던 회귀 케이스(Case3-3),
 * 다중 스킬 랜덤 선택 케이스(Case4-1) 포함.
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

	// @brief 테스트용 일반공격 스킬 생성 (KeepAlive에 등록해 GC 방지)
	UStaticAttackSkillData* MakeSkill(UWorld* World, TArray<UObject*>& KeepAlive, EAimPattern AimPattern, int32 AimRange)
	{
		UStaticAttackSkillData* Skill = NewObject<UStaticAttackSkillData>(World);
		Skill->mAimPattern = AimPattern;
		Skill->mAimRangeDefaultValue = AimRange;
		Skill->mCanAimBoardActor = true;
		Skill->mAimBlockerMask = static_cast<int32>(ETileLayerFlag::Obstacle | ETileLayerFlag::Unit);
		Skill->mEffectPattern = EEffectPattern::Single;
		Skill->mEffectAreaDefaultValue = 0;
		Skill->mEffectBlockerMask = static_cast<int32>(ETileLayerFlag::Obstacle | ETileLayerFlag::Unit);
		Skill->AddToRoot();
		KeepAlive.Add(Skill);
		return Skill;
	}

	// @brief 적 유닛의 한 턴을 계획해 커맨드 목록 반환
	// @param KeepAlive SkillComponent가 약참조라 GC 안당하도록 붙잡아두는 장치
	// @param AimPattern 스킬 조준 패턴 (Cross/Star 직사는 시야 검사 경로를 태움)
	// @param SecondSkillAimRange 슬롯 1에 장착할 Square 스킬의 사거리 (0이면 미장착. 스킬 랜덤 선택 검증용)
	// @param BlockerIndex 길/시야를 막는 제3의 유닛 배치 좌표 (Invalid면 미배치. 우회 접근 검증용)
	TArray<TInstancedStruct<FSRPGCommand>> Plan(
		UWorld* World,
		TArray<UObject*>& KeepAlive,
		EMoveTendency Tendency,
		int32 MoveRange,
		int32 AimRange,
		int32 TileMapWidth,
		int32 TileMapHeight,
		FTileIndex EnemyIndex,
		FTileIndex PlayerIndex,
		EAimPattern AimPattern = EAimPattern::Square,
		int32 SecondSkillAimRange = 0,
		FTileIndex BlockerIndex = FTileIndex::Invalid)
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
		Enemy->GetAttributeComponentModel()->ApplyModToAttribute(UEnemyUnitAttributeSet::GetMovementAttribute(), ETacticalModOp::Override, MoveRange);
		Enemy->GetAttributeComponentModel()->ApplyModToAttribute(UEnemyUnitAttributeSet::GetRechargeDiceSumAttribute(), ETacticalModOp::Override, 0);

		// 스킬 추가: 일반공격 계열
		Enemy->GetSkillComponentModel()->SetSkill(0, MakeSkill(World, KeepAlive, AimPattern, AimRange));
		// 두 번째 스킬(옵션): 스킬 랜덤 선택 검증용
		if (SecondSkillAimRange > 0)
		{
			Enemy->GetSkillComponentModel()->SetSkill(1, MakeSkill(World, KeepAlive, EAimPattern::Square, SecondSkillAimRange));
		}

		// 플레이어유닛과 적유닛 배치
		TileMap->PlaceActor(FTileTransform(PlayerIndex), Player);
		TileMap->PlaceActor(FTileTransform(EnemyIndex), Enemy);

		// 차단 유닛(옵션): 길/시야를 막는 제3의 유닛 (Unit 레이어 점유만 필요하니 플레이어 Mock 재사용)
		if (TileMap->IsValidIndex(BlockerIndex))
		{
			UMockPlayerUnitModel* Blocker = NewObject<UMockPlayerUnitModel>(World);
			TileMap->PlaceActor(FTileTransform(BlockerIndex), Blocker);
		}

		// 스킬 랜덤 선택용 고정 시드 스트림 (테스트 결정성 보장)
		const FRandomStream Stream(20260710);

		// AI 돌려서(ㅋㅋ) 적유닛의 예상커맨드 획득
		return USRPGEnemyTurnPlanner::PlanTurn(Enemy, Player, TileMap, Stream);
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
	 * Case1-3: 원거리(MoveAway) / 직사(Cross) / 플레이어와 일직선
	 *   -> 후퇴 타일의 시야가 자기 자신에게 막히면 안 됨 (제자리 사격 회귀 방지)
	 * 맵 (8x1): E(1,0) P(0,0)
	 *   -> 사거리 최대 지점인 (4,0)으로 후퇴 후 스킬 사용
	 */
	AddInfo(TEXT("=== Case1-3: 원거리(MoveAway) / 직사 / 일직선 후퇴 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::MoveAway,
			6, 4, 8, 1,
			FTileIndex(1, 0),
			FTileIndex(0, 0),
			EAimPattern::Cross);
		CheckTail(*this, Commands, TEXT("Case1-3"));
		const FSRPGMoveCommand* Move = FindMoveCommand(Commands);
		if (TestTrue(TEXT("[Case1-3] 이동커맨드 존재(제자리 사격이면 실패)"), Move != nullptr) &&
			TestTrue(TEXT("[Case1-3] 경로 2칸 이상"), Move->mPathTileIndexes.Num() >= 2))
		{
			// 이동 경로 확인
			const FTileIndex Dest = Move->mPathTileIndexes.Last();
			TestTrue(TEXT("[Case1-3] 출발지=(1,0)"), Move->mPathTileIndexes[0] == FTileIndex(1, 0));
			TestTrue(TEXT("[Case1-3] 목적지=(4,0)"), Dest == FTileIndex(4, 0));
		}
		TestTrue(TEXT("[Case1-3] 스킬커맨드 존재"), FindCast(Commands) != nullptr);
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

	/**
	 * Case3-3: 근거리(MoveClose) / 다른 몹이 앞을 막아 조준 불가 (제자리 멈춤 회귀 방지)
	 *   -> 맨해튼 거리로는 가까워지는 칸이 없어도, 우회 경로로 실제 가까워지는 칸으로 이동해야 함
	 * 맵 (8x2): P(0,1) M(1,1) E(2,1), 이동력 2, 직사(Cross) 사거리 4
	 *   -> 어느 도달 칸에서도 조준 불가 (열 정렬 칸은 이동력 밖, 행 정렬 칸은 M에 차폐)
	 *   -> 경로 거리 기준 최선인 (1,0)으로 우회 접근, 스킬 없음
	 */
	AddInfo(TEXT("=== Case3-3: 근거리(MoveClose) / 몹에 막힘 / 우회 접근 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::MoveClose,
			2, 4, 8, 2,
			FTileIndex(2, 1),
			FTileIndex(0, 1),
			EAimPattern::Cross,
			/*SecondSkillAimRange*/0,
			/*BlockerIndex*/FTileIndex(1, 1));
		CheckApproachNoCast(*this, Commands, TEXT("Case3-3"), FTileIndex(1, 0));
	}

	/**
	 * Case4-1: 다중 스킬 / 둘 다 조준 가능
	 *   -> 장착된 스킬 중 하나가 랜덤 선택되어 시전 (어느 쪽이든 유효 슬롯이어야 함)
	 * 맵 (6x3): E(2,1) P(3,1), 슬롯0 사거리1 + 슬롯1 사거리2 (둘 다 제자리에서 조준 가능)
	 *   -> 이동커맨드 없고, 스킬 인덱스는 0 또는 1
	 */
	AddInfo(TEXT("=== Case4-1: 다중 스킬 / 랜덤 선택 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::HoldRange,
			3, 1, 6, 3,
			FTileIndex(2, 1),
			FTileIndex(3, 1),
			EAimPattern::Square,
			/*SecondSkillAimRange*/2);
		CheckTail(*this, Commands, TEXT("Case4-1"));
		TestTrue(TEXT("[Case4-1] 이동커맨드 없음(현재위치에서 스킬 사용 가능)"), FindMoveCommand(Commands) == nullptr);
		const FSRPGSkillCastCommand* Cast = FindCast(Commands);
		if (TestTrue(TEXT("[Case4-1] 스킬커맨드 존재"), Cast != nullptr))
		{
			TestTrue(TEXT("[Case4-1] 스킬 인덱스는 장착 슬롯(0 또는 1)"), Cast->mSkillIndex == 0 || Cast->mSkillIndex == 1);
		}
	}

	// GC 안당하려고 KeepAlive에 마달아놨던 SkillComponent 연결 해제 -> GC 대상
	for (UObject* Object : KeepAlive)
		if (IsValid(Object))
			Object->RemoveFromRoot();

	return true;
}
