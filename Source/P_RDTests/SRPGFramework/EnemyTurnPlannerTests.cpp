/*****************************************************************//**
 * @file   EnemyTurnPlannerTests.cpp
 * @brief  USRPGEnemyTurnPlanner 유닛테스트
 * @details
 * 이동성향 3종(MoveClose/HoldRange/MoveAway) × 조준 가능/불가 2가지 엮어서 6가지 케이스 테스트.
 * 추가로 직사 스킬 일직선 후퇴 시 자기 차폐로 제자리 사격하던 회귀 케이스(Case1-3),
 * 다른 몹에 막혀 조준 불가일 때 우회 접근하지 않고 제자리에 서던 회귀 케이스(Case3-3),
 * 다중 스킬 랜덤 선택 케이스(Case4-1) 포함.
 * 다중 플레이어 대응: 최근접 타겟 선택(Case5-1), 시전 가능 타겟 우선(Case5-2),
 * 균형 후퇴(Case5-3), 최근접 접근 폴백(Case5-4).
 * 속박: 제자리 시전(Case6-1), 사거리 밖(Case6-2), 비용 있는 스킬 시전 회귀(Case6-3).
 * Single 조준 스킬: 영향 범위로 타격 판정(Case7-1), 자기 차폐 회귀(Case7-2), 접근 폴백(Case7-3).
 * 자기 버프: 공격 우선(Case8-1), 이동 후 남는 AP로 버프(Case8-2), 버프만 보유 시 성향 이동(Case8-3),
 *           버프 비용이 AP 전부면 버프 대신 접근(Case8-4).
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
#include "GameplayTagType.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"

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
	// @param EffectPattern 영향 범위 패턴 (Single 조준 스킬은 이 범위로 타격 여부가 결정됨)
	// @param EffectArea 영향 범위 크기
	// @param SkillCost 시전 비용 (플래너 AP 판정용, 0이면 비용 없음)
	// @param SkillType 스킬 타입 (Attack은 공격 후보, Spell은 자기 버프)
	UStaticSkillData* MakeSkill(UWorld* World, TArray<UObject*>& KeepAlive, EAimPattern AimPattern, int32 AimRange,
		EEffectPattern EffectPattern = EEffectPattern::Single, int32 EffectArea = 0, int32 SkillCost = 0,
		ESkillType SkillType = ESkillType::Attack)
	{
		UStaticUnitSkillData* Skill = NewObject<UStaticUnitSkillData>(World);
		Skill->mJobType = EUnitJobType::Common;	// 직업 무관 (직업 불일치면 SetSkill이 장착 거부)
		Skill->mSkillType = SkillType;
		Skill->mRequiredActionPoint = SkillCost;
		Skill->mAimPattern = AimPattern;
		Skill->mAimRange = AimRange;
		Skill->mCanAimBoardActor = true;
		Skill->mAimBlockerMask = static_cast<int32>(ETileLayerFlag::Obstacle | ETileLayerFlag::Unit);
		Skill->mEffectPattern = EffectPattern;
		Skill->mEffectArea = EffectArea;
		Skill->mEffectBlockerMask = static_cast<int32>(ETileLayerFlag::Obstacle | ETileLayerFlag::Unit);
		// 모션 레이어 없으면 FSkillEntry::IsValid()가 미장착으로 판정하므로 더미 1개 추가
		Skill->mSkillPhaseLayers.AddDefaulted();
		Skill->AddToRoot();
		KeepAlive.Add(Skill);
		return Skill;
	}

	// @brief 적 유닛의 한 턴을 계획해 커맨드 목록 반환
	// @param KeepAlive SkillComponent가 약참조라 GC 안당하도록 붙잡아두는 장치
	// @param AimPattern 스킬 조준 패턴 (Cross/Star 직사는 시야 검사 경로를 태움)
	// @param SecondSkillAimRange 슬롯 1에 장착할 Square 스킬의 사거리 (0이면 미장착. 스킬 랜덤 선택 검증용)
	// @param BlockerIndex 길/시야를 막는 제3의 유닛 배치 좌표 (Invalid면 미배치. 우회 접근 검증용)
	// @param SecondPlayerIndex 두 번째 플레이어 배치 좌표 (Invalid면 미배치. 다중 타겟 선택 검증용)
	// @param Rooted 속박 상태 여부 (이동 불가 계획 검증용)
	// @param EffectPattern 슬롯 0 스킬의 영향 범위 패턴 (Single 조준 검증용)
	// @param EffectArea 슬롯 0 스킬의 영향 범위 크기
	// @param SkillCost 슬롯 0 스킬의 시전 비용 (이동과 시전의 AP 분배 검증용)
	// @param SpellCost 슬롯 2에 장착할 자기 버프의 시전 비용 (0이면 미장착. 버프 턴 검증용)
	// @param HasAttack 슬롯 0 공격 스킬 장착 여부 (거짓이면 버프만 가진 적. 성향 이동 검증용)
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
		FTileIndex BlockerIndex = FTileIndex::Invalid,
		FTileIndex SecondPlayerIndex = FTileIndex::Invalid,
		bool Rooted = false,
		EEffectPattern EffectPattern = EEffectPattern::Single,
		int32 EffectArea = 0,
		int32 SkillCost = 0,
		int32 SpellCost = 0,
		bool HasAttack = true)
	{
		// 타일맵 생성
		UTileMapModel* TileMap = NewObject<UTileMapModel>(World);
		TileMap->SetDimensions(TileMapWidth, TileMapHeight);

		// 플레이어유닛 생성 (배열 순서가 곧 타겟 인덱스)
		TArray<UUnitModel*> Players;
		UMockPlayerUnitModel* Player = NewObject<UMockPlayerUnitModel>(World);
		Players.Add(Player);

		// 적유닛 생성
		UMockEnemyUnitModel* Enemy = NewObject<UMockEnemyUnitModel>(World);
		Enemy->Initialize();
		Enemy->BeginPlay();
		Enemy->SetMoveTendency(Tendency);
		Enemy->GetAttributeComponentModel()->ApplyModToAttribute(UEnemyUnitAttributeSet::GetActionPointAttribute(), ETacticalModOp::Override, MoveRange);

		// 속박 옵션: 이동불가 상태의 계획 검증용
		if (Rooted == true)
		{
			Enemy->GetAttributeComponentModel()->AddLooseGameplayTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Root, 1);
		}

		// 스킬 슬롯 풀 할당: Mock은 스폰 데이터 초기화를 건너뛰므로 빈 목록으로 슬롯만 확보
		Enemy->GetSkillComponentModel()->SetSkillFrom(TArray<TSoftObjectPtr<UStaticSkillData>>());
		// 스킬 추가: 일반공격 계열 (버프만 가진 적 검증 시 생략)
		if (HasAttack == true)
		{
			Enemy->GetSkillComponentModel()->SetSkill(0, MakeSkill(World, KeepAlive, AimPattern, AimRange, EffectPattern, EffectArea, SkillCost));
		}
		// 두 번째 스킬(옵션): 스킬 랜덤 선택 검증용
		if (SecondSkillAimRange > 0)
		{
			Enemy->GetSkillComponentModel()->SetSkill(1, MakeSkill(World, KeepAlive, EAimPattern::Square, SecondSkillAimRange));
		}
		// 자기 버프(옵션): Single 조준, 영향 범위 자기 칸
		if (SpellCost > 0)
		{
			Enemy->GetSkillComponentModel()->SetSkill(2, MakeSkill(World, KeepAlive, EAimPattern::Single, 0,
				EEffectPattern::Single, 0, SpellCost, ESkillType::Spell));
		}

		// 플레이어유닛과 적유닛 배치
		TileMap->PlaceActor(FTileTransform(PlayerIndex), Player);
		TileMap->PlaceActor(FTileTransform(EnemyIndex), Enemy);

		// 두 번째 플레이어(옵션): 다중 타겟 선택 검증용
		if (TileMap->IsValidIndex(SecondPlayerIndex))
		{
			UMockPlayerUnitModel* SecondPlayer = NewObject<UMockPlayerUnitModel>(World);
			Players.Add(SecondPlayer);
			TileMap->PlaceActor(FTileTransform(SecondPlayerIndex), SecondPlayer);
		}

		// 차단 유닛(옵션): 길/시야를 막는 제3의 유닛 (Unit 레이어 점유만 필요하니 플레이어 Mock 재사용)
		if (TileMap->IsValidIndex(BlockerIndex))
		{
			UMockPlayerUnitModel* Blocker = NewObject<UMockPlayerUnitModel>(World);
			TileMap->PlaceActor(FTileTransform(BlockerIndex), Blocker);
		}

		// 스킬 랜덤 선택용 고정 시드 스트림 (테스트 결정성 보장)
		const FRandomStream Stream(20260710);

		// AI 돌려서(ㅋㅋ) 적유닛의 예상커맨드 획득
		return USRPGEnemyTurnPlanner::PlanTurn(Enemy, Players, TileMap, Stream);
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

	/**
	 * Case5-1: 다중 플레이어 / 둘 다 시전 가능
	 *   -> 최근접 타겟을 선택해서 시전
	 * 맵 (6x3): E(2,1) P1(3,1) P2(0,1), 등거리 성향, 이동력 3, 사거리 1
	 *   -> P1(거리1)이 P2(거리2)보다 가까우므로 제자리에서 P1에게 시전
	 */
	AddInfo(TEXT("=== Case5-1: 다중 플레이어 / 최근접 타겟 선택 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::HoldRange,
			3, 1, 6, 3,
			FTileIndex(2, 1),
			FTileIndex(3, 1),
			EAimPattern::Square,
			/*SecondSkillAimRange*/0,
			/*BlockerIndex*/FTileIndex::Invalid,
			/*SecondPlayerIndex*/FTileIndex(0, 1));
		CheckTail(*this, Commands, TEXT("Case5-1"));
		TestTrue(TEXT("[Case5-1] 이동커맨드 없음(제자리에서 최근접 시전 가능)"), FindMoveCommand(Commands) == nullptr);
		const FSRPGSkillCastCommand* Cast = FindCast(Commands);
		if (TestTrue(TEXT("[Case5-1] 스킬커맨드 존재"), Cast != nullptr))
		{
			TestTrue(TEXT("[Case5-1] 타겟은 최근접 플레이어(3,1)"), Cast->mTargetIndex == FTileIndex(3, 1));
		}
	}

	/**
	 * Case5-2: 다중 플레이어 / 최근접은 차단유닛에 막혀 시전 불가
	 *   -> 시전 가능한 먼 타겟을 선택 (타겟 우선 구조 검증)
	 * 맵 (7x1): P1(0,0) M(1,0) E(2,0) P2(5,0), 근접 성향, 이동력 2, 사거리 1
	 *   -> P1은 인접 칸이 M에 막혀 시전/조준 모두 불가 -> P2에게 (4,0)으로 이동 후 시전
	 */
	AddInfo(TEXT("=== Case5-2: 다중 플레이어 / 시전 가능한 타겟 우선 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::MoveClose,
			2, 1, 7, 1,
			FTileIndex(2, 0),
			FTileIndex(0, 0),
			EAimPattern::Square,
			/*SecondSkillAimRange*/0,
			/*BlockerIndex*/FTileIndex(1, 0),
			/*SecondPlayerIndex*/FTileIndex(5, 0));
		CheckTail(*this, Commands, TEXT("Case5-2"));
		const FSRPGMoveCommand* Move = FindMoveCommand(Commands);
		if (TestTrue(TEXT("[Case5-2] 이동커맨드 존재"), Move != nullptr) &&
			TestTrue(TEXT("[Case5-2] 경로 2칸 이상"), Move->mPathTileIndexes.Num() >= 2))
		{
			TestTrue(TEXT("[Case5-2] 목적지=(4,0)"), Move->mPathTileIndexes.Last() == FTileIndex(4, 0));
		}
		const FSRPGSkillCastCommand* Cast = FindCast(Commands);
		if (TestTrue(TEXT("[Case5-2] 스킬커맨드 존재"), Cast != nullptr))
		{
			TestTrue(TEXT("[Case5-2] 타겟은 시전 가능한 두 번째 플레이어(5,0)"), Cast->mTargetIndex == FTileIndex(5, 0));
		}
	}

	/**
	 * Case5-3: 다중 플레이어 / 원거리(MoveAway) 성향의 균형 후퇴
	 *   -> 타겟에게서 멀어지되 다른 플레이어 옆으로 가면 안 됨 (최근접 거리 최대화 검증)
	 * 맵 (7x1): P1(0,0) E(2,0) P2(6,0), 이동력 2, 사거리 3
	 *   -> 타겟은 최근접 P1(거리2). 시전 가능 타일 (1,0)~(3,0) 중
	 *      두 플레이어 모두에게서 가장 먼 정중앙 (3,0) 선택
	 */
	AddInfo(TEXT("=== Case5-3: 다중 플레이어 / 균형 후퇴 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::MoveAway,
			2, 3, 7, 1,
			FTileIndex(2, 0),
			FTileIndex(0, 0),
			EAimPattern::Square,
			/*SecondSkillAimRange*/0,
			/*BlockerIndex*/FTileIndex::Invalid,
			/*SecondPlayerIndex*/FTileIndex(6, 0));
		CheckTail(*this, Commands, TEXT("Case5-3"));
		const FSRPGMoveCommand* Move = FindMoveCommand(Commands);
		if (TestTrue(TEXT("[Case5-3] 이동커맨드 존재"), Move != nullptr) &&
			TestTrue(TEXT("[Case5-3] 경로 2칸 이상"), Move->mPathTileIndexes.Num() >= 2))
		{
			TestTrue(TEXT("[Case5-3] 목적지는 두 플레이어의 정중앙(3,0)"), Move->mPathTileIndexes.Last() == FTileIndex(3, 0));
		}
		const FSRPGSkillCastCommand* Cast = FindCast(Commands);
		if (TestTrue(TEXT("[Case5-3] 스킬커맨드 존재"), Cast != nullptr))
		{
			TestTrue(TEXT("[Case5-3] 타겟은 최근접 플레이어(0,0)"), Cast->mTargetIndex == FTileIndex(0, 0));
		}
	}

	/**
	 * Case5-4: 다중 플레이어 / 전원 조준 불가
	 *   -> 최근접 플레이어에게 접근
	 * 맵 (10x3): P2(0,1) E(4,1) P1(9,1), 근접 성향, 이동력 2, 사거리 1
	 *   -> 어느 타일에서도 조준 불가 -> 최근접 P2(거리4) 쪽 (2,1)로 접근, 시전 없음
	 */
	AddInfo(TEXT("=== Case5-4: 다중 플레이어 / 최근접 접근 폴백 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::MoveClose,
			2, 1, 10, 3,
			FTileIndex(4, 1),
			FTileIndex(9, 1),
			EAimPattern::Square,
			/*SecondSkillAimRange*/0,
			/*BlockerIndex*/FTileIndex::Invalid,
			/*SecondPlayerIndex*/FTileIndex(0, 1));
		CheckApproachNoCast(*this, Commands, TEXT("Case5-4"), FTileIndex(2, 1));
	}

	/**
	 * Case6-1: 속박 / 제자리 조준 가능
	 *   -> 이동커맨드 없이 제자리에서 시전만
	 * 맵 (8x2): E(2,1) P(1,0), 원거리 성향이라 평소라면 물러났을 상황
	 */
	AddInfo(TEXT("=== Case6-1: 속박 / 제자리 조준 가능 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::MoveAway,
			6, 3, 8, 2,
			FTileIndex(2, 1),
			FTileIndex(1, 0),
			EAimPattern::Square,
			/*SecondSkillAimRange*/0,
			/*BlockerIndex*/FTileIndex::Invalid,
			/*SecondPlayerIndex*/FTileIndex::Invalid,
			/*Rooted*/true);
		CheckTail(*this, Commands, TEXT("Case6-1"));
		TestTrue(TEXT("[Case6-1] 이동커맨드 없음(속박)"), FindMoveCommand(Commands) == nullptr);
		TestTrue(TEXT("[Case6-1] 스킬커맨드 존재(제자리 시전)"), FindCast(Commands) != nullptr);
	}

	/**
	 * Case6-2: 속박 / 사거리 밖
	 *   -> 접근도 시전도 못 하고 턴 종료만
	 * 맵 (8x2): E(6,1) P(0,0), 근접 성향 + 사거리 1이라 평소라면 접근했을 상황
	 */
	AddInfo(TEXT("=== Case6-2: 속박 / 사거리 밖 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::MoveClose,
			6, 1, 8, 2,
			FTileIndex(6, 1),
			FTileIndex(0, 0),
			EAimPattern::Square,
			/*SecondSkillAimRange*/0,
			/*BlockerIndex*/FTileIndex::Invalid,
			/*SecondPlayerIndex*/FTileIndex::Invalid,
			/*Rooted*/true);
		CheckTail(*this, Commands, TEXT("Case6-2"));
		TestTrue(TEXT("[Case6-2] 이동커맨드 없음(속박)"), FindMoveCommand(Commands) == nullptr);
		TestTrue(TEXT("[Case6-2] 스킬커맨드 없음(사거리 밖)"), FindCast(Commands) == nullptr);
	}

	/**
	 * Case6-3: 속박 / 비용 있는 스킬 / 제자리 조준 가능 (이동 예산 0이 시전 예산까지 0으로 만들던 회귀 방지)
	 *   -> 이동은 못 해도 AP 6으로 비용 2 스킬은 시전 가능해야 함
	 * 맵 (6x3): E(2,1) P(3,1), 등거리 성향, AP 6, 스킬 비용 2
	 */
	AddInfo(TEXT("=== Case6-3: 속박 / 비용 있는 스킬 시전 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::HoldRange,
			6, 1, 6, 3,
			FTileIndex(2, 1),
			FTileIndex(3, 1),
			EAimPattern::Square,
			/*SecondSkillAimRange*/0,
			/*BlockerIndex*/FTileIndex::Invalid,
			/*SecondPlayerIndex*/FTileIndex::Invalid,
			/*Rooted*/true,
			EEffectPattern::Single, /*EffectArea*/0,
			/*SkillCost*/2);
		CheckTail(*this, Commands, TEXT("Case6-3"));
		TestTrue(TEXT("[Case6-3] 이동커맨드 없음(속박)"), FindMoveCommand(Commands) == nullptr);
		TestTrue(TEXT("[Case6-3] 스킬커맨드 존재(이동 불가여도 시전 예산은 AP 기준)"), FindCast(Commands) != nullptr);
	}

	/**
	 * Case7-1: Single 조준 / 영향 범위 Cross(1) / 플레이어 인접
	 *   -> 조준 타일은 자기 칸이지만 영향 범위에 플레이어가 들어오므로 제자리 시전
	 *   -> 시전 커맨드의 조준 타일은 플레이어 칸이 아니라 자기 칸이어야 함
	 * 맵 (6x3): E(2,1) P(3,1), 등거리 성향, 이동력 3
	 */
	AddInfo(TEXT("=== Case7-1: Single 조준 / 영향 범위로 인접 타격 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::HoldRange,
			3, 0, 6, 3,
			FTileIndex(2, 1),
			FTileIndex(3, 1),
			EAimPattern::Single,
			/*SecondSkillAimRange*/0,
			/*BlockerIndex*/FTileIndex::Invalid,
			/*SecondPlayerIndex*/FTileIndex::Invalid,
			/*Rooted*/false,
			EEffectPattern::Cross, /*EffectArea*/1);
		CheckTail(*this, Commands, TEXT("Case7-1"));
		TestTrue(TEXT("[Case7-1] 이동커맨드 없음(제자리 영향 범위에 플레이어 포함)"), FindMoveCommand(Commands) == nullptr);
		const FSRPGSkillCastCommand* Cast = FindCast(Commands);
		if (TestTrue(TEXT("[Case7-1] 스킬커맨드 존재"), Cast != nullptr))
		{
			TestTrue(TEXT("[Case7-1] 조준 타일은 자기 칸(2,1)"), Cast->mTargetIndex == FTileIndex(2, 1));
		}
	}

	/**
	 * Case7-2: Single 조준 / 영향 범위 Cross(2) / 출발 칸 너머의 플레이어 (자기 차폐 회귀 방지)
	 *   -> 한 칸 물러난 (2,0)에서 왼쪽으로 뻗는 영향 범위가 자기 출발 칸 (1,0)에 막히면 안 됨
	 * 맵 (8x1): P(0,0) E(1,0), 원거리 성향, 이동력 6
	 *   -> 시전 가능 타일은 (1,0)과 (2,0). 최근접 거리 최대인 (2,0)으로 이동 후 자기 칸 조준 시전
	 */
	AddInfo(TEXT("=== Case7-2: Single 조준 / 출발 칸 너머 타격 / 자기 차폐 회귀 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::MoveAway,
			6, 0, 8, 1,
			FTileIndex(1, 0),
			FTileIndex(0, 0),
			EAimPattern::Single,
			/*SecondSkillAimRange*/0,
			/*BlockerIndex*/FTileIndex::Invalid,
			/*SecondPlayerIndex*/FTileIndex::Invalid,
			/*Rooted*/false,
			EEffectPattern::Cross, /*EffectArea*/2);
		CheckTail(*this, Commands, TEXT("Case7-2"));
		const FSRPGMoveCommand* Move = FindMoveCommand(Commands);
		if (TestTrue(TEXT("[Case7-2] 이동커맨드 존재(자기 차폐로 제자리 시전이면 실패)"), Move != nullptr) &&
			TestTrue(TEXT("[Case7-2] 경로 2칸 이상"), Move->mPathTileIndexes.Num() >= 2))
		{
			TestTrue(TEXT("[Case7-2] 목적지=(2,0)"), Move->mPathTileIndexes.Last() == FTileIndex(2, 0));
		}
		const FSRPGSkillCastCommand* Cast = FindCast(Commands);
		if (TestTrue(TEXT("[Case7-2] 스킬커맨드 존재"), Cast != nullptr))
		{
			TestTrue(TEXT("[Case7-2] 조준 타일은 목적지(2,0)"), Cast->mTargetIndex == FTileIndex(2, 0));
		}
	}

	/**
	 * Case7-3: Single 조준 / 영향 범위 Cross(1) / 이동해도 닿지 않음
	 *   -> 플레이어에게 최대한 접근, 시전 없음
	 * 맵 (10x3): E(0,1) P(9,1), 근접 성향, 이동력 4
	 */
	AddInfo(TEXT("=== Case7-3: Single 조준 / 영향 범위 밖 / 접근 폴백 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::MoveClose,
			4, 0, 10, 3,
			FTileIndex(0, 1),
			FTileIndex(9, 1),
			EAimPattern::Single,
			/*SecondSkillAimRange*/0,
			/*BlockerIndex*/FTileIndex::Invalid,
			/*SecondPlayerIndex*/FTileIndex::Invalid,
			/*Rooted*/false,
			EEffectPattern::Cross, /*EffectArea*/1);
		CheckApproachNoCast(*this, Commands, TEXT("Case7-3"), FTileIndex(4, 1));
	}

	/**
	 * Case8-1: 공격 가능 + 자기 버프 보유
	 *   -> 턴당 시전 1회이므로 공격을 우선, 버프는 시전하지 않음
	 * 맵 (6x3): E(2,1) P(3,1), 등거리 성향, AP 3, 공격 사거리 1, 버프 비용 1 (슬롯 2)
	 */
	AddInfo(TEXT("=== Case8-1: 공격 가능 / 버프 보유 / 공격 우선 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::HoldRange,
			3, 1, 6, 3,
			FTileIndex(2, 1),
			FTileIndex(3, 1),
			EAimPattern::Square,
			/*SecondSkillAimRange*/0,
			/*BlockerIndex*/FTileIndex::Invalid,
			/*SecondPlayerIndex*/FTileIndex::Invalid,
			/*Rooted*/false,
			EEffectPattern::Single, /*EffectArea*/0,
			/*SkillCost*/0,
			/*SpellCost*/1);
		CheckTail(*this, Commands, TEXT("Case8-1"));
		TestTrue(TEXT("[Case8-1] 이동커맨드 없음"), FindMoveCommand(Commands) == nullptr);
		const FSRPGSkillCastCommand* Cast = FindCast(Commands);
		if (TestTrue(TEXT("[Case8-1] 스킬커맨드 존재"), Cast != nullptr))
		{
			TestEqual(TEXT("[Case8-1] 공격 슬롯 0 선택(버프 슬롯 2 아님)"), Cast->mSkillIndex, 0);
			TestTrue(TEXT("[Case8-1] 조준 타일은 플레이어 칸(3,1)"), Cast->mTargetIndex == FTileIndex(3, 1));
		}
	}

	/**
	 * Case8-2: 공격 불가 + 자기 버프 보유, 이동하고 AP가 남는 경우
	 *   -> 조준 가능한 타일로 먼저 이동하고, 남는 행동력으로 버프 시전
	 * 맵 (10x3): E(0,1) P(3,1), 근접 성향, AP 4, 공격 사거리 1, 공격 비용 3, 버프 비용 2
	 *   -> 공격은 이동2 + 시전3 = 5 > 4 로 불가 → 조준 타일 (2,1)로 선점이동(비용 2), 남은 AP 2로 버프 (조준 타일 = 목적지)
	 */
	AddInfo(TEXT("=== Case8-2: 공격 불가 / 이동 후 남는 AP로 버프 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::MoveClose,
			4, 1, 10, 3,
			FTileIndex(0, 1),
			FTileIndex(3, 1),
			EAimPattern::Square,
			/*SecondSkillAimRange*/0,
			/*BlockerIndex*/FTileIndex::Invalid,
			/*SecondPlayerIndex*/FTileIndex::Invalid,
			/*Rooted*/false,
			EEffectPattern::Single, /*EffectArea*/0,
			/*SkillCost*/3,
			/*SpellCost*/2);
		CheckTail(*this, Commands, TEXT("Case8-2"));
		const FSRPGMoveCommand* Move = FindMoveCommand(Commands);
		if (TestTrue(TEXT("[Case8-2] 이동커맨드 존재"), Move != nullptr) &&
			TestTrue(TEXT("[Case8-2] 경로 2칸 이상"), Move->mPathTileIndexes.Num() >= 2))
		{
			TestTrue(TEXT("[Case8-2] 목적지=(2,1) (조준 가능 타일로 선점이동)"), Move->mPathTileIndexes.Last() == FTileIndex(2, 1));
		}
		const FSRPGSkillCastCommand* Cast = FindCast(Commands);
		if (TestTrue(TEXT("[Case8-2] 스킬커맨드 존재(버프)"), Cast != nullptr))
		{
			TestEqual(TEXT("[Case8-2] 버프 슬롯 2 선택"), Cast->mSkillIndex, 2);
			TestTrue(TEXT("[Case8-2] 조준 타일은 목적지(2,1)"), Cast->mTargetIndex == FTileIndex(2, 1));
		}
	}

	/**
	 * Case8-4: 공격 불가 + 버프 비용이 AP 전부인 경우 (회귀: 버프만 하고 제자리에 서던 문제)
	 *   -> 버프를 포기하고 행동력 전부로 접근
	 * 맵 (10x3): E(0,1) P(9,1), 근접 성향, AP 3, 공격 사거리 1, 버프 비용 3
	 *   -> (3,1)까지 접근, 남은 AP 0이라 버프 없음
	 */
	AddInfo(TEXT("=== Case8-4: 공격 불가 / 버프 비용이 AP 전부 / 버프 대신 접근 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::MoveClose,
			3, 1, 10, 3,
			FTileIndex(0, 1),
			FTileIndex(9, 1),
			EAimPattern::Square,
			/*SecondSkillAimRange*/0,
			/*BlockerIndex*/FTileIndex::Invalid,
			/*SecondPlayerIndex*/FTileIndex::Invalid,
			/*Rooted*/false,
			EEffectPattern::Single, /*EffectArea*/0,
			/*SkillCost*/0,
			/*SpellCost*/3);
		CheckApproachNoCast(*this, Commands, TEXT("Case8-4"), FTileIndex(3, 1));
	}

	/**
	 * Case8-3: 버프만 보유 (공격 스킬 없음) + 원거리 성향
	 *   -> 접근 폴백이 아니라 이동 성향대로 멀어진 뒤 버프 시전
	 * 맵 (8x1): P(0,0) E(2,0), 원거리 성향, AP 4, 버프 비용 1
	 *   -> 이동 예산 3 → 최근접 거리 최대인 (5,0)으로 후퇴, 거기서 버프
	 */
	AddInfo(TEXT("=== Case8-3: 버프만 보유 / 성향 이동 후 버프 ==="));
	{
		const TArray<TInstancedStruct<FSRPGCommand>> Commands = Plan(
			World, KeepAlive, EMoveTendency::MoveAway,
			4, 1, 8, 1,
			FTileIndex(2, 0),
			FTileIndex(0, 0),
			EAimPattern::Square,
			/*SecondSkillAimRange*/0,
			/*BlockerIndex*/FTileIndex::Invalid,
			/*SecondPlayerIndex*/FTileIndex::Invalid,
			/*Rooted*/false,
			EEffectPattern::Single, /*EffectArea*/0,
			/*SkillCost*/0,
			/*SpellCost*/1,
			/*HasAttack*/false);
		CheckTail(*this, Commands, TEXT("Case8-3"));
		const FSRPGMoveCommand* Move = FindMoveCommand(Commands);
		if (TestTrue(TEXT("[Case8-3] 이동커맨드 존재(접근이 아닌 후퇴)"), Move != nullptr) &&
			TestTrue(TEXT("[Case8-3] 경로 2칸 이상"), Move->mPathTileIndexes.Num() >= 2))
		{
			TestTrue(TEXT("[Case8-3] 목적지=(5,0) (이동 예산 3으로 최대 후퇴)"), Move->mPathTileIndexes.Last() == FTileIndex(5, 0));
		}
		const FSRPGSkillCastCommand* Cast = FindCast(Commands);
		if (TestTrue(TEXT("[Case8-3] 스킬커맨드 존재(버프)"), Cast != nullptr))
		{
			TestEqual(TEXT("[Case8-3] 버프 슬롯 2 선택"), Cast->mSkillIndex, 2);
			TestTrue(TEXT("[Case8-3] 조준 타일은 목적지(5,0)"), Cast->mTargetIndex == FTileIndex(5, 0));
		}
	}

	// GC 안당하려고 KeepAlive에 마달아놨던 SkillComponent 연결 해제 -> GC 대상
	for (UObject* Object : KeepAlive)
		if (IsValid(Object))
			Object->RemoveFromRoot();

	return true;
}
