/*****************************************************************//**
 * @file   GimmickModelTests.cpp
 * @brief  기믹 유닛테스트 — 진입 트리거(발동/수명/필터/밀치기), 장판(라운드 끝 발동/교체/일괄 발동 이벤트)
 * @details
 *  베리어 구독자가 없는 시뮬레이션모드에서는 스킬이 동기로 완주하므로,
 *  Mock 기믹에 타일맵을 주입해서 트리거/수명/효과 적용을 자동화로 검증
 * @author 이문환
 * @date   2026-08-21
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "Actor/BoardActor/GimmickTestsHelper.h"                        // UMockOverlapGimmickModel, UMockGimmickVictimUnitModel
#include "Component/BoardMovementComponent/BoardMovementTestsHelper.h"  // UMockUnitMovementComponentModel

#include "GameplayTagType.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Stun.h"
#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Push.h"
#include "TAS/Effect/Cooldown/TacticalEffect_Cooldown.h"
#include "TAS/Effect/TacticalEffectQuery.h"

#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	// @brief 테스트용 월드 획득 (PIE/Game 우선)
	UWorld* GetAnyGameWorldForGimmickTests()
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

	// @brief 기믹 스킬 공통 골격 (단타, 자기 타일 조준, 전 팀 타격)
	UStaticSkillData* MakeGimmickSkillDataBase(UObject* Outer)
	{
		UStaticSkillData* SkillData = NewObject<UStaticSkillData>(Outer);
		// 스킬 시전 시 쿨다운 스펙을 만들므로 실제 기믹 DA와 같은 라운드 쿨다운 지정 (미지정 시 스펙이 무효)
		SkillData->mCooldownEffectClass = UTacticalEffect_RoundCooldown::StaticClass();
		SkillData->mSkillAnimationSet.mApplyMotionTags.Add(AnimationTags::Animation_Montage_Skill_Melee_Punch);
		SkillData->mSkillAnimationSet.mAutoRotateTowardTarget = false;
		SkillData->mTargetPattern = ETargetPattern::TargetOnly;
		SkillData->mEffectPattern = EEffectPattern::Single;

		FSkillPhaseLayer Phase;
		Phase.mTeamAttitudeFilter = StaticCast<int32>(ETeamAttitudeFilter::All);
		SkillData->mSkillPhaseLayers.Add(Phase);
		return SkillData;
	}

	// @brief 기절 트랩 스킬 (기절 태그 1 부여)
	UStaticSkillData* MakeStunSkillData(UObject* Outer)
	{
		UStaticSkillData* SkillData = MakeGimmickSkillDataBase(Outer);

		TInstancedStruct<FSkillEffectLayer> Layer;
		Layer.InitializeAs<FSkillEffectLayer_Stun>();
		Layer.GetMutable<FSkillEffectLayer_Stun>().mTagGain = 1;
		SkillData->mSkillPhaseLayers[0].mSkillEffectLayers.Add(Layer);
		return SkillData;
	}

	// @brief 밀치기 발판 스킬 (시전자 방향으로 밀침)
	UStaticSkillData* MakePushSkillData(UObject* Outer, int32 PushDistance)
	{
		UStaticSkillData* SkillData = MakeGimmickSkillDataBase(Outer);

		TInstancedStruct<FSkillEffectLayer> Layer;
		Layer.InitializeAs<FSkillEffectLayer_Push>();
		Layer.GetMutable<FSkillEffectLayer_Push>().mPushDistance = PushDistance;
		SkillData->mSkillPhaseLayers[0].mSkillEffectLayers.Add(Layer);
		return SkillData;
	}

	// @brief 테스트 공용 픽스처 (타일맵 + 기믹 + 피해자 유닛 + 타일맵 주입 컴포넌트)
	struct FGimmickFixture
	{
		UTileMapModel* TileMap = nullptr;
		UMockOverlapGimmickModel* Gimmick = nullptr;
		UMockGimmickVictimUnitModel* Unit = nullptr;
		UMockUnitMovementComponentModel* Movement = nullptr;
	};

	// @brief 8x8 타일맵에 기믹과 유닛을 배치하고 스킬/수명을 세팅
	FGimmickFixture MakeGimmickFixture(
		UWorld* World,
		const FTileTransform& GimmickTransform,
		const FTileTransform& UnitTransform,
		UStaticSkillData* SkillData,
		int32 TriggerCount)
	{
		FGimmickFixture Fixture;

		Fixture.TileMap = NewObject<UTileMapModel>(World);
		Fixture.TileMap->SetDimensions(8, 8);

		// 기믹: 타일맵 주입 + 스킬 장착 + 수명 세팅 후 배치
		Fixture.Gimmick = NewObject<UMockOverlapGimmickModel>(World);
		Fixture.Gimmick->Initialize();
		Fixture.Gimmick->BeginPlay();
		Fixture.Gimmick->SetTileMap(Fixture.TileMap);
		Fixture.Gimmick->GetSkillComponentModel()->SetSkillFrom(TArray<TSoftObjectPtr<UStaticSkillData>>{ SkillData });
		Fixture.Gimmick->SetRemainingTriggerCount(TriggerCount);
		Fixture.TileMap->PlaceActor(GimmickTransform, Fixture.Gimmick);

		// 피해자 유닛: 타일맵 주입 이동 컴포넌트 연결 후 배치
		Fixture.Unit = NewObject<UMockGimmickVictimUnitModel>(World);
		Fixture.Unit->Initialize();
		Fixture.Unit->BeginPlay();
		Fixture.Movement = NewObject<UMockUnitMovementComponentModel>(Fixture.Unit);
		Fixture.Movement->SetTileMap(Fixture.TileMap);
		Fixture.Unit->SetBoardMovementComponentModel(Fixture.Movement);
		Fixture.TileMap->PlaceActor(UnitTransform, Fixture.Unit);

		return Fixture;
	}

	// @brief 기절 태그 보유 여부
	bool HasStunTag(const UMockGimmickVictimUnitModel* Unit)
	{
		return Unit->GetAttributeComponentModel()->HasMatchingGameplayTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Stun);
	}

	// @brief 기절 스택 수 (장판처럼 여러 번 맞는 케이스의 발동 횟수 검증용)
	// 기절은 스택형 지속 효과라 태그는 1개만 붙고, 맞은 횟수는 스택에 누적됨
	int32 GetStunCount(const UMockGimmickVictimUnitModel* Unit)
	{
		const FTacticalEffectQuery Query = FTacticalEffectQuery::MakeQuery_MatchAnyEffectTags(
			FGameplayTagContainer(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Stun));
		return Unit->GetAttributeComponentModel()->GetAggregatedStackCount(Query);
	}

	// @brief 장판 테스트 픽스처 (타일맵 + 장판 + 피해자 유닛)
	struct FPuddleFixture
	{
		UTileMapModel* TileMap = nullptr;
		UMockPuddleGimmickModel* Puddle = nullptr;
		UMockGimmickVictimUnitModel* Unit = nullptr;
		UMockUnitMovementComponentModel* Movement = nullptr;
	};

	// @brief 장판 생성/배치 (타일맵 주입 + 스킬 장착 + 라운드 수명 세팅)
	UMockPuddleGimmickModel* MakePuddle(UWorld* World, UTileMapModel* TileMap, const FTileTransform& Transform, UStaticSkillData* SkillData, int32 RoundCount)
	{
		UMockPuddleGimmickModel* Puddle = NewObject<UMockPuddleGimmickModel>(World);
		Puddle->Initialize();
		Puddle->BeginPlay();
		Puddle->SetTileMap(TileMap);
		Puddle->GetSkillComponentModel()->SetSkillFrom(TArray<TSoftObjectPtr<UStaticSkillData>>{ SkillData });
		Puddle->SetRemainingRoundCount(RoundCount);
		TileMap->PlaceActor(Transform, Puddle);
		return Puddle;
	}

	// @brief 8x8 타일맵에 장판과 유닛을 배치
	FPuddleFixture MakePuddleFixture(
		UWorld* World,
		const FTileTransform& PuddleTransform,
		const FTileTransform& UnitTransform,
		UStaticSkillData* SkillData,
		int32 RoundCount)
	{
		FPuddleFixture Fixture;

		Fixture.TileMap = NewObject<UTileMapModel>(World);
		Fixture.TileMap->SetDimensions(8, 8);

		Fixture.Puddle = MakePuddle(World, Fixture.TileMap, PuddleTransform, SkillData, RoundCount);

		// 피해자 유닛: 타일맵 주입 이동 컴포넌트 연결 후 배치
		Fixture.Unit = NewObject<UMockGimmickVictimUnitModel>(World);
		Fixture.Unit->Initialize();
		Fixture.Unit->BeginPlay();
		Fixture.Movement = NewObject<UMockUnitMovementComponentModel>(Fixture.Unit);
		Fixture.Movement->SetTileMap(Fixture.TileMap);
		Fixture.Unit->SetBoardMovementComponentModel(Fixture.Movement);
		Fixture.TileMap->PlaceActor(UnitTransform, Fixture.Unit);

		return Fixture;
	}

	// @brief 추가 피해자 유닛 생성/배치 (2번째 유닛이 필요한 케이스용)
	UMockGimmickVictimUnitModel* MakeVictimUnit(UWorld* World, UTileMapModel* TileMap, const FTileTransform& UnitTransform, UMockUnitMovementComponentModel*& OutMovement)
	{
		UMockGimmickVictimUnitModel* Unit = NewObject<UMockGimmickVictimUnitModel>(World);
		Unit->Initialize();
		Unit->BeginPlay();
		OutMovement = NewObject<UMockUnitMovementComponentModel>(Unit);
		OutMovement->SetTileMap(TileMap);
		Unit->SetBoardMovementComponentModel(OutMovement);
		TileMap->PlaceActor(UnitTransform, Unit);
		return Unit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGimmickTriggerTests,
	"P_RD.SRPG.Gimmick.Trigger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief 진입 발동과 수명 검증
 *  1) 트랩 타일을 지나가면 발동 (효과 적용, 수명 차감, 이동은 계속)
 *  2) 수명 소진 시 사망 태그, 이후 진입은 미발동
 */
bool FGimmickTriggerTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForGimmickTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	/* Case1: 밟으면 발동, 이동은 계속 */
	AddInfo(TEXT("=== Case1: 트랩 통과 -> 기절 부여, 수명 차감, 목적지 도착 ==="));

	// (2,2)의 기절 트랩(수명 1) 위를 (1,2)->(3,2) 경로로 통과
	FGimmickFixture Fixture = MakeGimmickFixture(
		World,
		FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward),
		FTileTransform(FTileIndex(1, 2), ETileActorDirection::Forward),
		MakeStunSkillData(World),
		1);

	Fixture.Movement->MoveAlongPath({ FTileIndex(1, 2), FTileIndex(2, 2), FTileIndex(3, 2) });

	TestTrue(TEXT("[Case1] 밟은 유닛에 기절 태그 부여"), HasStunTag(Fixture.Unit));
	TestEqual(TEXT("[Case1] 수명 1 -> 0"), Fixture.Gimmick->GetRemainingTriggerCount(), 0);
	TestTrue(TEXT("[Case1] 트랩 발동에도 이동 계속 (목적지 도착)"), Fixture.Unit->GetTileTransform().mIndex == FTileIndex(3, 2));
	TestTrue(TEXT("[Case1] 수명 소진으로 사망 태그"), Fixture.Gimmick->IsDead());

	/* Case2: 소진된 트랩은 미발동 */
	AddInfo(TEXT("=== Case2: 소진 후 두 번째 유닛 통과 -> 미발동 ==="));

	UMockUnitMovementComponentModel* SecondMovement = nullptr;
	UMockGimmickVictimUnitModel* SecondUnit = MakeVictimUnit(World, Fixture.TileMap, FTileTransform(FTileIndex(1, 2), ETileActorDirection::Forward), SecondMovement);

	SecondMovement->MoveAlongPath({ FTileIndex(1, 2), FTileIndex(2, 2) });

	TestFalse(TEXT("[Case2] 두 번째 유닛은 기절 없음"), HasStunTag(SecondUnit));
	TestEqual(TEXT("[Case2] 수명 0 유지"), Fixture.Gimmick->GetRemainingTriggerCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGimmickLifetimeFilterTests,
	"P_RD.SRPG.Gimmick.LifetimeFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief 무제한 수명과 발동 대상 필터 검증
 *  1) 수명 음수(무제한): 여러 번 발동해도 수명 유지, 사망 없음
 *  2) 발동 대상 레이어가 아닌 액터(다른 Overlay 액터) 진입은 무시
 *  3) 발밑 기믹은 조준/피격 대상이 아님
 */
bool FGimmickLifetimeFilterTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForGimmickTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	/* Case1: 무제한 수명 */
	AddInfo(TEXT("=== Case1: 수명 -1 트랩은 반복 발동, 사망 없음 ==="));

	FGimmickFixture Fixture = MakeGimmickFixture(
		World,
		FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward),
		FTileTransform(FTileIndex(1, 2), ETileActorDirection::Forward),
		MakeStunSkillData(World),
		-1);

	// 첫 번째 유닛 통과
	Fixture.Movement->MoveAlongPath({ FTileIndex(1, 2), FTileIndex(2, 2), FTileIndex(3, 2) });
	TestTrue(TEXT("[Case1] 첫 유닛 기절"), HasStunTag(Fixture.Unit));

	// 두 번째 유닛 통과
	UMockUnitMovementComponentModel* SecondMovement = nullptr;
	UMockGimmickVictimUnitModel* SecondUnit = MakeVictimUnit(World, Fixture.TileMap, FTileTransform(FTileIndex(1, 2), ETileActorDirection::Forward), SecondMovement);
	SecondMovement->MoveAlongPath({ FTileIndex(1, 2), FTileIndex(2, 2) });

	TestTrue(TEXT("[Case1] 두 번째 유닛도 기절"), HasStunTag(SecondUnit));
	TestEqual(TEXT("[Case1] 무제한 수명 유지"), Fixture.Gimmick->GetRemainingTriggerCount(), -1);
	TestFalse(TEXT("[Case1] 사망 태그 없음"), Fixture.Gimmick->IsDead());

	/* Case2: 발동 대상 레이어 필터 */
	AddInfo(TEXT("=== Case2: 다른 Overlay 액터 진입 -> 미발동 ==="));
	{
		FGimmickFixture FilterFixture = MakeGimmickFixture(
			World,
			FTileTransform(FTileIndex(5, 5), ETileActorDirection::Forward),
			FTileTransform(FTileIndex(1, 1), ETileActorDirection::Forward),
			MakeStunSkillData(World),
			1);

		// 트랩 타일 위에 다른 Overlay 기믹을 배치 (유닛 레이어가 아니므로 발동하면 안 됨)
		UMockOverlapGimmickModel* OtherOverlay = NewObject<UMockOverlapGimmickModel>(World);
		OtherOverlay->Initialize();
		OtherOverlay->BeginPlay();
		OtherOverlay->SetTileMap(FilterFixture.TileMap);
		OtherOverlay->GetSkillComponentModel()->SetSkillFrom(TArray<TSoftObjectPtr<UStaticSkillData>>{ MakeStunSkillData(World) });
		OtherOverlay->SetRemainingTriggerCount(1);
		FilterFixture.TileMap->PlaceActor(FTileTransform(FTileIndex(5, 5), ETileActorDirection::Forward), OtherOverlay);

		TestEqual(TEXT("[Case2] 트랩 수명 유지 (미발동)"), FilterFixture.Gimmick->GetRemainingTriggerCount(), 1);
		TestEqual(TEXT("[Case2] 진입한 Overlay 액터도 미발동"), OtherOverlay->GetRemainingTriggerCount(), 1);
	}

	/* Case3: 조준/피격 대상 제외 */
	AddInfo(TEXT("=== Case3: 발밑 기믹은 IsTargetable false ==="));
	TestFalse(TEXT("[Case3] 트랩은 조준 불가"), Fixture.Gimmick->IsTargetable());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGimmickPlaceAndPushTests,
	"P_RD.SRPG.Gimmick.PlaceAndPush",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief 배치 즉시 발동과 밀치기 발판 검증
 *  1) 트랩 타일에 유닛을 직접 배치(방 시작 상황) -> 즉시 발동
 *  2) 이동 중 발판을 밟으면 남은 경로를 폐기하고 발판 방향으로 밀려남
 *  3) 정지 상태로 발판 위에 배치되면 즉시 밀려남
 */
bool FGimmickPlaceAndPushTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForGimmickTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	/* Case1: 배치 즉시 발동 */
	AddInfo(TEXT("=== Case1: 트랩 타일에 유닛 배치 -> 즉시 기절 ==="));
	{
		// 픽스처가 유닛을 트랩 타일 (2,2)에 바로 배치
		FGimmickFixture Fixture = MakeGimmickFixture(
			World,
			FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward),
			FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward),
			MakeStunSkillData(World),
			1);

		TestTrue(TEXT("[Case1] 배치 즉시 기절 부여"), HasStunTag(Fixture.Unit));
		TestEqual(TEXT("[Case1] 수명 차감"), Fixture.Gimmick->GetRemainingTriggerCount(), 0);
	}

	/* Case2: 이동 중 발판 -> 경로 교체 */
	AddInfo(TEXT("=== Case2: 걷다가 발판 -> 잔여 경로 폐기, 발판 방향(+Y)으로 2칸 밀림 ==="));
	{
		// (3,2)의 발판이 Right(+Y)를 바라봄. 유닛은 (2,2)->(4,2)로 걷는 중
		FGimmickFixture Fixture = MakeGimmickFixture(
			World,
			FTileTransform(FTileIndex(3, 2), ETileActorDirection::Right),
			FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward),
			MakePushSkillData(World, 2),
			1);

		Fixture.Movement->MoveAlongPath({ FTileIndex(2, 2), FTileIndex(3, 2), FTileIndex(4, 2) });

		TestTrue(TEXT("[Case2] 발판 방향으로 2칸 밀린 위치 도착"), Fixture.Unit->GetTileTransform().mIndex == FTileIndex(3, 4));
		TestTrue(TEXT("[Case2] 밀리는 동안 바라보는 방향 유지"), Fixture.Unit->GetTileTransform().mDirection == ETileActorDirection::Forward);
		TestEqual(TEXT("[Case2] 발판 수명 차감"), Fixture.Gimmick->GetRemainingTriggerCount(), 0);
	}

	/* Case3: 정지 상태 배치 -> 즉시 밀기 */
	AddInfo(TEXT("=== Case3: 발판 위 배치 -> 즉시 +Y로 2칸 밀림 ==="));
	{
		FGimmickFixture Fixture = MakeGimmickFixture(
			World,
			FTileTransform(FTileIndex(5, 2), ETileActorDirection::Right),
			FTileTransform(FTileIndex(5, 2), ETileActorDirection::Forward),
			MakePushSkillData(World, 2),
			1);

		TestTrue(TEXT("[Case3] 배치 즉시 밀린 위치 도착"), Fixture.Unit->GetTileTransform().mIndex == FTileIndex(5, 4));
		TestEqual(TEXT("[Case3] 발판 수명 차감"), Fixture.Gimmick->GetRemainingTriggerCount(), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPuddleRoundEndTests,
	"P_RD.SRPG.Gimmick.PuddleRoundEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief 장판의 라운드 끝 발동과 라운드 수명 검증
 *  1) 진입 시 발동 + 라운드 끝 발동 (한 라운드 2회 허용)
 *  2) 위에 아무도 없으면 라운드 끝에 발동하지 않되 수명은 차감
 *  3) 마지막 라운드에도 발동한 뒤 사망 태그
 *  4) 무제한 수명(-1)은 라운드가 지나도 유지
 */
bool FPuddleRoundEndTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForGimmickTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	/* Case1: 진입 + 라운드 끝 = 2회 */
	AddInfo(TEXT("=== Case1: 장판 진입 -> 기절 1, 라운드 끝 -> 기절 2, 수명 2 -> 1 ==="));

	FPuddleFixture Fixture = MakePuddleFixture(
		World,
		FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward),
		FTileTransform(FTileIndex(1, 2), ETileActorDirection::Forward),
		MakeStunSkillData(World),
		2);

	Fixture.Movement->MoveAlongPath({ FTileIndex(1, 2), FTileIndex(2, 2) });
	TestEqual(TEXT("[Case1] 진입 시 기절 1"), GetStunCount(Fixture.Unit), 1);

	Fixture.Puddle->TriggerRoundEnd(nullptr);
	TestEqual(TEXT("[Case1] 라운드 끝 발동으로 기절 2"), GetStunCount(Fixture.Unit), 2);
	TestEqual(TEXT("[Case1] 수명 2 -> 1"), Fixture.Puddle->GetRemainingRoundCount(), 1);
	TestFalse(TEXT("[Case1] 아직 생존"), Fixture.Puddle->IsDead());

	/* Case2: 빈 장판은 미발동, 수명만 차감 */
	AddInfo(TEXT("=== Case2: 유닛 이탈 후 라운드 끝 -> 미발동, 수명 1 -> 0, 사망 ==="));

	Fixture.Movement->MoveAlongPath({ FTileIndex(2, 2), FTileIndex(3, 2) });
	Fixture.Puddle->TriggerRoundEnd(nullptr);

	TestEqual(TEXT("[Case2] 기절 2 유지 (미발동)"), GetStunCount(Fixture.Unit), 2);
	TestEqual(TEXT("[Case2] 수명 1 -> 0"), Fixture.Puddle->GetRemainingRoundCount(), 0);
	TestTrue(TEXT("[Case2] 수명 소진으로 사망 태그"), Fixture.Puddle->IsDead());

	/* Case3: 마지막 라운드에도 발동 */
	AddInfo(TEXT("=== Case3: 수명 1 장판 위 유닛, 라운드 끝 -> 발동 후 사망 ==="));
	{
		FPuddleFixture LastFixture = MakePuddleFixture(
			World,
			FTileTransform(FTileIndex(4, 4), ETileActorDirection::Forward),
			FTileTransform(FTileIndex(4, 4), ETileActorDirection::Forward),
			MakeStunSkillData(World),
			1);

		LastFixture.Puddle->TriggerRoundEnd(nullptr);

		TestEqual(TEXT("[Case3] 배치 1 + 라운드 끝 1 = 기절 2"), GetStunCount(LastFixture.Unit), 2);
		TestTrue(TEXT("[Case3] 발동 후 사망 태그"), LastFixture.Puddle->IsDead());
	}

	/* Case4: 무제한 수명 */
	AddInfo(TEXT("=== Case4: 수명 -1 장판은 라운드가 지나도 유지 ==="));
	{
		FPuddleFixture InfFixture = MakePuddleFixture(
			World,
			FTileTransform(FTileIndex(6, 6), ETileActorDirection::Forward),
			FTileTransform(FTileIndex(6, 6), ETileActorDirection::Forward),
			MakeStunSkillData(World),
			-1);

		InfFixture.Puddle->TriggerRoundEnd(nullptr);
		InfFixture.Puddle->TriggerRoundEnd(nullptr);

		TestEqual(TEXT("[Case4] 배치 1 + 라운드 끝 2 = 기절 3"), GetStunCount(InfFixture.Unit), 3);
		TestEqual(TEXT("[Case4] 무제한 수명 유지"), InfFixture.Puddle->GetRemainingRoundCount(), -1);
		TestFalse(TEXT("[Case4] 사망 태그 없음"), InfFixture.Puddle->IsDead());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPuddleReplaceTests,
	"P_RD.SRPG.Gimmick.PuddleReplace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief 장판 배치와 겹침 교체 검증
 *  1) 유닛이 서 있는 타일에 장판을 깔면 즉시 발동 (장판통이 터져 유닛 발밑에 깔리는 상황)
 *  2) 같은 타일에 새 장판이 오면 기존 장판은 타일에서 빠지고 사망 태그, 새 장판만 남음
 *  3) 교체된 장판은 라운드 끝에 발동하지 않음
 */
bool FPuddleReplaceTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForGimmickTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	/* Case1: 유닛 위에 장판 배치 */
	AddInfo(TEXT("=== Case1: (2,2) 유닛 위에 장판 배치 -> 즉시 기절 ==="));

	// 유닛을 먼저 세우고, 그 타일에 장판을 나중에 깖
	UTileMapModel* TileMap = NewObject<UTileMapModel>(World);
	TileMap->SetDimensions(8, 8);
	UMockUnitMovementComponentModel* Movement = nullptr;
	UMockGimmickVictimUnitModel* Unit = MakeVictimUnit(World, TileMap, FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward), Movement);

	const FTileTransform PuddleTransform(FTileIndex(2, 2), ETileActorDirection::Forward);
	UMockPuddleGimmickModel* First = MakePuddle(World, TileMap, PuddleTransform, MakeStunSkillData(World), 3);
	TestEqual(TEXT("[Case1] 장판이 깔리자마자 기절 1"), GetStunCount(Unit), 1);

	/* Case2: 새 장판이 기존 장판을 덮어씀 */
	AddInfo(TEXT("=== Case2: 같은 타일에 두 번째 장판 -> 첫 장판 이탈+사망, 새 장판만 잔류 ==="));

	UMockPuddleGimmickModel* Second = MakePuddle(World, TileMap, PuddleTransform, MakeStunSkillData(World), 3);

	const TArray<UPuddleGimmickModel*> Puddles = TileMap->GetActorsOnTile<UPuddleGimmickModel>(PuddleTransform.mIndex, ETileLayerFlag::Overlay);
	TestEqual(TEXT("[Case2] 타일 위 장판은 1개"), Puddles.Num(), 1);
	TestTrue(TEXT("[Case2] 남은 장판은 새 장판"), Puddles.Num() == 1 && Puddles[0] == Second);
	TestTrue(TEXT("[Case2] 첫 장판 좌표 무효화"), First->GetTileTransform().mIndex == FTileIndex::Invalid);
	TestTrue(TEXT("[Case2] 첫 장판 사망 태그"), First->IsDead());
	TestFalse(TEXT("[Case2] 새 장판 생존"), Second->IsDead());
	TestEqual(TEXT("[Case2] 새 장판 진입으로 기절 2"), GetStunCount(Unit), 2);

	/* Case3: 교체된 장판은 라운드 끝에 미발동 */
	AddInfo(TEXT("=== Case3: 라운드 끝 -> 첫 장판 침묵, 새 장판만 발동 ==="));

	First->TriggerRoundEnd(nullptr);
	TestEqual(TEXT("[Case3] 교체된 장판은 미발동 (기절 2 유지)"), GetStunCount(Unit), 2);
	TestEqual(TEXT("[Case3] 교체된 장판은 수명도 그대로"), First->GetRemainingRoundCount(), 3);

	Second->TriggerRoundEnd(nullptr);
	TestEqual(TEXT("[Case3] 새 장판 발동으로 기절 3"), GetStunCount(Unit), 3);
	TestEqual(TEXT("[Case3] 새 장판 수명 3 -> 2"), Second->GetRemainingRoundCount(), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPuddleRoundEndEventTests,
	"P_RD.SRPG.Gimmick.PuddleRoundEndEvent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief 장판 일괄 발동 이벤트 검증
 *  1) 같은 타입 이벤트는 여러 번 등록해도 1개만 남음
 *  2) 라운드 끝 이벤트 한 번에 보드 위 모든 장판이 발동 (유닛 있는 장판만 효과, 수명은 전부 차감)
 *  3) 반복 이벤트라 다음 라운드 끝에도 다시 발동
 */
bool FPuddleRoundEndEventTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForGimmickTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	UTileMapModel* TileMap = NewObject<UTileMapModel>(World);
	TileMap->SetDimensions(8, 8);

	UMockGimmickCombatModel* CombatModel = NewObject<UMockGimmickCombatModel>(World);
	CombatModel->SetTileMap(TileMap);

	/* Case1: 중복 등록 방지 */
	AddInfo(TEXT("=== Case1: 일괄 발동 이벤트 등록 시도 2회 -> 1개 ==="));

	// 장판 2개가 스폰되며 각각 등록을 시도하는 상황
	FPuddleRoundEndEvent::Register(CombatModel);
	FPuddleRoundEndEvent::Register(CombatModel);
	TestEqual(TEXT("[Case1] 등록된 라운드 끝 이벤트 1개"), CombatModel->GetRoundEndEventCount(), 1);
	TestNotNull(TEXT("[Case1] 이름으로 조회 가능"), CombatModel->FindRoundEndEvent(FPuddleRoundEndEvent::EventName));

	/* Case2: 보드 위 모든 장판 일괄 발동 */
	AddInfo(TEXT("=== Case2: 장판 3개(유닛 2 + 빈 1), 이벤트 1회 -> 유닛 기절 +1, 수명 전부 -1 ==="));

	UMockPuddleGimmickModel* PuddleA = MakePuddle(World, TileMap, FTileTransform(FTileIndex(1, 1), ETileActorDirection::Forward), MakeStunSkillData(World), 3);
	UMockPuddleGimmickModel* PuddleB = MakePuddle(World, TileMap, FTileTransform(FTileIndex(5, 5), ETileActorDirection::Forward), MakeStunSkillData(World), 3);
	UMockPuddleGimmickModel* PuddleC = MakePuddle(World, TileMap, FTileTransform(FTileIndex(3, 3), ETileActorDirection::Forward), MakeStunSkillData(World), 3);

	UMockUnitMovementComponentModel* MovementA = nullptr;
	UMockUnitMovementComponentModel* MovementB = nullptr;
	UMockGimmickVictimUnitModel* UnitA = MakeVictimUnit(World, TileMap, FTileTransform(FTileIndex(1, 1), ETileActorDirection::Forward), MovementA);
	UMockGimmickVictimUnitModel* UnitB = MakeVictimUnit(World, TileMap, FTileTransform(FTileIndex(5, 5), ETileActorDirection::Forward), MovementB);

	CombatModel->TriggerAllRoundEndEvents();

	TestEqual(TEXT("[Case2] 유닛A 기절 2 (배치 1 + 라운드 끝 1)"), GetStunCount(UnitA), 2);
	TestEqual(TEXT("[Case2] 유닛B 기절 2"), GetStunCount(UnitB), 2);
	TestEqual(TEXT("[Case2] 장판A 수명 3 -> 2"), PuddleA->GetRemainingRoundCount(), 2);
	TestEqual(TEXT("[Case2] 장판B 수명 3 -> 2"), PuddleB->GetRemainingRoundCount(), 2);
	TestEqual(TEXT("[Case2] 빈 장판C도 수명 3 -> 2"), PuddleC->GetRemainingRoundCount(), 2);

	/* Case3: 반복 발동 */
	AddInfo(TEXT("=== Case3: 이벤트 한 번 더 -> 다시 발동 ==="));

	CombatModel->TriggerAllRoundEndEvents();

	TestEqual(TEXT("[Case3] 이벤트가 제거되지 않고 유지"), CombatModel->GetRoundEndEventCount(), 1);
	TestEqual(TEXT("[Case3] 유닛A 기절 3"), GetStunCount(UnitA), 3);
	TestEqual(TEXT("[Case3] 장판C 수명 2 -> 1"), PuddleC->GetRemainingRoundCount(), 1);

	return true;
}
