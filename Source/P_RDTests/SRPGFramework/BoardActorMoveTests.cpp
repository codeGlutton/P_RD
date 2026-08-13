/*****************************************************************//**
 * @file   BoardActorMoveTests.cpp
 * @brief  보드액터 이동(방향 산출·스텝 배리어) 유닛테스트
 * @details
 *  이동 연출 페이싱의 핵심 계약 검증:
 *   1) UTileMapModel::TileDeltaToDirection 방향 산출
 *   2) OnStartMoveStep + FPresentationBarrier 게이팅
 *  USRPGMoveAction 수명주기 전체는 월드 서브시스템 의존이라 PIE 검증으로 대체.
 * @author 이문환
 * @date   2026-07-03
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "SRPGFramework/EnemyTurnPlannerTestsHelper.h"   // UMockPlayerUnitModel

#include "Actor/TileMap/TileMapModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"

#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	// @brief 월드 목
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileDeltaToDirectionTests,
	"P_RD.SRPG.BoardActorMove.TileDeltaToDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/*
 * @brief From 타일에서 To타일을 바라볼 때의 Direction 계산이 제대로 되는 지 검증
 */
bool FTileDeltaToDirectionTests::RunTest(const FString& Parameters)
{
	// 4방향 단위 델타
	TestTrue(TEXT("+X는 Forward"), UTileMapModel::TileDeltaToDirection(FTileIndex(2, 2), FTileIndex(3, 2), ETileActorDirection::Left) == ETileActorDirection::Forward);
	TestTrue(TEXT("-X는 Backward"), UTileMapModel::TileDeltaToDirection(FTileIndex(2, 2), FTileIndex(1, 2), ETileActorDirection::Left) == ETileActorDirection::Backward);
	TestTrue(TEXT("+Y는 Right"), UTileMapModel::TileDeltaToDirection(FTileIndex(2, 2), FTileIndex(2, 3), ETileActorDirection::Left) == ETileActorDirection::Right);
	TestTrue(TEXT("-Y는 Left"), UTileMapModel::TileDeltaToDirection(FTileIndex(2, 2), FTileIndex(2, 1), ETileActorDirection::Forward) == ETileActorDirection::Left);

	// 제자리
	TestTrue(TEXT("제자리는 폴백 유지"), UTileMapModel::TileDeltaToDirection(FTileIndex(2, 2), FTileIndex(2, 2), ETileActorDirection::Right) == ETileActorDirection::Right);

	// 여러 칸 + 대각선 델타 -> 델타가 큰 우세측으로 결정되는 지 확인 
	TestTrue(TEXT("우세 축 X(+3,+1)는 Forward"), UTileMapModel::TileDeltaToDirection(FTileIndex(0, 0), FTileIndex(3, 1), ETileActorDirection::Left) == ETileActorDirection::Forward);
	TestTrue(TEXT("우세 축 Y(-1,-4)는 Left"), UTileMapModel::TileDeltaToDirection(FTileIndex(0, 0), FTileIndex(-1, -4), ETileActorDirection::Forward) == ETileActorDirection::Left);
	TestTrue(TEXT("같은 크기(+2,+2)는 X 우선 Forward"), UTileMapModel::TileDeltaToDirection(FTileIndex(0, 0), FTileIndex(2, 2), ETileActorDirection::Left) == ETileActorDirection::Forward);
	TestTrue(TEXT("같은 크기(-2,+2)는 X 우선 Backward"), UTileMapModel::TileDeltaToDirection(FTileIndex(0, 0), FTileIndex(-2, 2), ETileActorDirection::Left) == ETileActorDirection::Backward);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBoardActorMoveStepBarrierTests,
	"P_RD.SRPG.BoardActorMove.StepBarrier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief OnStartMoveStep 콜백이 PresentationBarrier
 * 있을 때(라이브모드)와
 * 없을 때(시뮬레이션모드) 정상적으로 동작하는 지 확인
 */
bool FBoardActorMoveStepBarrierTests::RunTest(const FString& Parameters)
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

	// OnStartMoveStep() 구독자를 없게 해서 시뮬레이션모드 테스트
	AddInfo(TEXT("=== Case1: 구독자 없음 -> 시뮬레이션모드 -> 즉시 완료 ==="));
	{
		UMockPlayerUnitModel* Unit = NewObject<UMockPlayerUnitModel>(World);

		bool bFinished = false;
		{
			TSharedPtr<FPresentationBarrier> Barrier = FPresentationBarrier::Make(
				FOnFinishPresentation::CreateLambda([&bFinished]()
				{
					bFinished = true;
				}));
			Unit->OnStartMoveStep.Broadcast(FTileTransform(), FTransform::Identity, Barrier, 0.0f, EBoardMoveMode::Normal);

			TestFalse(TEXT("배리어 참조가 남아있으면 미완료"), bFinished);
		}
		TestTrue(TEXT("스코프를 벗어나서 베리어 참조가 없으면 참조 소멸 시 즉시 완료"), bFinished);
	}

	// OnStartMoveStep 구독자를 만들어서 라이브모드 테스트
	AddInfo(TEXT("=== Case2: 구독자 있음 -> 라이브모드 -> 구독자가 놓으면 완료 ==="));
	{
		UMockPlayerUnitModel* Unit = NewObject<UMockPlayerUnitModel>(World);

		// 뷰 역할의 구독자 (Barrier를 보관하고 있어서 참조가 0이 안 되게 유지)
		TSharedPtr<FPresentationBarrier> HeldBarrier;
		Unit->OnStartMoveStep.AddLambda(
			[&HeldBarrier](const FTileTransform&, const FTransform&, TSharedPtr<FPresentationBarrier> Barrier, float, EBoardMoveMode) {
				HeldBarrier = Barrier;
			});

		bool bFinished = false;
		{
			TSharedPtr<FPresentationBarrier> Barrier = FPresentationBarrier::Make(
				FOnFinishPresentation::CreateLambda([&bFinished]()
				{
					bFinished = true;
				}));
			Unit->OnStartMoveStep.Broadcast(FTileTransform(), FTransform::Identity, Barrier, 0.0f, EBoardMoveMode::Normal);
		}

		TestTrue(TEXT("구독자가 배리어 수신"), HeldBarrier.IsValid());
		TestFalse(TEXT("발행자는 사라졌지만, 구독자가 잡고있는 동안은 미완료"), bFinished);

		// 구독자도 베리어를 놓음
		HeldBarrier.Reset();
		TestTrue(TEXT("구독자도 베리어 놓으면 콜백 호출되면서 완료로 변경"), bFinished);
	}

	return true;
}
