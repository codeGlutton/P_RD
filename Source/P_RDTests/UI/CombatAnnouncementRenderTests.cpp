#include "Misc/AutomationTest.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Editor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/SOverlay.h"
#include "Input/Events.h"
#include "TimerManager.h"
#include "RHI.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatAnnouncementRenderTest,
    "P_RD.UI.CombatHUD.AnnouncementBackground",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatAnnouncementRenderTest::RunTest(const FString&)
{
    if (GUsingNullRHI) { AddError(TEXT("Requires a real desktop renderer.")); return false; }
    UWorld* World = GEditor->GetEditorWorldContext().World();
    UClass* Class = LoadClass<UCombatLayoutHUDWidget>(nullptr,
        TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
    if (!TestNotNull(TEXT("Combat HUD class"), Class)) return false;
    auto* HUD = CreateWidget<UCombatLayoutHUDWidget>(World, Class);
    auto* Model = NewObject<UCombatUIModel>(HUD);
    HUD->BindUIModel(Model);
    // Bind before construction so the editor's automatic combat preview stays off.
    const TSharedRef<SWidget> SlateHUD = HUD->TakeWidget();
    FUnitUI Player;
    Player.mUnitId = 10;
    Player.mIsPlayer = true;
    Model->SetUnitUIs({Player});
    FTurnUI Turn;
    Turn.mCurrentUnitId = Player.mUnitId;
    Model->SetTurnUI(Turn);
    auto* Band = Cast<UBorder>(HUD->GetWidgetFromName(TEXT("CombatAnnouncementRoot")));
    auto* Blocker = Cast<UButton>(HUD->GetWidgetFromName(TEXT("CombatAnnouncementInputBlocker")));
    if (!TestNotNull(TEXT("Band"), Band) || !TestNotNull(TEXT("Input blocker"), Blocker)) return false;

    FWidgetRenderer Renderer(true, true);
    const FVector2D Size(640, 360);
    auto Capture = [&](TSharedRef<SWidget> Widget)
    {
        auto Root = SNew(SOverlay)
            + SOverlay::Slot()[SNew(SColorBlock).Color(FLinearColor(.12f, .24f, .08f, 1.f))]
            + SOverlay::Slot()[Widget];
        for (int32 Pass = 0; Pass < 2; ++Pass)
        {
            Renderer.DrawWidget(Root, Size);
            FlushRenderingCommands();
        }
        UTextureRenderTarget2D* Target = Renderer.DrawWidget(Root, Size);
        FlushRenderingCommands();
        TArray<FColor> Pixels;
        FReadSurfaceDataFlags Flags(RCM_UNorm); Flags.SetLinearToGamma(false);
        if (Target) Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags);
        return Pixels;
    };
    const TSharedRef<SWidget> SlateBlocker = Blocker->TakeWidget();
    Blocker->SetVisibility(ESlateVisibility::Collapsed);
    const TArray<FColor> Baseline = Capture(SlateBlocker);
    if (!TestEqual(TEXT("Rendered pixels"), Baseline.Num(), 640 * 360)) return false;
    Blocker->SetVisibility(ESlateVisibility::Visible);
    const FGeometry Geometry = FGeometry::MakeRoot(Size, FSlateLayoutTransform());
    const FPointerEvent Hover(0, FVector2D(320,180), FVector2D(319,180),
        TSet<FKey>(), EKeys::Invalid, 0.f, FModifierKeysState());
    const FPointerEvent Press(0, FVector2D(320,180), FVector2D(320,180),
        TSet<FKey>{EKeys::LeftMouseButton}, EKeys::LeftMouseButton, 0.f, FModifierKeysState());
    for (int32 State = 0; State < 4; ++State)
    {
        if (State == 1) SlateBlocker->OnMouseEnter(Geometry, Hover);
        if (State == 2) SlateBlocker->OnMouseButtonDown(Geometry, Press);
        if (State == 3)
        {
            SlateBlocker->OnMouseButtonUp(Geometry, Press);
            SlateBlocker->OnMouseLeave(Hover);
        }
        const TArray<FColor> Pixels = Capture(SlateBlocker);
        TestTrue(*FString::Printf(TEXT("Input blocker does not wash out screen in mouse state %d"), State), Pixels == Baseline);
    }
    Blocker->SetVisibility(ESlateVisibility::Collapsed);

    TArray<TSharedRef<bool>> Releases;
    for (int32 Kind = 0; Kind < 3; ++Kind)
    {
        const auto Released = MakeShared<bool>(false);
        Releases.Add(Released);
        auto Barrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateLambda([Released]{ *Released = true; }));
        if (Kind == 0) Model->OnBeginCombat.Broadcast(Barrier);
        if (Kind == 1) Model->OnBeginAnyRound.Broadcast(Barrier);
        if (Kind == 2) Model->OnBeginAnyTurn.Broadcast(Barrier);
        if (Kind == 2) TestEqual(TEXT("Skills remain hidden during turn announcement"),
            HUD->GetWidgetFromName(TEXT("CommandCard_0"))->GetVisibility(), ESlateVisibility::Collapsed);
        Barrier.Reset();
        SlateHUD->Tick(Geometry, 1.0, .25f);
        TestEqual(TEXT("Announcement visible"), Band->GetVisibility(), ESlateVisibility::HitTestInvisible);
        TestTrue(TEXT("Black band retained for combat, round and turn"), Band->GetBrushColor().A > .7f);
        const auto Pixels = Capture(Band->TakeWidget());
        if (Pixels.Num() == Baseline.Num())
        {
            const int32 Sample = 180 * 640 + 20;
            TestTrue(TEXT("Banner darkens its background"), Pixels[Sample].G + 20 < Baseline[Sample].G);
        }
        else AddError(TEXT("Banner capture failed"));
    }
    World->GetTimerManager().Tick(2.f);
    HUD->AddToRoot();
    const uint64 Frame = GFrameCounter;
    ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, World, HUD, SlateHUD, Blocker, Releases, Frame]
    {
        if (GFrameCounter == Frame) return false;
        World->GetTimerManager().Tick(2.f);
        for (const auto& Released : Releases) TestTrue(TEXT("Announcement releases gameplay barrier"), *Released);
        TestEqual(TEXT("Input blocker hidden after announcement"), Blocker->GetVisibility(), ESlateVisibility::Collapsed);
        TestEqual(TEXT("Current mercenary skills open after announcement finishes"),
            HUD->GetWidgetFromName(TEXT("CommandCard_0"))->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
        HUD->RemoveFromRoot();
        return true;
    }));
    return !HasAnyErrors();
}
#endif
