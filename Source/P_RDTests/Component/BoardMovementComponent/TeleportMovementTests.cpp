/*****************************************************************//**
 * @file   TeleportMovementTests.cpp
 * @brief  텔레포트 이동(TeleportTo) 유닛테스트
 * @details
 *  배리어 구독자가 없는 시뮬레이션모드에서는 동기로 완료되고,
 *  테스트가 OnTeleport를 구독해 배리어를 잡으면 연출 대기 상태를 검증할 수 있음
 * @author 이문환
 * @date   2026-09-16
 *********************************************************************/

#include "P_RDTests.h"
#include "TestObjectScope.h"
#include "Misc/AutomationTest.h"

#include "SRPGFramework/EnemyTurnPlannerTestsHelper.h"                // UMockEnemyUnitModel
#include "Component/BoardMovementComponent/BoardMovementTestsHelper.h" // UMockUnitMovementComponentModel, UMockOverlapSensorModel

#include "Actor/TileMap/TileMapModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"

#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	// @brief 테스트용 월드 획득 (PIE/Game 우선)
	UWorld* GetAnyGameWorldForTeleportTests()
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

	// @brief 테스트 공용 픽스처 (타일맵 + 유닛 + 타일맵 주입 컴포넌트)
	struct FTeleportFixture
	{
		UTileMapModel* TileMap = nullptr;
		UMockEnemyUnitModel* Unit = nullptr;
		UMockUnitMovementComponentModel* Movement = nullptr;
	};

	// @brief 8x8 타일맵에 유닛을 배치하고 이동 컴포넌트를 연결 (생성 객체는 스코프에 등록)
	FTeleportFixture MakeTeleportFixture(UWorld* World, FTestObjectScope& Scope, const FTileTransform& StartTransform)
	{
		FTeleportFixture Fixture;

		Fixture.TileMap = Scope.New<UTileMapModel>(World);
		Fixture.TileMap->SetDimensions(8, 8);

		Fixture.Unit = Scope.New<UMockEnemyUnitModel>(World);
		Fixture.Unit->Initialize();
		Fixture.Unit->BeginPlay();

		// 유닛을 Outer로 만들어서 GetOwnerModel()(Outer 체인 탐색)이 유닛을 찾게 함
		Fixture.Movement = Scope.New<UMockUnitMovementComponentModel>(Fixture.Unit);
		Fixture.Movement->SetTileMap(Fixture.TileMap);

		Fixture.TileMap->PlaceActor(StartTransform, Fixture.Unit);
		return Fixture;
	}

	// @brief 오버랩 관측 Mock을 타일에 배치 (스코프에 등록)
	UMockOverlapSensorModel* PlaceSensor(const FTeleportFixture& Fixture, FTestObjectScope& Scope, const FTileIndex& TileIndex)
	{
		UMockOverlapSensorModel* Sensor = Scope.New<UMockOverlapSensorModel>(Fixture.TileMap);
		Fixture.TileMap->PlaceActor(FTileTransform(TileIndex, ETileActorDirection::Forward), Sensor);
		return Sensor;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTeleportMovementBasicTests,
	"P_RD.SRPG.TeleportMovement.Basic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief TeleportTo 기본 동작 검증 (구독자 없음 = 동기 완료)
 *  1) 목표 타일 도착, 지정한 방향 유지, OnFinished 1회, 이동 종료 상태
 *  2) 이동 AP 불변
 *  3) 출발 타일 이탈 통지 1회, 도착 타일 진입 통지 1회
 */
bool FTeleportMovementBasicTests::RunTest(const FString& Parameters)
{
	FTestObjectScope Scope;

	UWorld* World = GetAnyGameWorldForTeleportTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	FTeleportFixture Fixture = MakeTeleportFixture(World, Scope, FTileTransform(FTileIndex(2, 2), ETileActorDirection::Left));
	UMockOverlapSensorModel* StartSensor = PlaceSensor(Fixture, Scope, FTileIndex(2, 2));
	UMockOverlapSensorModel* EndSensor = PlaceSensor(Fixture, Scope, FTileIndex(5, 5));

	UAttributeSetComponentModel* AttrComp = Fixture.Unit->GetAttributeComponentModel();
	if (TestNotNull(TEXT("속성 컴포넌트"), AttrComp) == false)
	{
		return false;
	}
	const float BeforeAP = AttrComp->GetAttributeCurrentValue(UUnitAttributeSet::GetActionPointAttribute());

	// 센서 배치로 생긴 진입 통지는 텔레포트와 무관하므로 기준값으로 기록
	const int32 StartBeginBase = StartSensor->mBeginCount;
	const int32 EndBeginBase = EndSensor->mBeginCount;

	int32 FinishCount = 0;
	const bool Started = Fixture.Movement->TeleportTo(
		FTileTransform(FTileIndex(5, 5), ETileActorDirection::Left),
		FOnBoardMoveFinished::CreateLambda([&FinishCount]()
		{
			++FinishCount;
		}));

	TestTrue(TEXT("시작 성공"), Started);
	TestTrue(TEXT("목표 타일 도착"), Fixture.Unit->GetTileTransform().mIndex == FTileIndex(5, 5));
	TestTrue(TEXT("지정한 방향 유지 (Left)"), Fixture.Unit->GetTileTransform().mDirection == ETileActorDirection::Left);
	TestEqual(TEXT("OnFinished 1회"), FinishCount, 1);
	TestFalse(TEXT("이동 종료 상태"), Fixture.Movement->IsMoving());

	const float AfterAP = AttrComp->GetAttributeCurrentValue(UUnitAttributeSet::GetActionPointAttribute());
	TestEqual(TEXT("이동 AP 불변"), AfterAP, BeforeAP);

	TestEqual(TEXT("출발 타일 이탈 통지 1회"), StartSensor->mEndCount, 1);
	TestEqual(TEXT("도착 타일 진입 통지 1회"), EndSensor->mBeginCount - EndBeginBase, 1);
	TestEqual(TEXT("출발 타일에 추가 진입 통지 없음"), StartSensor->mBeginCount - StartBeginBase, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTeleportMovementTrapTests,
	"P_RD.SRPG.TeleportMovement.Trap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief 도착 타일 밀치기 함정 연계 검증
 *  도착 진입 통지 시점에 이동 종료 상태여야 함정이 PushAlongPath로 즉시 밀 수 있음
 *  (이동 중 상태면 보류 등록으로 빠져 밀리지 않음)
 */
bool FTeleportMovementTrapTests::RunTest(const FString& Parameters)
{
	FTestObjectScope Scope;

	UWorld* World = GetAnyGameWorldForTeleportTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	FTeleportFixture Fixture = MakeTeleportFixture(World, Scope, FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward));

	// (5,5) 함정: 진입 통지에서 +Y로 2칸 밀치기 시도 (밀치기 이펙트의 정지 대상 경로 흉내)
	UMockOverlapSensorModel* TrapSensor = PlaceSensor(Fixture, Scope, FTileIndex(5, 5));
	bool MovingAtOverlap = true;
	bool PushStarted = false;
	TrapSensor->mOnBeginOverlap.BindLambda([&]()
	{
		MovingAtOverlap = Fixture.Movement->IsMoving();
		PushStarted = Fixture.Movement->PushAlongPath({ FTileIndex(5, 5), FTileIndex(5, 6), FTileIndex(5, 7) });
	});

	int32 FinishCount = 0;
	Fixture.Movement->TeleportTo(
		FTileTransform(FTileIndex(5, 5), ETileActorDirection::Forward),
		FOnBoardMoveFinished::CreateLambda([&FinishCount]()
		{
			++FinishCount;
		}));

	TestFalse(TEXT("진입 통지 시점에 이동 종료 상태"), MovingAtOverlap);
	TestTrue(TEXT("함정 밀치기 즉시 시작"), PushStarted);
	TestTrue(TEXT("밀치기 경로 끝에 도착"), Fixture.Unit->GetTileTransform().mIndex == FTileIndex(5, 7));
	TestEqual(TEXT("텔레포트 완료 통지 1회"), FinishCount, 1);
	TestFalse(TEXT("최종 이동 종료 상태"), Fixture.Movement->IsMoving());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTeleportMovementGuardTests,
	"P_RD.SRPG.TeleportMovement.Guard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief 시작 거부 가드 검증
 *  1) 도착 타일이 다른 유닛에게 막힘 -> 거부, 제자리, 완료 통지 없음
 *  2) 걷는 중 -> 거부. 걷기가 끝나면 허용
 */
bool FTeleportMovementGuardTests::RunTest(const FString& Parameters)
{
	FTestObjectScope Scope;

	UWorld* World = GetAnyGameWorldForTeleportTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	/* Case1: 막힌 타일 */
	AddInfo(TEXT("=== Case1: 도착 타일 점유 -> 시작 거부, 제자리 ==="));
	{
		FTeleportFixture Fixture = MakeTeleportFixture(World, Scope, FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward));

		UMockEnemyUnitModel* Blocker = Scope.New<UMockEnemyUnitModel>(World);
		Blocker->Initialize();
		Blocker->BeginPlay();
		Fixture.TileMap->PlaceActor(FTileTransform(FTileIndex(5, 5), ETileActorDirection::Forward), Blocker);

		int32 FinishCount = 0;
		const bool Started = Fixture.Movement->TeleportTo(
			FTileTransform(FTileIndex(5, 5), ETileActorDirection::Forward),
			FOnBoardMoveFinished::CreateLambda([&FinishCount]()
			{
				++FinishCount;
			}));

		TestFalse(TEXT("[Case1] 시작 거부"), Started);
		TestTrue(TEXT("[Case1] 제자리 유지"), Fixture.Unit->GetTileTransform().mIndex == FTileIndex(2, 2));
		TestTrue(TEXT("[Case1] 점유 유닛 그대로"), Blocker->GetTileTransform().mIndex == FTileIndex(5, 5));
		TestEqual(TEXT("[Case1] 완료 통지 없음"), FinishCount, 0);
		TestFalse(TEXT("[Case1] 이동 종료 상태"), Fixture.Movement->IsMoving());
	}

	/* Case2: 걷는 중 */
	AddInfo(TEXT("=== Case2: 걷는 중 텔레포트 거부, 걷기 종료 후 허용 ==="));
	{
		FTeleportFixture Fixture = MakeTeleportFixture(World, Scope, FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward));

		// 뷰 흉내: 걷기 스텝 배리어를 잡아 걷는 중 상태를 유지
		TSharedPtr<FPresentationBarrier> HeldStepBarrier;
		FDelegateHandle StepHandle = Fixture.Unit->OnStartMoveStep.AddLambda(
			[&HeldStepBarrier](const FTileTransform&, const FTransform&, TSharedPtr<FPresentationBarrier> Barrier, float, EBoardMoveMode)
			{
				HeldStepBarrier = Barrier;
			});

		Fixture.Movement->MoveAlongPath({ FTileIndex(2, 2), FTileIndex(3, 2) });
		TestTrue(TEXT("[Case2] 걷는 중 상태"), Fixture.Movement->IsMoving());
		TestFalse(TEXT("[Case2] 걷는 중 텔레포트 거부"), Fixture.Movement->TeleportTo(FTileTransform(FTileIndex(5, 5), ETileActorDirection::Forward)));

		// 걷기 종료 후에는 허용 (걷기 완주는 배리어 해제로 동기 진행)
		Fixture.Unit->OnStartMoveStep.Remove(StepHandle);
		HeldStepBarrier.Reset();
		TestFalse(TEXT("[Case2] 걷기 종료 상태"), Fixture.Movement->IsMoving());
		TestTrue(TEXT("[Case2] 걷기 종료 후 텔레포트 허용"), Fixture.Movement->TeleportTo(FTileTransform(FTileIndex(5, 5), ETileActorDirection::Forward)));
		TestTrue(TEXT("[Case2] 목표 타일 도착"), Fixture.Unit->GetTileTransform().mIndex == FTileIndex(5, 5));
	}

	return true;
}
