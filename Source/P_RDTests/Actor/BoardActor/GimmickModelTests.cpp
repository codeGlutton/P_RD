/*****************************************************************//**
 * @file   GimmickModelTests.cpp
 * @brief  진입 트리거 기믹(발동/수명/필터/밀치기) 유닛테스트
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
		return Unit->GetAttributeComponentModel()->HasMatchingGameplayTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Stun);
	}

	// @brief 추가 피해자 유닛 생성/배치 (2번째 유닛이 필요한 케이스용)
	UMockGimmickVictimUnitModel* MakeVictimUnit(UWorld* World, FGimmickFixture& Fixture, const FTileTransform& UnitTransform, UMockUnitMovementComponentModel*& OutMovement)
	{
		UMockGimmickVictimUnitModel* Unit = NewObject<UMockGimmickVictimUnitModel>(World);
		Unit->Initialize();
		Unit->BeginPlay();
		OutMovement = NewObject<UMockUnitMovementComponentModel>(Unit);
		OutMovement->SetTileMap(Fixture.TileMap);
		Unit->SetBoardMovementComponentModel(OutMovement);
		Fixture.TileMap->PlaceActor(UnitTransform, Unit);
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
	UMockGimmickVictimUnitModel* SecondUnit = MakeVictimUnit(World, Fixture, FTileTransform(FTileIndex(1, 2), ETileActorDirection::Forward), SecondMovement);

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
	UMockGimmickVictimUnitModel* SecondUnit = MakeVictimUnit(World, Fixture, FTileTransform(FTileIndex(1, 2), ETileActorDirection::Forward), SecondMovement);
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
