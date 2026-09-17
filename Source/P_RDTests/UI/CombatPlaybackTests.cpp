#include "Misc/AutomationTest.h"
#include "Component/TimeScaleComponent/CombatPlaybackSpeed.h"
#include "Editor.h"
#include "Component/TimeScaleComponent/TimeScaleComponent.h"
#include "Component/TimeScaleComponent/CombatPlaybackComponent.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatSpeedWidget.h"
#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerController.h"
#include "Pawn/Camera/CombatCameraPawn.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TextureCompiler.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatPlaybackLocalizationTest, "P_RD.Combat.Playback.Localization", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatPlaybackLocalizationTest::RunTest(const FString&)
{
    auto& I18N = FInternationalization::Get();
    const FString Previous = I18N.GetCurrentLanguage()->GetName();
    ON_SCOPE_EXIT { I18N.SetCurrentLanguage(Previous); };
    auto* World = GEditor->GetEditorWorldContext().World();
    auto* Widget = CreateWidget<UCombatSpeedWidget>(World);
    auto* Model = NewObject<UCombatUIModel>(Widget);
    Model->SetPlaybackSpeed(8, true);
    for (const TCHAR* Language : {TEXT("en"), TEXT("ko")})
    {
        I18N.SetCurrentLanguage(Language);
        Widget->Configure(Model);
        const FString Tip = Widget->GetToolTipText().ToString();
        TestTrue(TEXT("Localized tooltip shows the full range"), Tip.Contains(
            FCString::Strcmp(Language, TEXT("en")) == 0 ? TEXT("1x to 8x") : TEXT("1~8배")));
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatPlaybackMixerTest, "P_RD.Combat.Playback.MixingAndLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatPlaybackMixerTest::RunTest(const FString&)
{
    auto* World = GEditor->GetEditorWorldContext().World();
    auto* Actor = World->SpawnActor<AActor>();
    auto* Mixer = NewObject<UTimeScaleComponent>(Actor);
    Mixer->RegisterComponent();
    const float Original = World->GetWorldSettings()->TimeDilation;
    auto Playback = Mixer->RequestTimeScale(Actor, 8.f);
    Mixer->SetTimeScaleImmediately(Playback, 8.f);
    TestEqual(TEXT("8x playback"), World->GetWorldSettings()->TimeDilation, 8.f);
    auto Slow = Mixer->RequestTimeScale(Actor, .2f);
    Mixer->TickComponent(.01f, LEVELTICK_All, nullptr);
    TestTrue(TEXT("Cinematic and playback multiply"), FMath::IsNearlyEqual(World->GetWorldSettings()->TimeDilation, 1.6f, 0.0001f));
    Mixer->SetTimeScaleImmediately(Playback, 2.f);
    TestTrue(TEXT("Changing playback preserves cinematic"), FMath::IsNearlyEqual(World->GetWorldSettings()->TimeDilation, .4f, 0.0001f));
    Mixer->ReleaseTimeScale(Slow); Mixer->TickComponent(.01f, LEVELTICK_All, nullptr);
    TestEqual(TEXT("Ending cinematic preserves selected playback"), World->GetWorldSettings()->TimeDilation, 2.f);
    auto Other = Mixer->RequestTimeScale(Actor, .5f);
    Mixer->TickComponent(.01f, LEVELTICK_All, nullptr);
    Mixer->ReleaseTimeScaleImmediately(Playback);
    TestEqual(TEXT("Leaving combat removes only playback request"), World->GetWorldSettings()->TimeDilation, .5f);
    Mixer->ReleaseTimeScaleImmediately(Other);
    TestEqual(TEXT("No leftover speed"), World->GetWorldSettings()->TimeDilation, 1.f);
    Mixer->DestroyComponent();
    World->GetWorldSettings()->TimeDilation = Original; Actor->Destroy();
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatPlaybackSaveTest, "P_RD.Combat.Playback.Save", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatPlaybackSaveTest::RunTest(const FString&)
{
    auto* Data = NewObject<UOptionPersistData>();
    TestEqual(TEXT("Old saves default to 1x"), Data->GetCombatPlaybackSpeed(), 1);
    Data->SetCombatPlaybackSpeed(8);
    TArray<uint8> Bytes;
    { FMemoryWriter W(Bytes); FObjectAndNameAsStringProxyArchive A(W,false); A.ArIsSaveGame=true; A.ArNoDelta=true; Data->Serialize(A); }
    auto* Loaded = NewObject<UOptionPersistData>();
    { FMemoryReader R(Bytes); FObjectAndNameAsStringProxyArchive A(R,false); A.ArIsSaveGame=true; Loaded->Serialize(A); }
    TestEqual(TEXT("8x survives option serialization"), Loaded->GetCombatPlaybackSpeed(), 8);
    Loaded->SetCombatPlaybackSpeed(99); TestEqual(TEXT("At most 8x"), Loaded->GetCombatPlaybackSpeed(), 8);
    Loaded->SetCombatPlaybackSpeed(0); TestEqual(TEXT("At least 1x"), Loaded->GetCombatPlaybackSpeed(), 1);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatPlaybackHUDTest, "P_RD.Combat.Playback.HUD", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatPlaybackHUDTest::RunTest(const FString&)
{
    auto* World = GEditor->GetEditorWorldContext().World();
    auto* HUDClass = LoadClass<UCombatLayoutHUDWidget>(nullptr, TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
    auto* HUD = CreateWidget<UCombatLayoutHUDWidget>(World, HUDClass);
    auto* Model = NewObject<UCombatUIModel>(HUD);
    HUD->BindUIModel(Model);
    const auto Slate = HUD->TakeWidget();
    HUD->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    auto* SpeedWidget = HUD->GetPlaybackWidgetForTest();
    if (!TestNotNull(TEXT("Speed control exists in actual combat HUD"), SpeedWidget)) return false;
    int32 Clicks = 0;
    Model->OnCyclePlaybackSpeed.AddLambda([&] { ++Clicks; Model->SetPlaybackSpeed(CombatPlaybackSpeed::Next(Model->GetPlaybackSpeed()),true); });
    Model->SetPlaybackSpeed(1,true);
    auto* Button = SpeedWidget->GetSpeedButton();
    if (!TestNotNull(TEXT("Speed button initialized without player context"), Button)) return false;
    for (int32 Expected : {2,3,4,5,6,7,8,1})
    {
        Button->OnClicked.Broadcast();
        TestEqual(TEXT("Button sends cycle intent 1 through 8 then 1"), Model->GetPlaybackSpeed(), Expected);
    }
    TestEqual(TEXT("One intent per click"), Clicks, 8);
    const FString Dir = FPaths::ProjectSavedDir()/TEXT("UI/CombatPlayback");
    IFileManager::Get().MakeDirectory(*Dir,true);
    for (FVector2D Size : {FVector2D(1400,1165), FVector2D(1920,1080)})
    {
        Model->SetPlaybackSpeed(8,true);
        FTextureCompilingManager::Get().FinishAllCompilation();
        FWidgetRenderer Renderer(true,true); Renderer.SetIsPrepassNeeded(true);
        for(int I=0; I<4; ++I) { Renderer.DrawWidget(Slate,Size); HUD->PositionPlaybackButtonForTest(); }
        auto* Target = Renderer.DrawWidget(Slate,Size); FlushRenderingCommands();
        const auto& G = SpeedWidget->GetCachedGeometry();
        const FVector2D Pos=G.GetAbsolutePosition(), S=G.GetLocalSize();
        TestTrue(TEXT("Speed control fits viewport"), Pos.X>=0 && Pos.Y>=0 && Pos.X+S.X<=Size.X+1 && Pos.Y+S.Y<=Size.Y+1);
        TestTrue(TEXT("Speed control has a visible touch target"), S.X>=100 && S.Y>=56);
        TestTrue(TEXT("Speed button cannot tap the board underneath"), !HUD->IsGuidedBoardInputAt(G.LocalToAbsolute(S*.5f)));
        FBufferArchive Png; FImageUtils::ExportRenderTarget2DAsPNG(Target,Png);
        FFileHelper::SaveArrayToFile(Png,*(Dir/FString::Printf(TEXT("speed8-%.0fx%.0f.png"),Size.X,Size.Y)));
    }
    Model->SetPlaybackSpeed(8,false);
    TestFalse(TEXT("Unavailable outside active combat"), Button->GetIsEnabled());
    Model->OnCyclePlaybackSpeed.Clear(); HUD->RemoveFromParent();
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatPlaybackLifecycleTest, "P_RD.Combat.Playback.ControllerLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatPlaybackLifecycleTest::RunTest(const FString&)
{
    auto* World = GEditor->GetEditorWorldContext().World();
    auto* Controller = World->SpawnActor<APlayerController>();
    auto* Camera = World->SpawnActor<ACombatCameraPawn>();
    World->AddController(Controller);
    Controller->Possess(Camera);
    auto* Owner = World->SpawnActor<AActor>();
    auto* Playback = NewObject<UCombatPlaybackComponent>(Owner);
    Playback->RegisterComponent();
    Playback->BeginPlay();
    auto* Model = NewObject<UCombatUIModel>(Owner);
    Playback->StartPlayback(Model);
    TestTrue(TEXT("Playback connects to current camera"), Model->IsPlaybackSpeedAvailable());
    for (int32 Expected : {2,3,4,5,6,7,8,1,2})
    {
        Model->RequestCyclePlaybackSpeed();
        TestEqual(TEXT("Controller applies speed"), World->GetWorldSettings()->TimeDilation, float(Expected));
        TestEqual(TEXT("Model reflects applied selection"), Model->GetPlaybackSpeed(), Expected);
    }
    for(int32 I=0; I<20; ++I) Playback->TickComponent(.016f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Ticks never stack the persistent multiplier"), World->GetWorldSettings()->TimeDilation,2.f);
    Playback->StopPlayback();
    TestEqual(TEXT("Combat completion resets playback"), World->GetWorldSettings()->TimeDilation,1.f);
    TestFalse(TEXT("Result HUD cannot change speed"), Model->IsPlaybackSpeedAvailable());
    Model->RequestCyclePlaybackSpeed();
    TestEqual(TEXT("Stopped model has no active callback"), World->GetWorldSettings()->TimeDilation,1.f);
    Playback->StartPlayback(Model);
    TestEqual(TEXT("Next combat restores selected speed"), World->GetWorldSettings()->TimeDilation,2.f);
    Playback->EndPlay(EEndPlayReason::LevelTransition);
    TestEqual(TEXT("Abandon or level transition cleans up playback"), World->GetWorldSettings()->TimeDilation,1.f);
    Playback->DestroyComponent(); Owner->Destroy(); Controller->UnPossess(); Camera->Destroy(); Controller->Destroy();
    return !HasAnyErrors();
}
