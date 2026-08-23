#include "Reward/RewardPolicyContractTests.h"

#include "Misc/AutomationTest.h"
#include "PCGStage/Room.h"
#include "PCGStage/StageBuilder.h"
#include "UI/Reward/ArtifactRewardPolicy.h"
#include "UI/Reward/RewardUIModel.h"
#include "UObject/StrongObjectPtr.h"

void URewardPolicyTestListener::HandleSelectionRequested(
	const FPrimaryAssetId RewardId)
{
	++SelectionRequestCount;
	LastSelectedRewardId = RewardId;
}

void URewardPolicyTestListener::HandleGrantBundleRequested()
{
	++GrantBundleRequestCount;
}

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRewardSelectionPolicyContractTest,
	"P_RD.Reward.Policy.SelectionRequestUsesPrimaryAssetId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardSelectionPolicyContractTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<URewardUIModel> Model(NewObject<URewardUIModel>());
	TStrongObjectPtr<URewardPolicyTestListener> Listener(
		NewObject<URewardPolicyTestListener>());
	const FPrimaryAssetId ArtifactA(TEXT("Artifact"), TEXT("PolicyA"));
	const FPrimaryAssetId ArtifactB(TEXT("Artifact"), TEXT("PolicyB"));

	FRewardSelectionOfferUI Offer;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		FRewardChoiceUI& Choice = Offer.mOptions.AddDefaulted_GetRef();
		Choice.mChoiceIndex = Index;
		Choice.mKind = ERewardChoiceKind::Artifact;
		Choice.mSourceAssetId = Index == 0 ? ArtifactA : ArtifactB;
	}
	Model->SetSelectionOffer(Offer);
	Model->OnRewardSelectionRequested.AddDynamic(
		Listener.Get(), &URewardPolicyTestListener::HandleSelectionRequested);

	TestEqual(TEXT("SelectOne 정책으로 설정됨"),
		Model->GetAcquisitionPolicy(), ERewardAcquisitionPolicy::SelectOne);
	TestTrue(TEXT("후보의 PrimaryAssetId 선택 요청 수락"),
		Model->RequestSelectReward(ArtifactB));
	TestEqual(TEXT("선택 요청은 한 번만 전달됨"),
		Listener->SelectionRequestCount, 1);
	TestEqual(TEXT("선택 요청 ID 보존"),
		Listener->LastSelectedRewardId, ArtifactB);

	const FPrimaryAssetId Unknown(TEXT("Artifact"), TEXT("Unknown"));
	TestFalse(TEXT("후보 풀 밖의 선택 ID 거절"),
		Model->RequestSelectReward(Unknown));
	TestEqual(TEXT("잘못된 선택은 delegate를 호출하지 않음"),
		Listener->SelectionRequestCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRewardGrantAllPartialFailureContractTest,
	"P_RD.Reward.Policy.GrantAllContinuesAfterFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardGrantAllPartialFailureContractTest::RunTest(
	const FString& Parameters)
{
	const FPrimaryAssetId ArtifactA(TEXT("Artifact"), TEXT("GrantA"));
	const FPrimaryAssetId ArtifactB(TEXT("Artifact"), TEXT("GrantB"));
	const TArray<FPrimaryAssetId> InputIds = { ArtifactA, ArtifactB, ArtifactA };
	TArray<FPrimaryAssetId> AttemptedIds;

	const FRewardGrantBundleResultUI Result = ArtifactRewardPolicy::GrantAll(
		InputIds,
		[&AttemptedIds, &ArtifactB](const FPrimaryAssetId& ArtifactId)
		{
			AttemptedIds.Add(ArtifactId);
			return ArtifactId != ArtifactB;
		});

	TestEqual(TEXT("모든 항목을 입력 순서대로 시도"), AttemptedIds, InputIds);
	TestEqual(TEXT("중복 포함 성공 집합 보존"),
		Result.mGrantedItemIds, TArray<FPrimaryAssetId>({ ArtifactA, ArtifactA }));
	TestEqual(TEXT("실패 항목 한 건 기록"),
		Result.mFailedItemIds, TArray<FPrimaryAssetId>({ ArtifactB }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRewardRoomPolicyCompatibilityContractTest,
	"P_RD.Reward.Room.LegacyFallbackAndConfiguredEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardRoomPolicyCompatibilityContractTest::RunTest(
	const FString& Parameters)
{
	const FPrimaryAssetId LegacyArtifact(TEXT("Artifact"), TEXT("Legacy"));
	const FPrimaryAssetId NewArtifact(TEXT("Artifact"), TEXT("New"));

	FEliteMonsterRoom LegacyRoom;
	LegacyRoom.mRewardArtifactDataId = LegacyArtifact;
	TestEqual(TEXT("미설정 구형 엘리트는 단일 필드로 복구"),
		LegacyRoom.GetEffectiveRewardArtifactDataIds(),
		TArray<FPrimaryAssetId>({ LegacyArtifact }));

	LegacyRoom.mIsConfigured = true;
	TestTrue(TEXT("신규 configured-empty는 구형 단일 필드를 부활시키지 않음"),
		LegacyRoom.GetEffectiveRewardArtifactDataIds().IsEmpty());

	LegacyRoom.mRewardArtifactDataIds = { NewArtifact };
	TestEqual(TEXT("신규 배열이 설정되면 배열을 권위 있게 사용"),
		LegacyRoom.GetEffectiveRewardArtifactDataIds(),
		TArray<FPrimaryAssetId>({ NewArtifact }));

	FTreasureRoom TreasureRoom;
	TreasureRoom.mIsConfigured = true;
	TestTrue(TEXT("보물방 configured-empty는 빈 GrantAll bundle"),
		TreasureRoom.GetEffectiveRewardArtifactDataIds().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRewardArtifactRuntimeCandidateContractTest,
	"P_RD.Reward.ArtifactPool.ExcludesTestAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardArtifactRuntimeCandidateContractTest::RunTest(
	const FString& Parameters)
{
	for (const FName TestAssetName : {
		FName(TEXT("DA_TestArtifact_Common")),
		FName(TEXT("DA_TestArtifact_Rare")),
		FName(TEXT("DA_TestArtifact_Epic")) })
	{
		TestFalse(*FString::Printf(TEXT("%s는 보상 풀에서 제외"),
			*TestAssetName.ToString()),
			StageBuilderAssetFilter::IsRuntimeArtifactCandidate(TestAssetName));
	}

	for (const FName RuntimeAssetName : {
		FName(TEXT("DA_Artifact_Common_BrokenBow")),
		FName(TEXT("DA_Artifact_Rare_Seasoning")),
		FName(TEXT("DA_Artifact_Epic_LeftOver")) })
	{
		TestTrue(*FString::Printf(TEXT("%s는 보상 풀에 유지"),
			*RuntimeAssetName.ToString()),
			StageBuilderAssetFilter::IsRuntimeArtifactCandidate(RuntimeAssetName));
	}
	return true;
}

#endif
