#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatTurnRoundBoundaryTest,
	"P_RD.SRPGFramework.CombatTurn.RoundBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatTurnRoundBoundaryTest::RunTest(const FString& Parameters)
{
	TArray<FSRPGTurnCandidate> CurrentCandidates;
	FSRPGTurnCandidate& Slow = CurrentCandidates.AddDefaulted_GetRef();
	Slow.mRemainSpeedPoint = 14;
	Slow.mRechargedSpeedPoint = 5;
	FSRPGTurnCandidate& Fast = CurrentCandidates.AddDefaulted_GetRef();
	Fast.mRemainSpeedPoint = 9;
	Fast.mRechargedSpeedPoint = 15;

	const TArray<FSRPGPredictedRound> Predicted =
		USRPGCombatModel::PredictTurnRounds(CurrentCandidates,
			/*RequiredSpeedPointForTurn=*/10,
			/*InitialRandomSeed=*/1234,
			/*RoundCount=*/10);

	TestEqual(TEXT("요청한 미래 유효 라운드 수"), Predicted.Num(), 10);
	if (Predicted.Num() != 10)
	{
		return false;
	}

	TestEqual(TEXT("첫 미래 라운드 오프셋"), Predicted[0].mRoundOffset, 1);
	TestEqual(TEXT("첫 미래 라운드 턴 수"), Predicted[0].mCandidates.Num(), 2);
	if (Predicted[0].mCandidates.Num() == 2)
	{
		// 실제 진행과 같이 다음 라운드 충전을 먼저 적용한다.
		// 14+5-10=9, 9+15-10=14 이므로 빠른 유닛이 먼저다.
		TestEqual(TEXT("충전 후 첫 후보 잔여 속도"),
			Predicted[0].mCandidates[0].mRemainSpeedPoint, 14);
		TestEqual(TEXT("충전 후 둘째 후보 잔여 속도"),
			Predicted[0].mCandidates[1].mRemainSpeedPoint, 9);
	}
	TestEqual(TEXT("열 번째 미래 라운드 오프셋"),
		Predicted.Last().mRoundOffset, 10);
	TestEqual(TEXT("예측은 입력 스냅샷을 변경하지 않음"),
		CurrentCandidates[0].mRemainSpeedPoint, 14);

	TArray<FSRPGTurnCandidate> SparseCandidates;
	FSRPGTurnCandidate& Sparse = SparseCandidates.AddDefaulted_GetRef();
	Sparse.mRemainSpeedPoint = 0;
	Sparse.mRechargedSpeedPoint = 5;
	const TArray<FSRPGPredictedRound> SparsePredicted =
		USRPGCombatModel::PredictTurnRounds(SparseCandidates, 10, 1, 10);
	TestEqual(TEXT("빈 충전 주기는 건너뛰고 열 유효 라운드 계산"),
		SparsePredicted.Num(), 10);
	for (const FSRPGPredictedRound& Round : SparsePredicted)
	{
		TestEqual(TEXT("희소 라운드에는 턴 하나"), Round.mCandidates.Num(), 1);
	}

	SparseCandidates[0].mRechargedSpeedPoint = 0;
	TestTrue(TEXT("속도를 영구히 얻지 못하면 예측을 안전하게 중단"),
		USRPGCombatModel::PredictTurnRounds(SparseCandidates, 10, 1, 10).IsEmpty());

	return true;
}
