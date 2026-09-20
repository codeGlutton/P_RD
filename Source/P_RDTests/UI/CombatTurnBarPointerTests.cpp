#include "Misc/AutomationTest.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/MobileInputTestsHelper.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Editor.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBox.h"
#include "RenderingThread.h"
#include "TimerManager.h"
#include "Input/HittestGrid.h"
#include "Layout/WidgetPath.h"
#include "Widgets/SVirtualWindow.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatTurnBarPointerTest,
	"P_RD.UI.CombatHUD.TurnBarPointerGestures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatTurnBarPointerTest::RunTest(const FString&)
{
	auto* World = GEditor->GetEditorWorldContext().World();
	auto* Class = LoadClass<UCombatLayoutHUDWidget>(nullptr,
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
	auto* HUD = Class ? CreateWidget<UCombatLayoutHUDWidget>(World, Class) : nullptr;
	if (!TestNotNull(TEXT("Production HUD"), HUD)) return false;
	const TSharedRef<SWidget> SlateHUD = HUD->TakeWidget();
	auto* Model = NewObject<UCombatUIModel>(HUD);
	auto* Listener = NewObject<URDMobileInputTestListener>(HUD);
	Model->OnCombatCommand.AddDynamic(Listener, &URDMobileInputTestListener::Command);
	Model->OnCombatWorldTouch.AddDynamic(Listener, &URDMobileInputTestListener::WorldTouch);
	HUD->BindUIModel(Model);
	TArray<FUnitUI> Units;
	FTurnUI Turn;
	Turn.mCurrentUnitId = 100;
	Turn.mRound = 1;
	for (int32 Index = 0; Index < 32; ++Index)
	{
		auto& Unit = Units.AddDefaulted_GetRef();
		Unit.mUnitId = 100 + Index;
		Unit.mIsPlayer = Index % 2 == 0;
		Unit.mName = FText::FromString(FString::FromInt(Unit.mUnitId));
		Turn.mTurnOrderUnitIds.Add(Unit.mUnitId);
	}
	Model->SetUnitUIs(Units);
	Model->SetTurnUI(Turn);
	auto* Button = Cast<UButton>(HUD->WidgetTree->FindWidget(TEXT("TurnTokenButton_1")));
	if (!TestNotNull(TEXT("Real turn token button"), Button)
		|| !TestTrue(TEXT("Input observer registered"), HUD->mBoardPointerObserver.IsValid())) return false;
	auto& App = FSlateApplication::Get();
	auto Observer = HUD->mBoardPointerObserver;
	FWidgetRenderer Renderer(true, true);
	const TSharedRef<SWidget> Root = SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
		[SNew(SBox).WidthOverride(1920).HeightOverride(1080)[SlateHUD]];
	const TSharedRef<SVirtualWindow> Window = SNew(SVirtualWindow).Size(FVector2D(1920, 1080));
	Window->SetContent(Root);
	App.RegisterVirtualWindow(Window);
	for (const FVector2D Size : {FVector2D(1280, 720), FVector2D(2400, 1080), FVector2D(2560, 1920)})
	{
		Window->Resize(Size);
		FHittestGrid Grid;
		auto* RenderTarget = FWidgetRenderer::CreateTargetFor(Size, TF_Bilinear, true);
		Renderer.DrawWindow(RenderTarget, Grid, Window, 1.f, Size, 0.f);
		FlushRenderingCommands();
		TestTrue(TEXT("Turn button has real arranged geometry"), Button->GetCachedGeometry().GetLocalSize().X > 0);
		for (bool Touch : {true, false})
		{
			HUD->mTurnWindowStart = 0;
			HUD->mTurnSwipeConsumed = false;
			HUD->RefreshTurnOrder();
			const auto Geometry = HUD->mTurnPanel->GetCachedGeometry();
			const FVector2D Start = Button->GetCachedGeometry().GetAbsolutePosition()
				+ Button->GetCachedGeometry().GetAbsoluteSize() * .5;
			const FVector2D LocalStart = Geometry.AbsoluteToLocal(Start);
			auto Position = [&](double X, double Y = 0.) { return Geometry.LocalToAbsolute(LocalStart + FVector2D(X, Y)); };
			auto Event = [&](FVector2D P, uint32 Pointer = 0)
			{
				if (Touch) return FPointerEvent(uint32(0), Pointer, P, P, 1.f, true);
				return FPointerEvent(0, P, P, TSet<FKey>{EKeys::LeftMouseButton}, EKeys::LeftMouseButton, 0, FModifierKeysState());
			};
			auto Down = [&](FVector2D P) { Observer->HandleMouseButtonDownEvent(App, Event(P)); };
			auto Move = [&](FVector2D P) { Observer->HandleMouseMoveEvent(App, Event(P)); };
			auto Up = [&](FVector2D P) { Observer->HandleMouseButtonUpEvent(App, Event(P)); };
			const int32 Before = Listener->FocusRequests;
			Down(Start); Move(Position(-60)); Move(Position(-120)); Up(Position(-120));
			TestEqual(TEXT("One drag advances one page even with repeated move events"), HUD->mTurnWindowStart, 10);
			Button->OnClicked.Broadcast();
			TestEqual(TEXT("Child release after swipe never focuses the new occupant"), Listener->FocusRequests, Before);
			Down(Start); Up(Start); Button->OnClicked.Broadcast();
			TestEqual(TEXT("Fresh tap immediately after swipe focuses exactly once"), Listener->FocusRequests, Before + 1);
			TestEqual(TEXT("Focus uses the unit displayed on the new page"), Listener->FocusUnit, 111);
			TestEqual(TEXT("Enemy summary follows the tapped unit"), Model->GetTarget().mUnitId, 111);
			Down(Start); Up(Position(60)); Button->OnClicked.Broadcast();
			TestEqual(TEXT("Fast reverse swipe works without any move event"), HUD->mTurnWindowStart, 0);
			TestEqual(TEXT("Fast swipe is not a camera tap"), Listener->FocusRequests, Before + 1);
			Down(Start); Move(Position(0, 100)); Move(Start); Up(Start); Button->OnClicked.Broadcast();
			TestEqual(TEXT("Vertical drag returning to origin is not a tap"), Listener->FocusRequests, Before + 1);
			TestEqual(TEXT("Vertical drag does not page"), HUD->mTurnWindowStart, 0);
			// Swipe release outside the button generates no child click; next tap must still work.
			Down(Start); Move(Position(-60)); Up(Position(-200));
			Down(Start); Up(Start); Button->OnClicked.Broadcast();
			TestEqual(TEXT("Unclicked swipe release cannot swallow the next tap"), Listener->FocusRequests, Before + 2);
			if (Touch)
			{
				const int32 Page = HUD->mTurnWindowStart;
				Down(Start);
				Observer->HandleMouseButtonDownEvent(App, Event(Position(100), 1));
				Move(Position(-60));
				Observer->HandleMouseButtonUpEvent(App, Event(Position(100), 1));
				Up(Start); Button->OnClicked.Broadcast();
				TestEqual(TEXT("Second finger cancels paging"), HUD->mTurnWindowStart, Page);
				TestEqual(TEXT("Second finger cancels camera focus"), Listener->FocusRequests, Before + 2);
			}
		}
		// Route actual Slate touch events through the rendered hit-test path. This
		// catches invisible overlays/button modes that broadcasting OnClicked cannot.
		HUD->mTurnSwipeConsumed = false;
		HUD->mTurnWindowStart = 0;
		HUD->RefreshTurnOrder();
		Renderer.DrawWindow(RenderTarget, Grid, Window, 1.f, Size, 0.f);
		FlushRenderingCommands();
		const FVector2D Tap = Button->GetCachedGeometry().GetAbsolutePosition()
			+ Button->GetCachedGeometry().GetAbsoluteSize() * .5;
		auto Widgets = Grid.GetBubblePath(Tap, 0, false);
		FWidgetPath Path(Widgets);
		if (TestTrue(TEXT("Portrait hit path reaches its real button"),
			Path.ContainsWidget(Button->GetCachedWidget().Get())))
		{
			const FPointerEvent Touch(uint32(0), uint32(0), Tap, Tap, 1.f, true);
			const int32 Before = Listener->FocusRequests;
			Observer->HandleMouseButtonDownEvent(App, Touch);
			App.RoutePointerDownEvent(Path, Touch);
			TestEqual(TEXT("Touch down does not focus before swipe can be distinguished"), Listener->FocusRequests, Before);
			Observer->HandleMouseButtonUpEvent(App, Touch);
			App.RoutePointerUpEvent(Path, Touch);
			TestEqual(TEXT("Actual Slate touch release dispatches camera focus once"), Listener->FocusRequests, Before + 1);
			TestEqual(TEXT("Actual touch focuses the portrait's unit"), Listener->FocusUnit, 101);
			Observer->HandleMouseButtonDownEvent(App, Touch);
			App.RoutePointerDownEvent(Path, Touch);
			const auto BarGeometry = HUD->mTurnPanel->GetCachedGeometry();
			const FVector2D SwipeEnd = BarGeometry.LocalToAbsolute(
				BarGeometry.AbsoluteToLocal(Tap) - FVector2D(50, 0));
			const FPointerEvent Swipe(uint32(0), uint32(0), SwipeEnd, Tap, 1.f, true);
			Observer->HandleMouseMoveEvent(App, Swipe);
			App.RoutePointerMoveEvent(Path, Swipe, false);
			Observer->HandleMouseButtonUpEvent(App, Swipe);
			App.RoutePointerUpEvent(Path, Swipe);
			TestEqual(TEXT("Captured Slate touch swipe pages once"), HUD->mTurnWindowStart, 10);
			TestEqual(TEXT("Captured child button release cannot focus after swipe"), Listener->FocusRequests, Before + 1);
			Observer->HandleMouseButtonDownEvent(App, Touch);
			App.RoutePointerDownEvent(Path, Touch);
			Observer->HandleMouseButtonUpEvent(App, Touch);
			App.RoutePointerUpEvent(Path, Touch);
			TestEqual(TEXT("Real tap immediately after real swipe is restored"), Listener->FocusRequests, Before + 2);
			TestEqual(TEXT("Real tap after paging focuses the new portrait"), Listener->FocusUnit, 111);
		}
	}
	App.UnregisterVirtualWindow(Window);
	TestEqual(TEXT("Turn gestures never dispatch board commands"), Listener->Taps, 0);
	World->GetTimerManager().ClearAllTimersForObject(HUD);
	HUD->NativeDestruct();
	return !HasAnyErrors();
}
#endif
