#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Internationalization/Text.h"
#include "DataAsset/GameplayAssetPolicy.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "Engine/AssetManager.h"
#include "UI/Shop/SkillReplacementDialog.h"
#include "Components/TextBlock.h"
#include "Editor.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSkillLocalizationTest,
    "P_RD.UI.Localization.SkillsAndReplacement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillLocalizationTest::RunTest(const FString&)
{
    auto HasKorean = [](const FString& Text)
    {
        for (TCHAR C : Text) if (C >= 0xAC00 && C <= 0xD7A3) return true;
        return false;
    };
    TArray<UStaticUnitSkillData*> Skills;
    TArray<FPrimaryAssetId> Ids;
    auto& Manager = UAssetManager::Get();
    Manager.GetPrimaryAssetIdList(SkillPrimaryAssetTypes::GetActiveType(), Ids);
    for (const auto& Id : Ids)
        if (auto* Skill = Cast<UStaticUnitSkillData>(Manager.GetPrimaryAssetPath(Id).TryLoad()))
            if (GameplayAssetPolicy::IsPlayerFacing(Skill->GetPrimaryAssetId())) Skills.Add(Skill);
    TestTrue(TEXT("Actual production skill assets loaded"), Skills.Num() > 100);
    const FString OriginalCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
    for (const FString Culture : {FString(TEXT("en")), FString(TEXT("ko")), FString(TEXT("en"))})
    {
        FInternationalization::Get().SetCurrentCulture(Culture);
        auto& Loc = FTextLocalizationManager::Get();
        Loc.WaitForAsyncTasks();
        Loc.UpdateFromLocalizationResource(FPaths::ProjectContentDir()/TEXT("Localization/Game")/Culture/TEXT("Game.locres"));
        for (const auto* Skill : Skills)
            for (const FText* Text : { &Skill->mName, &Skill->mDescription })
            {
                const FString* Source = FTextInspector::GetSourceString(*Text);
                if (!Source || !HasKorean(*Source)) continue;
                if (HasKorean(Text->ToString()) != (Culture==TEXT("ko")))
                {
                    const auto Key = FTextInspector::GetKey(*Text);
                    const auto Namespace = FTextInspector::GetNamespace(*Text);
                    AddError(FString::Printf(TEXT("%s %s source=%s display=%s ns=%s key=%s invariant=%d"),
                        *Culture,*Skill->GetName(),**Source,*Text->ToString(),Namespace.IsSet()?*Namespace.GetValue():TEXT("NONE"),Key.IsSet()?*Key.GetValue():TEXT("NONE"),Text->IsCultureInvariant()));
                }
            }
        auto* Dialog = CreateWidget<USkillReplacementDialog>(GEditor->GetEditorWorldContext().World());
        Dialog->SetSkills(FText::FromString(TEXT("A")), FText::FromString(TEXT("B")), FSlateFontInfo());
        auto* Title = Cast<UTextBlock>(Dialog->GetWidgetFromName(TEXT("ReplaceTitle")));
        TestNotNull(TEXT("Replacement title exists"), Title);
        if (Title) TestEqual(TEXT("Replacement title follows language"),HasKorean(Title->GetText().ToString()),Culture==TEXT("ko"));
    }
    FInternationalization::Get().SetCurrentCulture(OriginalCulture);
    FTextLocalizationManager::Get().WaitForAsyncTasks();
    FTextLocalizationManager::Get().UpdateFromLocalizationResource(
        FPaths::ProjectContentDir()/TEXT("Localization/Game")/OriginalCulture/TEXT("Game.locres"));
    return !HasAnyErrors();
}
#endif
