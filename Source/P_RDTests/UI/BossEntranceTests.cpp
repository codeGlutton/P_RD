#include "Misc/AutomationTest.h"
#include "UI/StageVictory/BossEntranceWidget.h"
#include "UI/UITextureLoader.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossEntranceAssetsTest, "P_RD.UI.BossEntrance.AssetsAndRouting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBossEntranceAssetsTest::RunTest(const FString&)
{
    TestFalse(TEXT("Ordinary combat does not show boss entrance"), UBossEntranceWidget::ShouldPlay(false, false, 1));
    TestFalse(TEXT("Cleared room does not replay entrance"), UBossEntranceWidget::ShouldPlay(true, true, 3));
    TestFalse(TEXT("Unknown stage does not choose a substitute boss"), UBossEntranceWidget::ShouldPlay(true, false, 0));
    TestTrue(TEXT("Unsupported stage has no path"), UBossEntranceWidget::VideoPath(4).IsEmpty());
    for (int32 Stage = 1; Stage <= 3; ++Stage)
    {
        TestTrue(TEXT("Uncleared boss room plays entrance"), UBossEntranceWidget::ShouldPlay(true, false, Stage));
        const FString Path = UBossEntranceWidget::VideoPath(Stage);
        if (!TestTrue(TEXT("SVN entrance movie exists"), FPaths::FileExists(Path))) return false;
        const FVector2D Size = RDUITexture::ReadMediaFileDimensions(Path);
        TestEqual(TEXT("Approved landscape video width"), Size.X, 1280.0);
        TestEqual(TEXT("Approved landscape video height"), Size.Y, 720.0);
    }
    return true;
}
#endif
