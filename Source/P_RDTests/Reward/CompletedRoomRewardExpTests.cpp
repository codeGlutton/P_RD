#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "GameMode/CombatGameMode.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "SaveGame/SaveCheckpoint.h"
#include "UI/Reward/RewardUIModel.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompletedRoomRewardExpPresentationTest,
	"P_RD.UI.Reward.CompletedRoomExpPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCompletedRoomRewardExpPresentationTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<URunPersistData> SavedRun(NewObject<URunPersistData>());
	TStrongObjectPtr<URunPersistData> RestoredRun(NewObject<URunPersistData>());
	SavedRun->GetRoomTransactionsMutable().ExpClaimed = true;
	TArray<uint8> Bytes;
	TestTrue(TEXT("Serialize completed-room claim"), RDCheckpoint::Serialize(SavedRun.Get(), Bytes));
	TestTrue(TEXT("Restore completed-room claim"), RDCheckpoint::Deserialize(Bytes, RestoredRun.Get()));

	// A newly created reward UI has no pre-claim animation data after a process restart.
	TStrongObjectPtr<URewardUIModel> UIModel(NewObject<URewardUIModel>());
	TestTrue(TEXT("Recreated reward UI has no previous rows"), UIModel->GetReward().mMercenaryExp.IsEmpty());
	FRewardUI Reward;
	Reward.mExpGained = 60;
	FRewardMercenaryExpUI& Row = Reward.mMercenaryExp.AddDefaulted_GetRef();
	Row.mName = FText::FromString(TEXT("Restored mercenary"));
	ACombatGameMode::FillRewardExpWithoutLevelUp(Reward, Row, 7, 42.f, 250.f,
		RestoredRun->GetRoomTransactions().ExpClaimed);
	UIModel->SetReward(Reward);
	const FRewardUI& Displayed = UIModel->GetReward();
	const FRewardMercenaryExpUI& DisplayedRow = Displayed.mMercenaryExp[0];
	TestEqual(TEXT("Already granted EXP is not shown as a new gain"), Displayed.mExpGained, 0);
	TestEqual(TEXT("Current level retained"), DisplayedRow.mLevel, 7);
	TestEqual(TEXT("No repeated level-up marker"), DisplayedRow.GetLevelUpCount(), 0);
	TestEqual(TEXT("Current EXP before retained"), DisplayedRow.mExpBefore, 42.f);
	TestEqual(TEXT("Current EXP after retained"), DisplayedRow.mExpAfter, 42.f);
	TestEqual(TEXT("Current level maximum retained"), DisplayedRow.mMaxExp, 250.f);
	TestEqual(TEXT("Single stable animation segment"), DisplayedRow.mProgressSteps.Num(), 1);
	if (DisplayedRow.mProgressSteps.Num() == 1)
	{
		const FRewardExpProgressStepUI& Step = DisplayedRow.mProgressSteps[0];
		TestEqual(TEXT("Animation has no duplicated EXP increase"), Step.mExpAfter, Step.mExpBefore);
		TestEqual(TEXT("Animation has no duplicated level increase"), Step.mLevelAfter, Step.mLevelBefore);
	}

	// The same no-level-up branch must still preview unclaimed EXP correctly.
	FRewardUI Unclaimed;
	Unclaimed.mExpGained = 60;
	FRewardMercenaryExpUI& UnclaimedRow = Unclaimed.mMercenaryExp.AddDefaulted_GetRef();
	ACombatGameMode::FillRewardExpWithoutLevelUp(Unclaimed, UnclaimedRow, 7, 42.f, 250.f, false);
	TestEqual(TEXT("Unclaimed reward remains visible"), Unclaimed.mExpGained, 60);
	TestEqual(TEXT("Unclaimed preview retains level"), UnclaimedRow.mLevelAfter, 7);
	TestEqual(TEXT("Unclaimed preview adds EXP once"), UnclaimedRow.mExpAfter, 102.f);
	TestEqual(TEXT("Unclaimed animation adds EXP once"), UnclaimedRow.mProgressSteps[0].mExpAfter, 102.f);
	return true;
}
