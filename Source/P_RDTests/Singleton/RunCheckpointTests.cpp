#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "Singleton/RunCheckpointTestsHelper.h"
#include "SaveGame/SaveCheckpoint.h"
#include "SaveGame/BinarySaveGame.h"
#include "Pawn/PlayerLevelProgressionTestsHelper.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

namespace
{
	struct FTemporaryCheckpointSlot
	{
		FString Name = TEXT("P_RD_CheckpointTest_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
		~FTemporaryCheckpointSlot()
		{
			UGameplayStatics::DeleteGameInSlot(Name, 0);
			UGameplayStatics::DeleteGameInSlot(Name + TEXT("_A"), 0);
			UGameplayStatics::DeleteGameInSlot(Name + TEXT("_B"), 0);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunCheckpointRoundTripTest, "P_RD.Save.CheckpointRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRunCheckpointRoundTripTest::RunTest(const FString& Parameters)
{
	FTemporaryCheckpointSlot Slot;
	TStrongObjectPtr<URunCheckpointTestData> Live(NewObject<URunCheckpointTestData>());
	TStrongObjectPtr<URunCheckpointTestData> Recreated(NewObject<URunCheckpointTestData>());
	Live->Seed();
	FRandomStream ExpectedEvent = Live->GetEventStream();
	FRandomStream ExpectedStage = Live->GetStageBuildStream();
	const TArray<FPrimaryAssetId> ExpectedParty = Live->GetPlayerUnitIds();
	FRunRoomCheckpoint Checkpoint;
	TestTrue(TEXT("Capture before combat generation"), Checkpoint.Capture(Live.Get(), true));
	Live->MutateDuringCombat();
	TArray<uint8> Data, ReadBack;
	TestTrue(TEXT("Build save using entry snapshot"), Checkpoint.MakeSaveData(Live.Get(), Data));
	TestTrue(TEXT("Write physical checkpoint"), RDCheckpoint::SaveSlot(Slot.Name, Data));
	TestTrue(TEXT("Read physical checkpoint"), RDCheckpoint::LoadSlot(Slot.Name, ReadBack));
	TestTrue(TEXT("Recreate run after process restart"), RDCheckpoint::Deserialize(ReadBack, Recreated.Get()));
	TestEqual(TEXT("Entry money restored"), Recreated->Money(), 100.f);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TestEqual(FString::Printf(TEXT("Party %d entry HP"), Index), Recreated->HP(Index), 80.f + Index);
	}
	TestTrue(TEXT("Party identities restored"), Recreated->GetPlayerUnitIds() == ExpectedParty);
	TestEqual(TEXT("Entry artifacts restored"), Recreated->GetArtifactIds().Num(), 1);
	TestEqual(TEXT("Entry skills restored"), Recreated->GetRewardSkillIds().Num(), 1);
	TestEqual(TEXT("Entry equipment restored"), Recreated->GetRewardEquipmentIds().Num(), 1);
	TestEqual(TEXT("Entry run log restored"), Recreated->GetRunLog().mKilledEnemyUnits.Num(), 1);
	TestEqual(TEXT("Combat starts incomplete"), Recreated->GetStage().mClearData.mIsCleared, false);
	TestEqual(TEXT("Room index restored"), Recreated->GetStage().mCurColumn, 0);
	TestEqual(TEXT("Event stream resumes before combat"), Recreated->GetEventStream().FRand(), ExpectedEvent.FRand());
	TestEqual(TEXT("Stage stream resumes before combat"), Recreated->GetStageBuildStream().FRand(), ExpectedStage.FRand());
	TestTrue(TEXT("Restore in-memory run before frontend autosave"), Checkpoint.Restore(Live.Get()));
	TestEqual(TEXT("Frontend cannot overwrite entry with combat damage"), Live->HP(0), 80.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunTransactionRoundTripTest, "P_RD.Save.TransactionRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRunTransactionRoundTripTest::RunTest(const FString& Parameters)
{
	FTemporaryCheckpointSlot Slot;
	TStrongObjectPtr<URunCheckpointTestData> Live(NewObject<URunCheckpointTestData>());
	TStrongObjectPtr<URunCheckpointTestData> Recreated(NewObject<URunCheckpointTestData>());
	Live->Seed();
	FRunRoomCheckpoint Checkpoint;
	Checkpoint.Capture(Live.Get(), true);
	Live->GrantCompletedRoomTransaction();
	Checkpoint.Reset();
	TArray<uint8> Bytes;
	TestTrue(TEXT("Completed combat serializes live transaction"), Checkpoint.MakeSaveData(Live.Get(), Bytes));
	TestTrue(TEXT("Commit transaction to disk"), RDCheckpoint::SaveSlot(Slot.Name, Bytes));
	TestTrue(TEXT("Read transaction from disk"), RDCheckpoint::LoadSlot(Slot.Name, Bytes));
	TestTrue(TEXT("Deserialize transaction"), RDCheckpoint::Deserialize(Bytes, Recreated.Get()));
	const FRoomTransactionState& Claims = Recreated->GetRoomTransactions();
	TestEqual(TEXT("Granted currency durable"), Recreated->Money(), 150.f);
	TestTrue(TEXT("Gold and EXP claims durable"), Claims.GoldClaimed && Claims.ExpClaimed);
	TestTrue(TEXT("Treasure and rest use durable"), Claims.TreasureOpened && Claims.RestUsed);
	TestTrue(TEXT("Granted artifact and claim stored together"), Recreated->GetArtifactIds().Contains(Claims.SelectedArtifact));
	TestTrue(TEXT("Choice claims durable"), Claims.ClaimedChoices.Contains(2));
	TestTrue(TEXT("Shop purchases durable"), Claims.SoldShopSlots.Contains(4));
	TestTrue(TEXT("Completed-room resume retains completion"), Recreated->GetStage().mClearData.mIsCleared);
	Recreated->SetCurrentRoomIndex(0, 0);
	TestTrue(TEXT("Re-entering same room retains claims"), Recreated->GetRoomTransactions().GoldClaimed);
	Recreated->SetCurrentRoomIndex(0, 1);
	TestFalse(TEXT("Next room has fresh claims"), Recreated->GetRoomTransactions().GoldClaimed);
	TestTrue(TEXT("Next room has fresh shop stock"), Recreated->GetRoomTransactions().SoldShopSlots.IsEmpty());
	Recreated->ClearRun();
	TestFalse(TEXT("Abandon clears active run"), Recreated->IsActive());
	TestTrue(TEXT("Serialize abandoned run"), RDCheckpoint::Serialize(Recreated.Get(), Bytes));
	TestTrue(TEXT("Persist abandoned run"), RDCheckpoint::SaveSlot(Slot.Name, Bytes));
	TestTrue(TEXT("Reload abandoned run"), RDCheckpoint::LoadSlot(Slot.Name, Bytes));
	TestTrue(TEXT("Apply abandoned run"), RDCheckpoint::Deserialize(Bytes, Live.Get()));
	TestFalse(TEXT("Abandoned run cannot continue after restart"), Live->IsActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCheckpointStorageRecoveryTest, "P_RD.Save.RecoveryAndWriteOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCheckpointStorageRecoveryTest::RunTest(const FString& Parameters)
{
	FTemporaryCheckpointSlot Slot;
	const TArray<uint8> First = { 1, 2, 3, 4 }, Second = { 5, 6, 7, 8 }, Latest = { 9, 10, 11, 12 };
	TArray<uint8> Loaded;
	TestTrue(TEXT("Initial write"), RDCheckpoint::SaveSlot(Slot.Name, First));
	TestTrue(TEXT("Second bank write"), RDCheckpoint::SaveSlot(Slot.Name, Second));
	TestTrue(TEXT("Latest bank read"), RDCheckpoint::LoadSlot(Slot.Name, Loaded));
	TestTrue(TEXT("Latest data selected"), Loaded == Second);
	TestFalse(TEXT("Invalid write fails"), RDCheckpoint::SaveSlot(Slot.Name, TArray<uint8>()));
	TestTrue(TEXT("Failed write retains latest checkpoint"), RDCheckpoint::LoadSlot(Slot.Name, Loaded) && Loaded == Second);
	TArray<uint8> Corrupt;
	TestTrue(TEXT("Read newest bank to simulate interrupted write"), UGameplayStatics::LoadDataFromSlot(Corrupt, Slot.Name + TEXT("_B"), 0));
	if (Corrupt.IsEmpty()) return false;
	Corrupt.Last() ^= 0xff;
	TestTrue(TEXT("Write damaged bank"), UGameplayStatics::SaveDataToSlot(Corrupt, Slot.Name + TEXT("_B"), 0));
	TestTrue(TEXT("Recover intact previous bank"), RDCheckpoint::LoadSlot(Slot.Name, Loaded) && Loaded == First);
	RDCheckpoint::SaveSlotAsync(Slot.Name, Second, [](bool) {});
	TestTrue(TEXT("Background flush waits for queued save then commits latest"), RDCheckpoint::SaveSlot(Slot.Name, Latest));
	TestTrue(TEXT("Older async save cannot overwrite background checkpoint"), RDCheckpoint::LoadSlot(Slot.Name, Loaded) && Loaded == Latest);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCheckpointLegacyMigrationTest, "P_RD.Save.LegacyMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCheckpointLegacyMigrationTest::RunTest(const FString& Parameters)
{
	FTemporaryCheckpointSlot Slot;
	TStrongObjectPtr<UBinarySaveGame> Legacy(NewObject<UBinarySaveGame>());
	Legacy->mData = { 10, 20, 30 };
	TestTrue(TEXT("Create pre-migration save"), UGameplayStatics::SaveGameToSlot(Legacy.Get(), Slot.Name, 0));
	TArray<uint8> Data;
	TestTrue(TEXT("Recognize legacy save"), RDCheckpoint::DoesSlotExist(Slot.Name));
	TestTrue(TEXT("Read legacy payload"), RDCheckpoint::LoadSlot(Slot.Name, Data) && Data == Legacy->mData);
	Data = { 40, 50, 60 };
	TestTrue(TEXT("Migrate on next save"), RDCheckpoint::SaveSlot(Slot.Name, Data));
	TestTrue(TEXT("Legacy source preserved"), UGameplayStatics::DoesSaveGameExist(Slot.Name, 0));
	TArray<uint8> Loaded;
	TestTrue(TEXT("New checkpoint takes precedence"), RDCheckpoint::LoadSlot(Slot.Name, Loaded) && Loaded == Data);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCheckpointPassiveStackReinsertTest, "P_RD.Save.PassiveStackReinsert",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCheckpointPassiveStackReinsertTest::RunTest(const FString& Parameters)
{
	FTemporaryCheckpointSlot Slot;
	TStrongObjectPtr<UPlayerLevelProgressionTestModel> Model(NewObject<UPlayerLevelProgressionTestModel>());
	TStrongObjectPtr<UCheckpointPlayerPersistTestData> Data(NewObject<UCheckpointPlayerPersistTestData>());
	Model->Initialize();
	Data->RegisterPlayerUnit(Model.Get());
	const FGameplayTag Stack = FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Cost.PassiveStack"));
	UAttributeSetComponentModel* Attributes = Model->GetAttributeComponentModel();
	Attributes->AddLooseGameplayTag(Stack, 1);
	TestEqual(TEXT("First stack inserts missing map entry"), Data->StackCount(Stack), 1);
	Attributes->RemoveLooseGameplayTag(Stack, 1);
	TestEqual(TEXT("Removing all stacks removes entry"), Data->StackCount(Stack), 0);
	Attributes->AddLooseGameplayTag(Stack, 2);
	TestEqual(TEXT("Stack can be reinserted after removal"), Data->StackCount(Stack), 2);
	TArray<uint8> Bytes;
	TestTrue(TEXT("Serialize tracked stacks"), RDCheckpoint::Serialize(Data.Get(), Bytes));
	TestTrue(TEXT("Write tracked stacks"), RDCheckpoint::SaveSlot(Slot.Name, Bytes));
	TestTrue(TEXT("Read tracked stacks"), RDCheckpoint::LoadSlot(Slot.Name, Bytes));
	TStrongObjectPtr<UCheckpointPlayerPersistTestData> Reloaded(NewObject<UCheckpointPlayerPersistTestData>());
	TestTrue(TEXT("Deserialize tracked stacks"), RDCheckpoint::Deserialize(Bytes, Reloaded.Get()));
	TestEqual(TEXT("Passive stack count survives restart"), Reloaded->StackCount(Stack), 2);
	Data->UnregisterPlayerUnit(Model.Get());
	Model->Uninitialize();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCheckpointStalledRoundGuardTest, "P_RD.Save.StalledRoundGuard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCheckpointStalledRoundGuardTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Positive fractional speed can accumulate"), USRPGCombatModel::CanAccumulateTurn(0.f, .5f, 100.f));
	TestTrue(TEXT("Tiny positive speed is not falsely impossible"), USRPGCombatModel::CanAccumulateTurn(0.f, .0001f, 100.f));
	TestTrue(TEXT("Stored speed can produce a turn without recharge"), USRPGCombatModel::CanAccumulateTurn(100.f, 0.f, 100.f));
	TestFalse(TEXT("Zero speed below threshold is stalled"), USRPGCombatModel::CanAccumulateTurn(1.f, 0.f, 100.f));
	TestFalse(TEXT("Invalid threshold rejected"), USRPGCombatModel::CanAccumulateTurn(1.f, 1.f, 0.f));
	TestFalse(TEXT("NaN rejected"), USRPGCombatModel::CanAccumulateTurn(std::numeric_limits<float>::quiet_NaN(), 1.f, 100.f));
	TStrongObjectPtr<UCheckpointCombatTestModel> Model(NewObject<UCheckpointCombatTestModel>());
	int32 BlockedCount = 0;
	Model->OnCombatProgressBlocked.AddLambda([&BlockedCount]() { ++BlockedCount; });
	TStrongObjectPtr<UCheckpointCombatTestModel> Preview(DuplicateObject<UCheckpointCombatTestModel>(Model.Get(), GetTransientPackage()));
	TestFalse(TEXT("Native exit listener is not copied to simulation preview"), Preview->OnCombatProgressBlocked.IsBound());
	AddExpectedError(TEXT("Combat speed cannot produce a turn"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Empty combat returns instead of spinning in Shipping"), Model->EvaluateEmptyRound());
	TestEqual(TEXT("Blocked combat reports once"), BlockedCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveExitCompletionRaceTest, "P_RD.Save.SaveExitCompletionRace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSaveExitCompletionRaceTest::RunTest(const FString& Parameters)
{
	FTemporaryCheckpointSlot Slot;
	TStrongObjectPtr<URunCheckpointTestData> Live(NewObject<URunCheckpointTestData>());
	TStrongObjectPtr<URunCheckpointTestData> Restarted(NewObject<URunCheckpointTestData>());
	Live->Seed();
	FRunRoomCheckpoint Checkpoint;
	TestTrue(TEXT("Capture incomplete combat entry"), Checkpoint.Capture(Live.Get(), true));
	TestFalse(TEXT("Cancelled exit before victory keeps entry policy"), Checkpoint.CancelExit());
	TestTrue(TEXT("Still using entry checkpoint"), Checkpoint.IsCombatEntry());
	Live->GrantCompletedRoomTransaction();
	Checkpoint.CompleteCombat(true);
	TArray<uint8> Data;
	TestTrue(TEXT("Victory during exit keeps entry while transition pending"), Checkpoint.MakeSaveData(Live.Get(), Data));
	TestTrue(TEXT("Read pending exit snapshot"), RDCheckpoint::Deserialize(Data, Restarted.Get()));
	TestFalse(TEXT("Successful exit would restart incomplete combat"), Restarted->GetStage().mClearData.mIsCleared);
	TestTrue(TEXT("Failed save/transition releases deferred completed combat"), Checkpoint.CancelExit());
	TestTrue(TEXT("Build post-failure live snapshot"), Checkpoint.MakeSaveData(Live.Get(), Data));
	TestTrue(TEXT("Persist victory after failed exit"), RDCheckpoint::SaveSlot(Slot.Name, Data));
	TestTrue(TEXT("Read victory after failed exit"), RDCheckpoint::LoadSlot(Slot.Name, Data));
	TestTrue(TEXT("Restore victory after failed exit"), RDCheckpoint::Deserialize(Data, Restarted.Get()));
	TestTrue(TEXT("Victory remains completed"), Restarted->GetStage().mClearData.mIsCleared);
	TestEqual(TEXT("Earned rewards remain granted"), Restarted->Money(), 150.f);
	TestTrue(TEXT("Claim markers remain granted"), Restarted->GetRoomTransactions().GoldClaimed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompletedRunJournalRecoveryTest, "P_RD.Save.CompletedRunJournalRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCompletedRunJournalRecoveryTest::RunTest(const FString& Parameters)
{
	FTemporaryCheckpointSlot RunSlot, UserSlot;
	TStrongObjectPtr<URunCheckpointTestData> Run(NewObject<URunCheckpointTestData>());
	TStrongObjectPtr<UUserPersistData> User(NewObject<UUserPersistData>());
	User->MakeUser(FText::FromString(TEXT("Checkpoint Test")));
	TArray<uint8> Bytes;
	TestTrue(TEXT("Serialize user before run completion"), RDCheckpoint::Serialize(User.Get(), Bytes));
	TestTrue(TEXT("Persist user before run completion"), RDCheckpoint::SaveSlot(UserSlot.Name, Bytes));
	Run->Seed();
	Run->QueueCompletedRunLog(Run->GetRunLog());
	Run->ClearRun();
	// User disk writes can remain unavailable through a second run.
	Run->Seed();
	Run->QueueCompletedRunLog(Run->GetRunLog());
	Run->ClearRun();
	TestEqual(TEXT("Clearing/starting runs retains uncommitted journals"), Run->GetPendingCompletedRuns().Num(), 2);
	TestTrue(TEXT("Serialize abandoned run with journal"), RDCheckpoint::Serialize(Run.Get(), Bytes));
	TestTrue(TEXT("Persist abandoned run with journal"), RDCheckpoint::SaveSlot(RunSlot.Name, Bytes));
	TStrongObjectPtr<URunCheckpointTestData> RestartedRun(NewObject<URunCheckpointTestData>());
	TStrongObjectPtr<UUserPersistData> RestartedUser(NewObject<UUserPersistData>());
	TestTrue(TEXT("Read run after kill between slots"), RDCheckpoint::LoadSlot(RunSlot.Name, Bytes));
	TestTrue(TEXT("Recreate run journal"), RDCheckpoint::Deserialize(Bytes, RestartedRun.Get()));
	TestFalse(TEXT("Ended run stays inactive while log awaits commit"), RestartedRun->IsActive());
	TestTrue(TEXT("Read old user after kill"), RDCheckpoint::LoadSlot(UserSlot.Name, Bytes));
	TestTrue(TEXT("Recreate old user"), RDCheckpoint::Deserialize(Bytes, RestartedUser.Get()));
	for (const FPendingCompletedRun& Pending : RestartedRun->GetPendingCompletedRuns())
	{
		TestTrue(TEXT("Apply missing completed run exactly once"), RestartedUser->ApplyRunLogOnce(Pending.TransactionId, Pending.Log));
	}
	TestEqual(TEXT("Both completed runs recovered"), RestartedUser->GetUserLog().mRunCount, 2);
	TestTrue(TEXT("Discovery log recovered"), RestartedUser->GetUserLog().mKnownEnemyUnitIds.Contains(FPrimaryAssetId(TEXT("Enemy"), TEXT("Entry"))));
	TestTrue(TEXT("Serialize committed user"), RDCheckpoint::Serialize(RestartedUser.Get(), Bytes));
	TestTrue(TEXT("Persist committed user"), RDCheckpoint::SaveSlot(UserSlot.Name, Bytes));
	// Kill again before removing the Run journal: the User transaction IDs prevent replay.
	TestTrue(TEXT("Reload committed user"), RDCheckpoint::LoadSlot(UserSlot.Name, Bytes));
	TestTrue(TEXT("Apply committed user"), RDCheckpoint::Deserialize(Bytes, User.Get()));
	for (const FPendingCompletedRun& Pending : RestartedRun->GetPendingCompletedRuns())
	{
		TestFalse(TEXT("Recovered journal cannot double count"), User->ApplyRunLogOnce(Pending.TransactionId, Pending.Log));
	}
	TestEqual(TEXT("Run count still exactly two"), User->GetUserLog().mRunCount, 2);
	return true;
}
