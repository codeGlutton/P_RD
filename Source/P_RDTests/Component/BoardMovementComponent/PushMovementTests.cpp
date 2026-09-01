/*****************************************************************//**
 * @file   PushMovementTests.cpp
 * @brief  밀치기 이동(PushAlongPath/보류 경로 연쇄) 유닛테스트
 * @details
 *  베리어 구독자가 없는 시뮬레이션모드에서는 이동 루프가 동기로 완주하므로,
 *  Mock 컴포넌트에 타일맵을 주입해서 이동/연쇄 로직을 자동화로 검증
 * @author 이문환
 * @date   2026-08-13
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "SRPGFramework/EnemyTurnPlannerTestsHelper.h"                // UMockEnemyUnitModel
#include "Component/BoardMovementComponent/BoardMovementTestsHelper.h" // UMockUnitMovementComponentModel

#include "Actor/TileMap/TileMapModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"

#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	// @brief 테스트용 월드 획득 (PIE/Game 우선)
	UWorld* GetAnyGameWorldForPushTests()
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
	struct FPushMovementFixture
	{
		UTileMapModel* TileMap = nullptr;
		UMockEnemyUnitModel* Unit = nullptr;
		UMockUnitMovementComponentModel* Movement = nullptr;
	};

	// @brief 8x8 타일맵에 유닛을 배치하고 이동 컴포넌트를 연결
	FPushMovementFixture MakePushMovementFixture(UWorld* World, const FTileTransform& StartTransform)
	{
		FPushMovementFixture Fixture;

		Fixture.TileMap = NewObject<UTileMapModel>(World);
		Fixture.TileMap->SetDimensions(8, 8);

		Fixture.Unit = NewObject<UMockEnemyUnitModel>(World);
		Fixture.Unit->Initialize();
		Fixture.Unit->BeginPlay();

		// 유닛을 Outer로 만들어서 GetOwnerModel()(Outer 체인 탐색)이 유닛을 찾게 함
		Fixture.Movement = NewObject<UMockUnitMovementComponentModel>(Fixture.Unit);
		Fixture.Movement->SetTileMap(Fixture.TileMap);

		Fixture.TileMap->PlaceActor(StartTransform, Fixture.Unit);
		return Fixture;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPushMovementBasicTests,
	"P_RD.SRPG.PushMovement.Basic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief PushAlongPath 기본 동작 검증
 *  1) 경로대로 도착, 바라보는 방향 불변, OnFinished 1회
 *  2) 일반 이동은 스텝당 AP 차감, 밀치기는 AP 불변
 */
bool FPushMovementBasicTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForPushTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	/* Case1: 밀치기 기본 (도착/방향/완료 통지) */
	AddInfo(TEXT("=== Case1: PushAlongPath -> 도착, 방향 불변, OnFinished 1회 ==="));
	{
		// (2,2)에서 Left를 바라보는 유닛을 +X로 2칸 밀침
		FPushMovementFixture Fixture = MakePushMovementFixture(World, FTileTransform(FTileIndex(2, 2), ETileActorDirection::Left));

		int32 FinishCount = 0;
		const bool Started = Fixture.Movement->PushAlongPath(
			{ FTileIndex(2, 2), FTileIndex(3, 2), FTileIndex(4, 2) },
			FOnBoardMoveFinished::CreateLambda([&FinishCount]()
			{
				++FinishCount;
			}));

		TestTrue(TEXT("[Case1] 시작 성공"), Started);
		TestTrue(TEXT("[Case1] 목표 타일 도착"), Fixture.Unit->GetTileTransform().mIndex == FTileIndex(4, 2));
		TestTrue(TEXT("[Case1] 바라보는 방향 불변 (Left 유지)"), Fixture.Unit->GetTileTransform().mDirection == ETileActorDirection::Left);
		TestEqual(TEXT("[Case1] OnFinished 1회"), FinishCount, 1);
		TestFalse(TEXT("[Case1] 이동 종료 상태"), Fixture.Movement->IsMoving());
		TestTrue(TEXT("[Case1] 마지막 모드는 Push"), Fixture.Movement->GetMoveMode() == EBoardMoveMode::Push);
	}

	/* Case2: AP 정산 (일반 이동 차감 vs 밀치기 불변) */
	AddInfo(TEXT("=== Case2: 일반 이동 AP 차감 vs 밀치기 AP 불변 ==="));
	{
		FPushMovementFixture Fixture = MakePushMovementFixture(World, FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward));

		UAttributeSetComponentModel* AttrComp = Fixture.Unit->GetAttributeComponentModel();
		if (TestNotNull(TEXT("[Case2] 속성 컴포넌트"), AttrComp) == false)
		{
			return false;
		}

		// 일반 이동 2스텝: AP 2 차감
		const float BeforeWalk = AttrComp->GetAttributeCurrentValue(UUnitAttributeSet::GetActionPointAttribute());
		Fixture.Movement->MoveAlongPath({ FTileIndex(2, 2), FTileIndex(3, 2), FTileIndex(4, 2) });
		const float AfterWalk = AttrComp->GetAttributeCurrentValue(UUnitAttributeSet::GetActionPointAttribute());
		TestEqual(TEXT("[Case2] 일반 이동 2스텝은 AP 2 차감"), AfterWalk, BeforeWalk - 2.0f);

		// 밀치기 2스텝: AP 불변
		Fixture.Movement->PushAlongPath({ FTileIndex(4, 2), FTileIndex(4, 3), FTileIndex(4, 4) });
		const float AfterPush = AttrComp->GetAttributeCurrentValue(UUnitAttributeSet::GetActionPointAttribute());
		TestEqual(TEXT("[Case2] 밀치기 2스텝은 AP 불변"), AfterPush, AfterWalk);
	}

	/* Case3: 당기기 (밀치기와 같은 규칙, 모드만 Pull) */
	AddInfo(TEXT("=== Case3: PullAlongPath -> 도착, 방향 불변, AP 불변, 마지막 모드 Pull ==="));
	{
		// (5,2)에서 Left를 바라보는 유닛을 -X로 2칸 당김
		FPushMovementFixture Fixture = MakePushMovementFixture(World, FTileTransform(FTileIndex(5, 2), ETileActorDirection::Left));

		UAttributeSetComponentModel* AttrComp = Fixture.Unit->GetAttributeComponentModel();
		const float BeforePull = AttrComp->GetAttributeCurrentValue(UUnitAttributeSet::GetActionPointAttribute());

		const bool Started = Fixture.Movement->PullAlongPath({ FTileIndex(5, 2), FTileIndex(4, 2), FTileIndex(3, 2) });
		const float AfterPull = AttrComp->GetAttributeCurrentValue(UUnitAttributeSet::GetActionPointAttribute());

		TestTrue(TEXT("[Case3] 시작 성공"), Started);
		TestTrue(TEXT("[Case3] 목표 타일 도착"), Fixture.Unit->GetTileTransform().mIndex == FTileIndex(3, 2));
		TestTrue(TEXT("[Case3] 바라보는 방향 불변 (Left 유지)"), Fixture.Unit->GetTileTransform().mDirection == ETileActorDirection::Left);
		TestEqual(TEXT("[Case3] 당기기 2스텝은 AP 불변"), AfterPull, BeforePull);
		TestTrue(TEXT("[Case3] 마지막 모드는 Pull"), Fixture.Movement->GetMoveMode() == EBoardMoveMode::Pull);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPushMovementChainTests,
	"P_RD.SRPG.PushMovement.Chain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief 보류 밀치기 등록/연쇄 검증
 *  1) 걷기 중 등록 -> 남은 걷기 경로 폐기, 밀치기 경로 완주, OnFinished 1회
 *  2) 같은 함정은 한 연쇄에서 1회만 발동
 *  3) 새 외부 요청 시작 시 연쇄 기록 초기화
 */
bool FPushMovementChainTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForPushTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	/* Case1: 걷기 중 함정 등록 -> 경로 교체 */
	AddInfo(TEXT("=== Case1: 걷기 중 등록 -> 잔여 걷기 폐기, 밀치기 경로 완주 ==="));
	{
		FPushMovementFixture Fixture = MakePushMovementFixture(World, FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward));

		// (3,2)에 밀치기 함정이 있는 상황: 도착 통지에서 +Y 2칸 밀치기 등록
		TArray<bool> RegisterResults;
		FDelegateHandle TrapHandle = Fixture.Unit->OnEndMoveStep.AddLambda(
			[&RegisterResults, &Fixture](const FTileTransform& TileTransform, const FTransform&)
			{
				if (TileTransform.mIndex == FTileIndex(3, 2))
				{
					RegisterResults.Add(Fixture.Movement->TryRegisterPendingPush(
						FTileIndex(3, 2), { FTileIndex(3, 2), FTileIndex(3, 3), FTileIndex(3, 4) }));
				}
			});

		int32 FinishCount = 0;
		Fixture.Movement->MoveAlongPath(
			{ FTileIndex(2, 2), FTileIndex(3, 2), FTileIndex(4, 2), FTileIndex(5, 2) },
			FOnBoardMoveFinished::CreateLambda([&FinishCount]()
			{
				++FinishCount;
			}));

		TestTrue(TEXT("[Case1] 함정 등록 1회 성공"), RegisterResults == TArray<bool>{ true });
		TestTrue(TEXT("[Case1] 걷기 잔여 경로 폐기, 밀치기 경로 끝에 도착"), Fixture.Unit->GetTileTransform().mIndex == FTileIndex(3, 4));
		TestEqual(TEXT("[Case1] 연쇄 전체에 OnFinished 1회"), FinishCount, 1);
		TestFalse(TEXT("[Case1] 이동 종료 상태"), Fixture.Movement->IsMoving());

		Fixture.Unit->OnEndMoveStep.Remove(TrapHandle);
	}

	/* Case2: 같은 함정 1회 제한 + 새 요청에서 기록 초기화 */
	AddInfo(TEXT("=== Case2: 같은 함정 재등록 거부, 새 요청이면 다시 허용 ==="));
	{
		FPushMovementFixture Fixture = MakePushMovementFixture(World, FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward));

		// 함정 키를 (3,2)로 고정하고 매 도착마다 등록 시도: 첫 발동만 성공해야 함
		TArray<bool> RegisterResults;
		FDelegateHandle TrapHandle = Fixture.Unit->OnEndMoveStep.AddLambda(
			[&RegisterResults, &Fixture](const FTileTransform& TileTransform, const FTransform&)
			{
				const FTileIndex Cur = TileTransform.mIndex;
				RegisterResults.Add(Fixture.Movement->TryRegisterPendingPush(
					FTileIndex(3, 2), { Cur, FTileIndex(Cur.mX, Cur.mY + 1) }));
			});

		// 걷기 1스텝 -> (3,2) 도착에서 등록 성공 -> 밀치기 (3,3) 도착에서 재등록 거부 -> 종료
		int32 FinishCount = 0;
		Fixture.Movement->MoveAlongPath(
			{ FTileIndex(2, 2), FTileIndex(3, 2) },
			FOnBoardMoveFinished::CreateLambda([&FinishCount]()
			{
				++FinishCount;
			}));

		TestTrue(TEXT("[Case2] 첫 등록만 성공, 재등록 거부"), RegisterResults == (TArray<bool>{ true, false }));
		TestTrue(TEXT("[Case2] 밀치기 1칸 후 종료"), Fixture.Unit->GetTileTransform().mIndex == FTileIndex(3, 3));
		TestEqual(TEXT("[Case2] OnFinished 1회"), FinishCount, 1);

		// 새 외부 요청: 연쇄 기록이 초기화되어 같은 함정 키가 다시 성공
		RegisterResults.Empty();
		Fixture.Movement->MoveAlongPath({ FTileIndex(3, 3), FTileIndex(3, 4) });

		TestTrue(TEXT("[Case2] 새 요청에서 같은 함정 다시 성공"), RegisterResults == (TArray<bool>{ true, false }));
		TestTrue(TEXT("[Case2] 새 연쇄도 밀치기 1칸 후 종료"), Fixture.Unit->GetTileTransform().mIndex == FTileIndex(3, 5));

		Fixture.Unit->OnEndMoveStep.Remove(TrapHandle);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPushMovementGuardTests,
	"P_RD.SRPG.PushMovement.Guard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief 등록/시작 거부 가드 검증
 *  1) 정지 상태 TryRegisterPendingPush -> false
 *  2) 경로 2칸 미만: PushAlongPath/TryRegisterPendingPush 모두 거부
 */
bool FPushMovementGuardTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForPushTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	FPushMovementFixture Fixture = MakePushMovementFixture(World, FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward));

	/* Case1: 정지 상태 등록 거부 (정지 대상은 PushAlongPath를 쓰라는 신호) */
	AddInfo(TEXT("=== Case1: 정지 상태 TryRegisterPendingPush -> false ==="));
	{
		const bool Registered = Fixture.Movement->TryRegisterPendingPush(
			FTileIndex(3, 2), { FTileIndex(2, 2), FTileIndex(2, 3) });
		TestFalse(TEXT("[Case1] 정지 상태 등록 거부"), Registered);
	}

	/* Case2: 경로 2칸 미만 거부 */
	AddInfo(TEXT("=== Case2: 경로 2칸 미만 -> false ==="));
	{
		// 시작 API: 빈 경로/한 칸 경로 거부 (타일맵 접근 전 가드)
		TestFalse(TEXT("[Case2] PushAlongPath 빈 경로 거부"), Fixture.Movement->PushAlongPath({}));
		TestFalse(TEXT("[Case2] PushAlongPath 한 칸 경로 거부"), Fixture.Movement->PushAlongPath({ FTileIndex(2, 2) }));

		// 등록 API: 이동 중이어야 경로 검사까지 도달하므로 걷기 도중에 한 칸 경로 등록 시도
		TArray<bool> RegisterResults;
		FDelegateHandle TrapHandle = Fixture.Unit->OnEndMoveStep.AddLambda(
			[&RegisterResults, &Fixture](const FTileTransform& TileTransform, const FTransform&)
			{
				RegisterResults.Add(Fixture.Movement->TryRegisterPendingPush(
					FTileIndex(3, 2), { TileTransform.mIndex }));
			});

		Fixture.Movement->MoveAlongPath({ FTileIndex(2, 2), FTileIndex(3, 2) });

		TestTrue(TEXT("[Case2] 이동 중이어도 한 칸 경로 등록 거부"), RegisterResults == TArray<bool>{ false });
		TestTrue(TEXT("[Case2] 등록 거부라서 걷기 경로 그대로 종료"), Fixture.Unit->GetTileTransform().mIndex == FTileIndex(3, 2));

		Fixture.Unit->OnEndMoveStep.Remove(TrapHandle);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPushMovementCancelTests,
	"P_RD.SRPG.PushMovement.Cancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief 취소와 함정의 우선순위 검증
 *  취소 요청된 스텝에서 함정 타일에 도착해도, 취소 검사가 도착 처리보다 먼저라서
 *  함정 미발동, 완료 통지 없이 정지 (기존 취소 의미 유지)
 */
bool FPushMovementCancelTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForPushTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	FPushMovementFixture Fixture = MakePushMovementFixture(World, FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward));

	// (3,2)로 향하는 스텝이 시작되면 취소 요청 (스텝 연출 종료 시점에 반영됨)
	FDelegateHandle CancelHandle = Fixture.Unit->OnStartMoveStep.AddLambda(
		[&Fixture](const FTileTransform& NextTileTransform, const FTransform&, TSharedPtr<FPresentationBarrier>, float, EBoardMoveMode)
		{
			if (NextTileTransform.mIndex == FTileIndex(3, 2))
			{
				Fixture.Movement->CancelMove();
			}
		});

	// (3,2)의 함정: 도착 통지가 오면 밀치기 등록 (취소가 먼저라서 통지 자체가 안 와야 함)
	TArray<bool> RegisterResults;
	FDelegateHandle TrapHandle = Fixture.Unit->OnEndMoveStep.AddLambda(
		[&RegisterResults, &Fixture](const FTileTransform& TileTransform, const FTransform&)
		{
			if (TileTransform.mIndex == FTileIndex(3, 2))
			{
				RegisterResults.Add(Fixture.Movement->TryRegisterPendingPush(
					FTileIndex(3, 2), { FTileIndex(3, 2), FTileIndex(3, 3) }));
			}
		});

	int32 FinishCount = 0;
	Fixture.Movement->MoveAlongPath(
		{ FTileIndex(2, 2), FTileIndex(3, 2), FTileIndex(4, 2) },
		FOnBoardMoveFinished::CreateLambda([&FinishCount]()
		{
			++FinishCount;
		}));

	// 점유는 이미 취소 스텝 타일로 옮겨졌으므로 (3,2)에서 정지
	TestTrue(TEXT("취소 스텝 타일에서 정지"), Fixture.Unit->GetTileTransform().mIndex == FTileIndex(3, 2));
	TestEqual(TEXT("함정 미발동 (도착 통지 없음)"), RegisterResults.Num(), 0);
	TestEqual(TEXT("완료 통지 없음"), FinishCount, 0);
	TestFalse(TEXT("이동 종료 상태"), Fixture.Movement->IsMoving());

	Fixture.Unit->OnStartMoveStep.Remove(CancelHandle);
	Fixture.Unit->OnEndMoveStep.Remove(TrapHandle);

	return true;
}
