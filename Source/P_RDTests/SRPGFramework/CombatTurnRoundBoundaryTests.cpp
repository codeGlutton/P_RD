#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "SRPGFramework/EnemyTurnPlannerTestsHelper.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatTurnRoundBoundaryTest,
	"P_RD.SRPGFramework.CombatTurn.RoundBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatTurnRoundBoundaryTest::RunTest(const FString& Parameters)
{
	USRPGCombatModel* CombatModel = NewObject<USRPGCombatModel>();
	if (!TestNotNull(TEXT("전투 모델"), CombatModel))
	{
		return false;
	}

	TestEqual(TEXT("턴이 없으면 잔여 턴도 없다"),
		CombatModel->GetRemainingTurnCountInRound(), 0);

	TArray<TObjectPtr<UMockPlayerUnitModel>> Units;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UMockPlayerUnitModel* Unit = NewObject<UMockPlayerUnitModel>(CombatModel);
		Units.Add(Unit);
		TestNotNull(*FString::Printf(TEXT("테스트 유닛 %d"), Index), Unit);
		TestNotNull(*FString::Printf(TEXT("턴 컨텍스트 %d"), Index),
			CombatModel->RegisterTurn(Unit));
	}

	const TArray<TObjectPtr<USRPGTurnContext>> Ordered =
		CombatModel->GetOrderedTurnContexts();
	TestEqual(TEXT("등록한 턴 수"), Ordered.Num(), 3);
	TestEqual(TEXT("라운드 시작에서는 전체 턴이 이번 라운드에 남는다"),
		CombatModel->GetRemainingTurnCountInRound(), Ordered.Num());

	return true;
}
