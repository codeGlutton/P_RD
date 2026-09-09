#include "Misc/AutomationTest.h"
#include "UI/Reward/RewardConcept03Widget.h"
#include "Editor.h"
#include "Components/Button.h"
#include "Components/Image.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossRewardBackgroundTest,"P_RD.UI.BossCollapse.RewardBackground",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBossRewardBackgroundTest::RunTest(const FString&)
{
    UWorld* World=GEditor->GetEditorWorldContext().World();
    UClass* Class=LoadClass<URewardConcept03Widget>(nullptr,TEXT("/Game/UI/RewardConcept03New/WBP_RewardConcept03_Frameless.WBP_RewardConcept03_Frameless_C"));
    if(!TestNotNull(TEXT("Existing reward widget"),Class))return false;
    auto* Reward=CreateWidget<URewardConcept03Widget>(World,Class);
    Reward->TakeWidget();Reward->InitializeInteractionBindingsForTest();
    auto* Background=Cast<UImage>(Reward->GetWidgetFromName(TEXT("NewRewardBackgroundImage")));
    auto* Button=Cast<UButton>(Reward->GetWidgetFromName(TEXT("NewBottomActionButton")));
    if(!TestNotNull(TEXT("Background"),Background)||!TestNotNull(TEXT("Next button"),Button))return false;
    UObject* Original=Background->GetBrush().GetResourceObject();
    for(int32 Stage=1;Stage<=3;++Stage)
    {
        TestTrue(TEXT("Stage background loads"),Reward->SetStageClearBackground(Stage));
        TestEqual(TEXT("Stage matches background"),Background->GetBrush().GetResourceObject()->GetName(),FString::Printf(TEXT("T_StageReward%d"),Stage));
    }
    TestEqual(TEXT("Experience begins immediately"),Reward->GetCurrentStepIndex(),0);
    Button->OnClicked.Broadcast();TestEqual(TEXT("Next goes directly to chest"),Reward->GetCurrentStepIndex(),1);
    Reward->SetStageClearBackground(0);TestEqual(TEXT("Regular rewards restore background"),Background->GetBrush().GetResourceObject(),Original);
    return true;
}

#endif
