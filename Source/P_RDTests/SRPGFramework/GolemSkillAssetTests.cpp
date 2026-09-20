#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Engine/AssetManager.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGolemSkillAssetReferencesTest,
    "P_RD.Assets.GolemSkillReferences",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGolemSkillAssetReferencesTest::RunTest(const FString&)
{
    auto& Manager = UAssetManager::Get();
    auto* Golem = LoadObject<UStaticEnemyUnitSpawnData>(nullptr,
        TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_GolemUnit.DA_GolemUnit"));
    if (!TestNotNull(TEXT("Production golem unit loads"), Golem)) return false;

    for (const TCHAR* Suffix : { TEXT("EarthQuake"), TEXT("Stamp"), TEXT("Swing") })
    {
        const FString OldName = FString::Printf(TEXT("DA_Goelm_%s"), Suffix);
        const FString NewName = FString::Printf(TEXT("DA_Golem_%s"), Suffix);
        const FPrimaryAssetId OldId(SkillPrimaryAssetTypes::GetActiveType(), FName(*OldName));
        const FPrimaryAssetId NewId(SkillPrimaryAssetTypes::GetActiveType(), FName(*NewName));
        TestEqual(TEXT("Persisted old skill IDs redirect to the renamed skill"),
            Manager.GetRedirectedPrimaryAssetId(OldId), NewId);

        const FString Package = TEXT("/Game/BP/DataAsset/Skill/Enemy/Stage1/Golem/") + NewName;
        auto* Skill = LoadObject<UStaticUnitSkillData>(nullptr, *(Package + TEXT(".") + NewName));
        if (!TestNotNull(*NewName, Skill)) continue;
        TestEqual(TEXT("Asset manager resolves the renamed skill"),
            Manager.GetPrimaryAssetPath(NewId).TryLoad(), static_cast<UObject*>(Skill));
        TestFalse(TEXT("Old misspelled package/redirector was removed"),
            FPaths::FileExists(FPaths::ProjectContentDir() /
                TEXT("BP/DataAsset/Skill/Enemy/Stage1/Golem") / (OldName + TEXT(".uasset"))));
    }
    TestTrue(TEXT("Production golem retains its equipped skills"), !Golem->mSkillDatas.IsEmpty());
    for (const auto& Ref : Golem->mSkillDatas)
    {
        TestFalse(TEXT("Saved unit references no longer contain the old spelling"),
            Ref.ToSoftObjectPath().ToString().Contains(TEXT("DA_Goelm_")));
        TestNotNull(TEXT("Equipped golem skill resolves without an old redirector"), Ref.LoadSynchronous());
    }
    return !HasAnyErrors();
}
