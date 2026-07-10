/*****************************************************************//**
 * @file   BoardActorRotateTests.cpp
 * @brief  보드액터 방향 전환(RotateActor) 유닛테스트
 * @details
 *  방향 전환의 핵심 계약 검증:
 *   1) UTileMapModel::RotateActor 논리 방향 갱신
 *   2) 같은 방향 요청 시 OnRotate 미발행
 *   3) OnRotate + FPresentationBarrier 게이팅
 *  뷰(AUnit) 회전 보간은 틱 의존이라 PIE 검증으로 대체.
 * @author 이문환
 * @date   2026-07-06
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
	FBoardActorRotateTests,
	"P_RD.SRPG.BoardActorRotate.RotateActor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief RotateActor가 논리 방향을 갱신하고 OnRotate를 조건에 맞게 발행하는 지,
 * 배리어가 구독자 유/무에 따라 완료되는 지 확인
 */
bool FBoardActorRotateTests::RunTest(const FString& Parameters)
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

	// 타일맵과 유닛 준비
	UTileMapModel* TileMap = NewObject<UTileMapModel>(World);
	TileMap->SetDimensions(5, 5);

	UMockPlayerUnitModel* Unit = NewObject<UMockPlayerUnitModel>(World);
	TileMap->PlaceActor(FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward), Unit);

	// 뷰 역할의 구독자 (발행 횟수와 배리어 수신 기록)
	int32 RotateBroadcastCount = 0;
	TSharedPtr<FPresentationBarrier> HeldBarrier;
	Unit->OnRotate.AddLambda(
		[&RotateBroadcastCount, &HeldBarrier](const FRotator&, TSharedPtr<FPresentationBarrier> Barrier) {
			++RotateBroadcastCount;
			HeldBarrier = Barrier;
		});

	// Case1: 다른 방향으로 전환 -> 논리 방향 갱신 + OnRotate 발행
	AddInfo(TEXT("=== Case1: 다른 방향 -> 방향 갱신 + OnRotate 발행 ==="));
	{
		TileMap->RotateActor(ETileActorDirection::Right, Unit);

		TestTrue(TEXT("논리 방향이 Right로 갱신"), Unit->GetTileTransform().mDirection == ETileActorDirection::Right);
		TestTrue(TEXT("타일 인덱스는 그대로"), Unit->GetTileTransform().mIndex == FTileIndex(2, 2));
		TestTrue(TEXT("OnRotate 1회 발행"), RotateBroadcastCount == 1);
	}

	// Case2: 같은 방향으로 전환 -> 아무 일도 하지 않음
	AddInfo(TEXT("=== Case2: 같은 방향 -> OnRotate 미발행 ==="));
	{
		TileMap->RotateActor(ETileActorDirection::Right, Unit);

		TestTrue(TEXT("논리 방향 유지"), Unit->GetTileTransform().mDirection == ETileActorDirection::Right);
		TestTrue(TEXT("OnRotate 추가 발행 없음"), RotateBroadcastCount == 1);
	}

	// Case3: 배리어 게이팅 -> 구독자가 잡고 있는 동안 미완료, 놓으면 완료
	AddInfo(TEXT("=== Case3: 배리어 -> 구독자가 놓으면 완료 ==="));
	{
		HeldBarrier.Reset();

		bool bFinished = false;
		{
			TSharedPtr<FPresentationBarrier> Barrier = FPresentationBarrier::Make(
				FOnFinishPresentation::CreateLambda([&bFinished]()
				{
					bFinished = true;
				}));
			TileMap->RotateActor(ETileActorDirection::Left, Unit, Barrier);
		}

		TestTrue(TEXT("논리 방향이 Left로 갱신"), Unit->GetTileTransform().mDirection == ETileActorDirection::Left);
		TestTrue(TEXT("구독자가 배리어 수신"), HeldBarrier.IsValid());
		TestFalse(TEXT("발행자는 사라졌지만, 구독자가 잡고있는 동안은 미완료"), bFinished);

		// 구독자도 베리어를 놓음
		HeldBarrier.Reset();
		TestTrue(TEXT("구독자도 베리어 놓으면 콜백 호출되면서 완료로 변경"), bFinished);
	}

	return true;
}
