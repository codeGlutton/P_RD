#include "Misc/AutomationTest.h"
#include "UI/Reward/LevelUpSkillRewardFlow.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "DataAsset/RarityRate.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "SaveGame/SaveCheckpoint.h"
#include "UObject/StrongObjectPtr.h"
#include "Reward/LevelUpSkillTestPlayer.h"
#include "Component/SkillComponent/UnitSkillComponentModel.h"
#include "Engine/AssetManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelUpSkillCandidateTest,
	"P_RD.Reward.LevelUp.Candidates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FLevelUpSkillCandidateTest::RunTest(const FString& Parameters)
{
	TArray<UStaticUnitSkillData*> Pool;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		UStaticUnitSkillData* Skill = NewObject<UStaticUnitSkillData>(GetTransientPackage(),
			*FString::Printf(TEXT("RewardCandidate%d"), Index));
		Skill->mRarityType = Index < 3 ? ERarityType::Common : ERarityType::Rare;
		Pool.Add(Skill);
	}
	FRarityRate Rate;
	Rate.mWeights[static_cast<uint8>(ERarityType::Common)] = 1.f;
	UStaticUnitSkillData* Fixture = NewObject<UStaticUnitSkillData>(GetTransientPackage(), TEXT("DA_TestKnight_Spell_Common"));
	TestTrue(TEXT("Prototype skills cannot return through the restored level-up reward pool"),
		LevelUpSkillReward::ChooseCandidates(TArray<UStaticUnitSkillData*>{Fixture}, Rate, FRandomStream(17)).IsEmpty());
	const auto First = LevelUpSkillReward::ChooseCandidates(Pool, Rate, FRandomStream(17));
	TestEqual(TEXT("Three choices"), First.Num(), 3);
	TSet<FPrimaryAssetId> Unique(First);
	TestEqual(TEXT("No duplicate candidate"), Unique.Num(), 3);
	for (const auto& Id : First)
		TestTrue(TEXT("Available weighted rarity honored"), Id == Pool[0]->GetPrimaryAssetId()
			|| Id == Pool[1]->GetPrimaryAssetId() || Id == Pool[2]->GetPrimaryAssetId());
	TestTrue(TEXT("Same seed restores identical offer"), First == LevelUpSkillReward::ChooseCandidates(Pool, Rate, FRandomStream(17)));
	Pool.SetNum(2);
	TestEqual(TEXT("Small pool never fabricates or repeats a skill"),
		LevelUpSkillReward::ChooseCandidates(Pool, FRarityRate(), FRandomStream(1)).Num(), 2);
	Pool.Reset();
	TestTrue(TEXT("Exhausted pool is safe"), LevelUpSkillReward::ChooseCandidates(Pool, Rate, FRandomStream(1)).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelUpSkillEquipTest,
	"P_RD.Reward.LevelUp.Equip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FLevelUpSkillEquipTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<ULevelUpSkillTestPlayer> Unit(NewObject<ULevelUpSkillTestPlayer>());
	USkillComponentModel* Skills = Unit->GetSkillComponentModel();
	Skills->SetSkillFrom(TArray<FPrimaryAssetId>());
	UAssetManager& Manager = UAssetManager::Get();
	TArray<FPrimaryAssetId> Ids;
	Manager.GetPrimaryAssetIdList(SkillPrimaryAssetTypes::GetActiveType(), Ids);
	TArray<UStaticUnitSkillData*> Eligible;
	UStaticUnitSkillData* OtherJob = nullptr;
	for (const auto& Id : Ids)
	{
		auto* Skill = Cast<UStaticUnitSkillData>(Manager.GetPrimaryAssetPath(Id).TryLoad());
		if (!Skill || Skill->mSkillPhaseLayers.IsEmpty()) continue;
		if (Skills->IsAcquirableSkill(Skill)) Eligible.Add(Skill);
		else OtherJob = Skill;
	}
	if (!TestTrue(TEXT("Real knight skill pool available"), Eligible.Num() >= 3)
		|| !TestNotNull(TEXT("Other-job skill available"), OtherJob)) return false;
	Skills->SetSkill(0, Eligible[0]);
	FLevelUpSkillReward Reward;
	Reward.Offered = true;
	Reward.Candidates = { Eligible[1]->GetPrimaryAssetId(), Eligible[2]->GetPrimaryAssetId(), OtherJob->GetPrimaryAssetId() };
	TestFalse(TEXT("Basic attack cannot be overwritten"), LevelUpSkillReward::TryEquip(Reward, Unit.Get(), Reward.Candidates[0], 0));
	TestFalse(TEXT("Out-of-range slot rejected"), LevelUpSkillReward::TryEquip(Reward, Unit.Get(), Reward.Candidates[0], 6));
	TestFalse(TEXT("Other-job candidate rejected by gameplay"), LevelUpSkillReward::TryEquip(Reward, Unit.Get(), Reward.Candidates[2], 1));
	TestFalse(TEXT("Unoffered skill rejected"), LevelUpSkillReward::TryEquip(Reward, Unit.Get(), Eligible[0]->GetPrimaryAssetId(), 1));
	TestTrue(TEXT("Chosen skill actually equipped"), LevelUpSkillReward::TryEquip(Reward, Unit.Get(), Reward.Candidates[0], 1));
	TestTrue(TEXT("Correct skill in model"), Skills->GetSkill(1)->mData == Eligible[1]);
	TestFalse(TEXT("Same reward cannot grant twice"), LevelUpSkillReward::TryEquip(Reward, Unit.Get(), Reward.Candidates[1], 2));
	Reward.Completed = false;
	TestFalse(TEXT("Next reward cannot duplicate an owned skill"), LevelUpSkillReward::TryEquip(Reward, Unit.Get(), Reward.Candidates[0], 2));
	TestTrue(TEXT("Full slot can be explicitly replaced"), LevelUpSkillReward::TryEquip(Reward, Unit.Get(), Reward.Candidates[1], 1));
	TestTrue(TEXT("Basic attack preserved"), Skills->GetSkill(0)->mData == Eligible[0]);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelUpSkillSaveTest,
	"P_RD.Reward.LevelUp.Restart", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FLevelUpSkillSaveTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<URunPersistData> Run(NewObject<URunPersistData>());
	Run->GetRoomTransactionsMutable().ExpClaimed = true;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		auto& Reward = Run->GetRoomTransactionsMutable().LevelUpSkills.AddDefaulted_GetRef();
		Reward.UnitIndex = Index == 2 ? 1 : 0;
		Reward.Level = Index == 1 ? 3 : 2;
		Reward.Completed = Index == 0;
		Reward.Offered = Index < 2;
		if (Reward.Offered) Reward.Candidates.Add(FPrimaryAssetId(TEXT("Skill"), TEXT("FixedOffer")));
	}
	TArray<uint8> Bytes;
	TStrongObjectPtr<URunPersistData> Restored(NewObject<URunPersistData>());
	TestTrue(TEXT("Save after first choice"), RDCheckpoint::Serialize(Run.Get(), Bytes));
	TestTrue(TEXT("Load pending choices"), RDCheckpoint::Deserialize(Bytes, Restored.Get()));
	const auto& State = Restored->GetRoomTransactions();
	TestTrue(TEXT("EXP is not awarded twice"), State.ExpClaimed);
	if (!TestEqual(TEXT("Multi-level and multi-unit queue retained"), State.LevelUpSkills.Num(), 3)) return false;
	TestTrue(TEXT("Granted choice stays completed"), State.LevelUpSkills[0].Completed);
	TestFalse(TEXT("Second level still pending"), State.LevelUpSkills[1].Completed);
	TestTrue(TEXT("Visible offer is retained without reroll"), State.LevelUpSkills[1].Candidates == Run->GetRoomTransactions().LevelUpSkills[1].Candidates);
	TestEqual(TEXT("Next mercenary preserved"), State.LevelUpSkills[2].UnitIndex, 1);
	TestFalse(TEXT("Next offer waits until earlier equipment is applied"), State.LevelUpSkills[2].Offered);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelUpRewardRecipientTest,
	"P_RD.Reward.LevelUp.RecipientAndSkip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FLevelUpRewardRecipientTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<URunPersistData> Run(NewObject<URunPersistData>());
	auto& Rewards = Run->GetRoomTransactionsMutable().LevelUpSkills;
	for (int32 UnitIndex : {0, 0, 1})
	{
		auto& Reward = Rewards.AddDefaulted_GetRef();
		Reward.UnitIndex = UnitIndex;
		Reward.Offered = true;
		Reward.Candidates.Add(FPrimaryAssetId(TEXT("Skill"), TEXT("SavedChoice")));
	}
	TestEqual(TEXT("Second ally can be chosen before first ally"), LevelUpSkillReward::FindPendingReward(Rewards, 1), 2);
	TestTrue(TEXT("A nonempty offer may be skipped"), LevelUpSkillReward::Skip(Rewards[2]));
	TestFalse(TEXT("Skip cannot complete the same offer twice"), LevelUpSkillReward::Skip(Rewards[2]));
	TestEqual(TEXT("Other ally keeps earliest unclaimed level"), LevelUpSkillReward::FindPendingReward(Rewards, 0), 0);
	TestEqual(TEXT("Completed ally has no pending reward"), LevelUpSkillReward::FindPendingReward(Rewards, 1), INDEX_NONE);
	TArray<uint8> Bytes;
	TStrongObjectPtr<URunPersistData> Restored(NewObject<URunPersistData>());
	TestTrue(TEXT("Save out-of-order skip"), RDCheckpoint::Serialize(Run.Get(), Bytes));
	TestTrue(TEXT("Restore out-of-order skip"), RDCheckpoint::Deserialize(Bytes, Restored.Get()));
	const auto& Loaded = Restored->GetRoomTransactions().LevelUpSkills;
	TestTrue(TEXT("Skipped offer remains complete"), Loaded[2].Completed);
	TestTrue(TEXT("Changing recipients preserves candidates"), Loaded[0].Candidates == Rewards[0].Candidates);
	TestEqual(TEXT("Unfinished first ally survives restart"), LevelUpSkillReward::FindPendingReward(Loaded, 0), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelUpSkillIconTest,
	"P_RD.Reward.LevelUp.SkillIcons", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FLevelUpSkillIconTest::RunTest(const FString& Parameters)
{
	TArray<FPrimaryAssetId> Ids;
	UAssetManager::Get().GetPrimaryAssetIdList(SkillPrimaryAssetTypes::GetActiveType(), Ids);
	TestTrue(TEXT("Real player skills are registered"), Ids.Num() > 0);
	for (const auto& Id : Ids)
		if (auto* Skill = Cast<UStaticUnitSkillData>(UAssetManager::Get().GetPrimaryAssetPath(Id).TryLoad()))
			TestNotNull(*FString::Printf(TEXT("Skill icon loads: %s"), *Id.ToString()), Skill->mIcon.LoadSynchronous());
	return true;
}
