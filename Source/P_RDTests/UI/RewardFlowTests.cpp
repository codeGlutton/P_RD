#include "UI/RewardFlowTests.h"

#include "Misc/AutomationTest.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "UI/Reward/RewardUIModel.h"
#include "UObject/StrongObjectPtr.h"

void URewardFlowTestListener::HandleClaimRequested(ERewardClaimKind ClaimKind, int32 ChoiceIndex)
{
	++mRequestCount;
	mLastRequestKind = ClaimKind;
	mLastRequestChoiceIndex = ChoiceIndex;
}

void URewardFlowTestListener::HandleClaimConfirmed(ERewardClaimKind ClaimKind, int32 ChoiceIndex)
{
	++mConfirmationCount;
	mLastConfirmationKind = ClaimKind;
	mLastConfirmationChoiceIndex = ChoiceIndex;
}

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRewardClaimAcknowledgementTest,
	"P_RD.UI.Reward.ClaimAcknowledgement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRewardClaimAcknowledgementTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<URewardUIModel> UIModel(NewObject<URewardUIModel>());
	TStrongObjectPtr<URewardFlowTestListener> Listener(NewObject<URewardFlowTestListener>());

	UIModel->OnRewardClaimRequested.AddDynamic(
		Listener.Get(), &URewardFlowTestListener::HandleClaimRequested);
	UIModel->OnRewardClaimConfirmed.AddDynamic(
		Listener.Get(), &URewardFlowTestListener::HandleClaimConfirmed);

	UIModel->RequestClaimReward(ERewardClaimKind::Choice, 2);

	TestEqual(TEXT("클릭은 지급 요청을 한 번 보낸다"), Listener->mRequestCount, 1);
	TestEqual(TEXT("요청만으로 수령 완료 처리하지 않는다"), Listener->mConfirmationCount, 0);
	TestEqual(TEXT("요청의 보상 종류가 유지된다"), Listener->mLastRequestKind, ERewardClaimKind::Choice);
	TestEqual(TEXT("요청의 선택 인덱스가 유지된다"), Listener->mLastRequestChoiceIndex, 2);

	UIModel->ConfirmRewardClaim(ERewardClaimKind::Choice, 2);

	TestEqual(TEXT("게임플레이 성공 확정이 한 번 전달된다"), Listener->mConfirmationCount, 1);
	TestEqual(TEXT("확정의 보상 종류가 유지된다"), Listener->mLastConfirmationKind, ERewardClaimKind::Choice);
	TestEqual(TEXT("확정의 선택 인덱스가 유지된다"), Listener->mLastConfirmationChoiceIndex, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRewardInventoryPersistenceTest,
	"P_RD.UI.Reward.InventoryPersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRewardInventoryPersistenceTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<URunPersistData> RunData(NewObject<URunPersistData>());
	const FPrimaryAssetId SkillId(TEXT("Skill"), TEXT("RewardSkill"));
	const FPrimaryAssetId EquipmentId(TEXT("Equipment"), TEXT("RewardEquipment"));

	TestFalse(TEXT("유효하지 않은 스킬 보상은 거절한다"),
		RunData->AddRewardSkill(FPrimaryAssetId()));
	TestFalse(TEXT("유효하지 않은 장비 보상은 거절한다"),
		RunData->AddRewardEquipment(FPrimaryAssetId()));

	TestTrue(TEXT("스킬 보상을 보관함에 넣는다"), RunData->AddRewardSkill(SkillId));
	TestTrue(TEXT("동일 스킬도 다시 보관할 수 있다"), RunData->AddRewardSkill(SkillId));
	TestTrue(TEXT("장비 보상을 보관함에 넣는다"), RunData->AddRewardEquipment(EquipmentId));

	TestEqual(TEXT("동일 스킬 두 개를 모두 보관한다"), RunData->GetRewardSkillIds().Num(), 2);
	TestEqual(TEXT("장비 한 개를 보관한다"), RunData->GetRewardEquipmentIds().Num(), 1);
	TestEqual(TEXT("스킬 획득 로그도 두 번 기록한다"),
		RunData->GetRunLog().mAcquiredSkills.FindRef(SkillId), 2);
	TestEqual(TEXT("장비 획득 로그도 기록한다"),
		RunData->GetRunLog().mAcquiredEquipment.FindRef(EquipmentId), 1);

	RunData->ClearRun();
	TestTrue(TEXT("런 종료 시 스킬 보관함을 비운다"), RunData->GetRewardSkillIds().IsEmpty());
	TestTrue(TEXT("런 종료 시 장비 보관함을 비운다"), RunData->GetRewardEquipmentIds().IsEmpty());
	return true;
}

#endif
