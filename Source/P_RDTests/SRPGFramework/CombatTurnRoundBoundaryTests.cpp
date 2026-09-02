#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "Math/RandomStream.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatTurnPredictionRandomSeedTest,
	"P_RD.SRPGFramework.CombatTurn.RandomSeedContinuity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatTurnPredictionRandomSeedTest::RunTest(const FString& Parameters)
{
	constexpr int32 InitialSeed = 712367;
	constexpr int32 RequiredSpeedPointForTurn = 10;

	// 두 후보의 잔여 속도가 매 라운드 정확히 같아 정렬 순서를 난수만으로
	// 결정하게 한다. 예측 결과는 같은 초기 seed에서 항상 같은 값을 내야 한다.
	TArray<FSRPGTurnCandidate> TiedCandidates;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		FSRPGTurnCandidate& Candidate = TiedCandidates.AddDefaulted_GetRef();
		Candidate.mRemainSpeedPoint = 0;
		Candidate.mRechargedSpeedPoint = RequiredSpeedPointForTurn;
	}

	const TArray<FSRPGPredictedRound> FirstPrediction =
		USRPGCombatModel::PredictTurnRounds(TiedCandidates,
			RequiredSpeedPointForTurn, InitialSeed, /*RoundCount=*/2);
	const TArray<FSRPGPredictedRound> RepeatedPrediction =
		USRPGCombatModel::PredictTurnRounds(TiedCandidates,
			RequiredSpeedPointForTurn, InitialSeed, /*RoundCount=*/2);

	if (!TestEqual(TEXT("동률 예측 라운드 수"), FirstPrediction.Num(), 2)
		|| !TestEqual(TEXT("반복 동률 예측 라운드 수"),
			RepeatedPrediction.Num(), 2))
	{
		return false;
	}

	FRandomStream FirstRoundStream(InitialSeed);
	TArray<float> ExpectedFirstRound = {
		FirstRoundStream.FRand(), FirstRoundStream.FRand()
	};
	const int32 SecondRoundSeed = FirstRoundStream.GetCurrentSeed();
	FRandomStream SecondRoundStream(SecondRoundSeed);
	TArray<float> ExpectedSecondRound = {
		SecondRoundStream.FRand(), SecondRoundStream.FRand()
	};
	ExpectedFirstRound.Sort(TGreater<float>());
	ExpectedSecondRound.Sort(TGreater<float>());

	for (int32 RoundIndex = 0; RoundIndex < 2; ++RoundIndex)
	{
		const FSRPGPredictedRound& Round = FirstPrediction[RoundIndex];
		if (!TestEqual(*FString::Printf(TEXT("동률 라운드 %d 후보 수"),
			RoundIndex + 1), Round.mCandidates.Num(), 2))
		{
			return false;
		}
		const TArray<float>& Expected = RoundIndex == 0
			? ExpectedFirstRound : ExpectedSecondRound;
		for (int32 CandidateIndex = 0; CandidateIndex < 2; ++CandidateIndex)
		{
			TestEqual(*FString::Printf(
				TEXT("동률 라운드 %d 난수 %d"), RoundIndex + 1,
				CandidateIndex),
				Round.mCandidates[CandidateIndex].mRandomTieBreaker,
				Expected[CandidateIndex]);
			TestEqual(*FString::Printf(
				TEXT("같은 seed 반복 결과 라운드 %d 후보 %d"), RoundIndex + 1,
				CandidateIndex),
				RepeatedPrediction[RoundIndex].mCandidates[CandidateIndex]
					.mRandomTieBreaker,
				Round.mCandidates[CandidateIndex].mRandomTieBreaker);
		}
	}

	// 두 번째 라운드가 초기 seed를 다시 사용했다면 첫 라운드 난수열과 같아진다.
	// 정확히 다음 seed를 사용한 기대값을 위에서 비교했으므로, 이 검사는 회귀
	// 원인을 메시지에서 바로 드러내기 위한 보조 단언이다.
	TestTrue(TEXT("중간 라운드에서 갱신된 seed가 다음 라운드로 이어짐"),
		!FMath::IsNearlyEqual(ExpectedFirstRound[0], ExpectedSecondRound[0])
		|| !FMath::IsNearlyEqual(ExpectedFirstRound[1], ExpectedSecondRound[1]));

	return true;
}
