#include "Misc/AutomationTest.h"
#include "DataAsset/GameplayAssetPolicy.h"
#include "UI/Reward/ArtifactRewardPolicy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameplayAssetPoolRegressionTest,
    "P_RD.Assets.ProductionPoolsExcludeFixtures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGameplayAssetPoolRegressionTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("Prototype artifact cannot be offered from an old save"), GameplayAssetPolicy::IsPlayerFacing(FPrimaryAssetId(TEXT("Artifact"), TEXT("DA_TestArtifact_Rare"))));
    TestFalse(TEXT("Prototype class skill cannot enter a new shop"), GameplayAssetPolicy::IsPlayerFacing(FPrimaryAssetId(TEXT("Active"), TEXT("DA_TestMage_Spell_Epic"))));
    TestFalse(TEXT("Mixed-job developer mercenary is not recruitable"), GameplayAssetPolicy::IsPlayerFacing(FPrimaryAssetId(TEXT("PlayerUnit"), TEXT("DA_TestPlayerUnit"))));
    TestTrue(TEXT("Production Knight keeps its historical Test name"), GameplayAssetPolicy::IsPlayerFacing(FPrimaryAssetId(TEXT("PlayerUnit"), TEXT("DA_TestKnightPlayerUnit"))));
    TestTrue(TEXT("Production treasure room keeps its historical Test name"), GameplayAssetPolicy::IsPlayerFacing(FPrimaryAssetId(TEXT("TreasureRoom"), TEXT("DA_TestTreasure_Stage1"))));
    TestTrue(TEXT("New production artifact remains available"), GameplayAssetPolicy::IsPlayerFacing(FPrimaryAssetId(TEXT("Artifact"), TEXT("DA_Artifact_A001_ChaliceOfLife"))));
    const FPrimaryAssetId Fixture(TEXT("Artifact"), TEXT("DA_TestArtifact_Common"));
    const FPrimaryAssetId Production(TEXT("Artifact"), TEXT("DA_Artifact_A001_ChaliceOfLife"));
    FPrimaryAssetId Selected = Production;
    const TArray<FPrimaryAssetId> SavedCandidates = { Fixture, Production };
    TestFalse(TEXT("Old treasure with only fixtures can complete as a gold-only room"),
        GameplayAssetPolicy::HasPlayerFacingAssets(TArray<FPrimaryAssetId>{Fixture}));
    TestTrue(TEXT("Mixed old treasure still requires claiming its production artifact"),
        GameplayAssetPolicy::HasPlayerFacingAssets(SavedCandidates));
    TestFalse(TEXT("Old saved fixture cannot be claimed even if it is a candidate"),
        ArtifactRewardPolicy::TrySelectOne(SavedCandidates, Fixture, Selected));
    TestFalse(TEXT("Rejected selection clears the previous grant target"), Selected.IsValid());
    TestTrue(TEXT("Valid reward from the same old save can still be claimed"),
        ArtifactRewardPolicy::TrySelectOne(SavedCandidates, Production, Selected));
    TestEqual(TEXT("The real artifact is selected"), Selected, Production);
    return true;
}
