#include "Misc/AutomationTest.h"
#include "UI/MobileInputTestsHelper.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/RDHUDScalingRule.h"
#include "UI/RDUIInputSubsystem.h"
#include "UI/TitleMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/SafeZone.h"
#include "Editor.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Input/Events.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "TimerManager.h"
#include "UI/SCenteredSafeZone.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/ArrangedChildren.h"
#include "Misc/CoreDelegates.h"
#include "Widgets/Layout/SSpacer.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatBoardMultitouchTest, "P_RD.UI.Mobile.BoardPointerSessions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatBoardMultitouchTest::RunTest(const FString&)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	auto* HUD = CreateWidget<UCombatLayoutHUDWidget>(World);
	if (!TestNotNull(TEXT("Board HUD"), HUD)) return false;
	auto* Model = NewObject<UCombatUIModel>();
	auto* Listener = NewObject<URDMobileInputTestListener>();
	Model->OnCombatWorldTouch.AddDynamic(Listener, &URDMobileInputTestListener::WorldTouch);
	FTurnUI Turn;
	Turn.mPhase = ECombatBuildPhaseUI::Preview;
	Model->SetTurnUI(Turn);
	HUD->mUIModel = Model;
	const FGeometry Geometry;
	auto Touch = [](uint32 Id, double X) { return FPointerEvent(uint32(0), Id, FVector2D(X, 100), FVector2D(X, 100), 1.f, true); };
	auto Begin = [&](uint32 Id, double X) { HUD->NativeOnTouchStarted(Geometry, Touch(Id, X)); };
	auto End = [&](uint32 Id, double X) { HUD->NativeOnTouchEnded(Geometry, Touch(Id, X)); };
	Begin(0, 100); Begin(1, 300);
	HUD->HandleBoardLongPress();
	End(1, 300); End(0, 100);
	TestEqual(TEXT("Two contacts never make a tile tap"), Listener->Taps, 0);
	TestEqual(TEXT("Second contact cancels the pending hold"), Listener->Holds, 0);
	Begin(3, 100); End(3, 100);
	TestEqual(TEXT("A fresh single contact works after both fingers lift"), Listener->Taps, 1);
	Begin(0, 100);
	// This contact is consumed by a child button: only the preprocessor sees it.
	HUD->ObserveBoardTouchDown(1);
	HUD->ObserveBoardTouchUp(1);
	HUD->HandleBoardLongPress();
	End(0, 100);
	TestEqual(TEXT("A second finger on UI also consumes the board gesture"), Listener->Taps, 1);
	TestEqual(TEXT("Lifting the secondary finger cannot revive a hold"), Listener->Holds, 0);
	Begin(0, 100);
	HUD->NativeOnTouchMoved(Geometry, Touch(0, 110));
	End(0, 110);
	TestEqual(TEXT("Touch jitter below the shared pan threshold remains a tap"), Listener->Taps, 2);
	Begin(0, 100);
	HUD->NativeOnTouchMoved(Geometry, Touch(0, 130));
	HUD->NativeOnTouchMoved(Geometry, Touch(0, 100));
	End(0, 100);
	TestEqual(TEXT("A drag returning to its origin cannot become a tap"), Listener->Taps, 2);
	const TSet<FKey> Buttons{EKeys::LeftMouseButton};
	auto Mouse = [&](double X) { return FPointerEvent(0, FVector2D(X, 100), FVector2D(X, 100), Buttons, EKeys::LeftMouseButton, 0, FModifierKeysState()); };
	HUD->NativeOnMouseButtonDown(Geometry, Mouse(100));
	HUD->NativeOnMouseMove(Geometry, Mouse(104));
	HUD->NativeOnMouseButtonUp(Geometry, Mouse(104));
	TestEqual(TEXT("Desktop pan and board tap use the same 3px threshold"), Listener->Taps, 2);
	Begin(0, 100);
	HUD->HandleBoardLongPress();
	End(0, 100);
	TestEqual(TEXT("Ordinary single-finger hold remains available"), Listener->Holds, 1);
	TestEqual(TEXT("A hold release is not also a tap"), Listener->Taps, 2);
	World->GetTimerManager().ClearAllTimersForObject(HUD);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatSettingsModalGateTest, "P_RD.UI.Mobile.SettingsBlocksWorldGestures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatSettingsModalGateTest::RunTest(const FString&)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	auto* Widgets = World->GetSubsystem<UWorldWidgetSubsystem>();
	UUserWidget* Previous = Widgets->GetWorldWidget<UUserWidget>(EWorldWidgetType::InGameSettings);
	auto* Settings = CreateWidget<URDMobileSettingsTestWidget>(World);
	auto* HUD = CreateWidget<UCombatLayoutHUDWidget>(World);
	Widgets->SetWorldWidgetForTest(EWorldWidgetType::InGameSettings, Settings);
	Settings->TestOpened = true;
	TestTrue(TEXT("Settings participates in the same world gesture gate as details"), HUD->IsWorldInputModalShown());
	Settings->TestOpened = false;
	TestFalse(TEXT("Closing settings releases the modal gate"), HUD->IsWorldInputModalShown());
	HUD->mCombatResultFlowActive = true;
	TestTrue(TEXT("Result flow keeps world gestures blocked"), HUD->IsWorldInputModalShown());
	Widgets->SetWorldWidgetForTest(EWorldWidgetType::InGameSettings, Previous);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRDMobileSafeAreaAndDPITest, "P_RD.UI.Mobile.SafeAreaAndResolutionScaling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRDMobileSafeAreaAndDPITest::RunTest(const FString&)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	auto* Settings = CreateWidget<USettingsPanelWidget>(World);
	auto* Title = CreateWidget<UTitleMenuWidget>(World);
	TestTrue(TEXT("Top-level settings receives a safe area"), Settings->ShouldWrapMobileSafeArea());
	TestFalse(TEXT("Title keeps its authored SafeZone without a second wrapper"), Title->ShouldWrapMobileSafeArea());
	auto* ParentCanvas = NewObject<UCanvasPanel>();
	ParentCanvas->AddChild(Settings);
	TestFalse(TEXT("Nested opt-in widgets do not apply the viewport inset again"), Settings->ShouldWrapMobileSafeArea());
	ParentCanvas->RemoveChild(Settings);
	TestTrue(TEXT("Standalone placement restores safe-area eligibility"), Settings->ShouldWrapMobileSafeArea());
	if (!Settings->WidgetTree) Settings->WidgetTree = NewObject<UWidgetTree>(Settings);
	Settings->WidgetTree->RootWidget = Settings->WidgetTree->ConstructWidget<USafeZone>();
	TestFalse(TEXT("An already safe root is not padded twice"), Settings->ShouldWrapMobileSafeArea());
	const auto* Rule = GetDefault<URDHUDScalingRule>();
	auto RelativeHeight = [&](FIntPoint Size) { return Rule->GetDPIScaleBasedOnSize(Size) * 100.f / Size.Y; };
	TestTrue(TEXT("20:9 physical proportions agree at FHD and QHD"), FMath::IsNearlyEqual(RelativeHeight({2400,1080}), RelativeHeight({3200,1440})));
	TestTrue(TEXT("4:3 tablet proportions agree at both resolutions"), FMath::IsNearlyEqual(RelativeHeight({1440,1080}), RelativeHeight({2560,1920})));
	for (FIntPoint Size : {FIntPoint(1280,720), FIntPoint(2400,1080), FIntPoint(1296,1080), FIntPoint(2560,1920)})
	{
		const float Scale = Rule->GetDPIScaleBasedOnSize(Size);
		TestTrue(TEXT("Design fits both axes on phone/fold/tablet"), 1920.f * Scale <= Size.X + .01f && 1080.f * Scale <= Size.Y + .01f);
	}
	TestEqual(TEXT("Invalid viewport has a finite neutral scale"), Rule->GetDPIScaleBasedOnSize({0,0}), 1.f);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRDCenteredNotchTest, "P_RD.UI.Mobile.CenteredNotchLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRDCenteredNotchTest::RunTest(const FString&)
{
	auto& App = FSlateApplication::Get();
	const bool HadCustom = App.IsCustomSafeZoneSet();
	const FMargin Previous = App.GetCustomSafeZone();
	const auto GlobalScale = SSafeZone::GetGlobalSafeZoneScale();
	SSafeZone::SetGlobalSafeZoneScale(TOptional<float>());
	int32 Cases = 0;
	for (const FVector2D Pixels : {FVector2D(1920,1080), FVector2D(2400,1080), FVector2D(3200,1440)})
	for (const float Dpi : {1.f, 1.5f, 2.f})
	{
		const auto Child = SNew(SSpacer).Size(FVector2D(100, 50));
		const auto Safe = SNew(SCenteredSafeZone).OverrideScreenSize(Pixels)[Child];
		const FGeometry Viewport = FGeometry::MakeRoot(Pixels / Dpi, FSlateLayoutTransform(Dpi, FVector2D(37,19)));
		// Reuse the same widget as the device rotates or system bars change.
		for (const FMargin Inset : {FMargin(96,0,0,0), FMargin(0,0,96,0),
			FMargin(96,0,24,48), FMargin(0), FMargin(48,24,48,24)})
		{
			App.SetCustomSafeZone(FMargin(2*Inset.Left/Pixels.X, 2*Inset.Top/Pixels.Y,
				2*Inset.Right/Pixels.X, 2*Inset.Bottom/Pixels.Y));
			FCoreDelegates::OnSafeFrameChangedEvent.Broadcast();
			Safe->SlatePrepass(Dpi);
			FArrangedChildren Arranged(EVisibility::Visible);
			Safe->ArrangeChildren(Viewport, Arranged);
			if (!TestEqual(TEXT("One safe content root"), Arranged.Num(), 1)) continue;
			const FGeometry& Content = Arranged[0].Geometry;
			const FVector2D Origin = Viewport.AbsoluteToLocal(Content.LocalToAbsolute(FVector2D::ZeroVector)) * Dpi;
			const FVector2D Extent = Content.GetLocalSize() * Dpi;
			TestTrue(TEXT("HUD center stays at viewport center with either notch orientation"),
				(Origin + Extent*.5).Equals(Pixels*.5, .1));
			TestTrue(TEXT("All four physical unsafe edges are excluded"), Origin.X >= Inset.Left-1 && Origin.Y >= Inset.Top-1
				&& Origin.X+Extent.X <= Pixels.X-Inset.Right+1 && Origin.Y+Extent.Y <= Pixels.Y-Inset.Bottom+1);
			if (Inset == FMargin(0))
				TestTrue(TEXT("No cutout leaves the full viewport available"), Origin.IsNearlyZero() && Extent.Equals(Pixels,.1));
			++Cases;
		}
	}
	if (HadCustom) App.SetCustomSafeZone(Previous); else App.ResetCustomSafeZone();
	SSafeZone::SetGlobalSafeZoneScale(GlobalScale);
	AddInfo(FString::Printf(TEXT("Checked %d live safe-area arrangements across resolutions, DPI and notch rotation."), Cases));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRDMobileBackNavigationTest, "P_RD.UI.Mobile.BackClosesInnermostConfirmation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRDMobileBackNavigationTest::RunTest(const FString&)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	auto* Settings = CreateWidget<USettingsPanelWidget>(World);
	auto* Listener = NewObject<URDMobileInputTestListener>();
	Settings->OnBackRequested.AddDynamic(Listener, &URDMobileInputTestListener::Back);
	Settings->AbandonConfirmPanel = NewObject<UCanvasPanel>(Settings);
	Settings->ShowAbandonConfirm();
	TestTrue(TEXT("System Back consumed by confirmation"), Settings->HandleBackNavigation());
	TestFalse(TEXT("Back dismisses the confirmation"), Settings->AbandonConfirmPanel->IsVisible());
	TestEqual(TEXT("Confirmation Back does not navigate the settings screen"), Listener->Backs, 0);
	Settings->HandleBackNavigation();
	TestEqual(TEXT("Next Back performs the ordinary settings Back action once"), Listener->Backs, 1);
	TestTrue(TEXT("Android hardware/gesture Back recognized"), URDUIInputSubsystem::IsBackKey(EKeys::Android_Back));
	TestTrue(TEXT("Desktop Escape recognized"), URDUIInputSubsystem::IsBackKey(EKeys::Escape));
	TestFalse(TEXT("Ordinary editing keys not intercepted"), URDUIInputSubsystem::IsBackKey(EKeys::BackSpace));
	return !HasAnyErrors();
}
#endif
