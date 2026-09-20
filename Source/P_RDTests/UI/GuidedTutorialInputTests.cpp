#include "Misc/AutomationTest.h"
#include "Tutorial/TutorialInputGate.h"
#include "Framework/Application/SlateApplication.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuidedInputTest, "P_RD.Tutorial.Guided.InputGate",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGuidedInputTest::RunTest(const FString &)
{
	auto &App = FSlateApplication::Get();
	FTutorialInputGate Gate;
	bool Active = true, Focused = true;
	FVector2D Target(100, 100);
	Gate.IsActive = [&] { return Active; };
	Gate.IsGameFocused = [&] { return Focused; };
	Gate.IsInViewport = [](const FVector2D &P) { return P.X >= 0 && P.Y >= 0 && P.X < 800 && P.Y < 600; };
	Gate.IsAllowed = [&](const FVector2D &P) { return (P - Target).Size() < 20; };
	const TSet<FKey> Buttons{EKeys::LeftMouseButton};
	auto Mouse = [&](FVector2D P, FKey Key = EKeys::LeftMouseButton) {
		return FPointerEvent(0, P, P, Buttons, Key, 0, FModifierKeysState());
	};
	auto Touch = [](uint32 Finger, FVector2D P) { return FPointerEvent(uint32(0), Finger, P, P, 1.f, true); };
	const auto Other = Mouse(FVector2D(400, 400));
	TestTrue(TEXT("Other UI press is consumed"), Gate.HandleMouseButtonDownEvent(App, Other));
	TestTrue(TEXT("Other UI release is consumed"), Gate.HandleMouseButtonUpEvent(App, Other));
	const auto Allowed = Mouse(Target);
	TestFalse(TEXT("Instruction target receives press for click or hold"),
			  Gate.HandleMouseButtonDownEvent(App, Allowed));
	const auto Secondary = Mouse(Target, EKeys::RightMouseButton);
	TestTrue(TEXT("Secondary mouse button blocked during accepted press"),
			 Gate.HandleMouseButtonDownEvent(App, Secondary));
	TestTrue(TEXT("Secondary release cannot release the primary press"), Gate.HandleMouseButtonUpEvent(App, Secondary));
	Target = FVector2D(200, 100);
	TestFalse(TEXT("Accepted hold receives movement even when step changes"), Gate.HandleMouseMoveEvent(App, Other));
	TestFalse(TEXT("Accepted press always receives release after transition"),
			  Gate.HandleMouseButtonUpEvent(App, Allowed));
	TestTrue(TEXT("Previous target cannot be pressed again"), Gate.HandleMouseButtonDownEvent(App, Allowed));
	Active = false;
	TestTrue(TEXT("Rejected press cannot leak release when tutorial ends"),
			 Gate.HandleMouseButtonUpEvent(App, Allowed));
	TestFalse(TEXT("Completed tutorial restores ordinary input"), Gate.HandleMouseButtonDownEvent(App, Other));
	Active = true;
	const auto Finger = Touch(1, Target);
	TestTrue(TEXT("Uses real touch event semantics"), Finger.IsTouchEvent());
	TestFalse(TEXT("Target accepts touch"), Gate.HandleMouseButtonDownEvent(App, Finger));
	const auto Second = Touch(2, Target);
	TestTrue(TEXT("Second finger cannot bypass the active step"), Gate.HandleMouseButtonDownEvent(App, Second));
	TestTrue(TEXT("Second finger release blocked"), Gate.HandleMouseButtonUpEvent(App, Second));
	TestFalse(TEXT("First finger release delivered"), Gate.HandleMouseButtonUpEvent(App, Finger));
	const auto Outside = Mouse(FVector2D(900, 900));
	TestFalse(TEXT("Outside game viewport unaffected"), Gate.HandleMouseButtonDownEvent(App, Outside));
	TestTrue(TEXT("Camera wheel blocked"), Gate.HandleMouseWheelOrGestureEvent(App, Other, nullptr));
	const FKeyEvent Tab(EKeys::Tab, FModifierKeysState(), 0, false, 0, 0);
	TestTrue(TEXT("Keyboard navigation cannot bypass target"), Gate.HandleKeyDownEvent(App, Tab));
	Focused = false;
	TestFalse(TEXT("Other application/editor keyboard focus unaffected"), Gate.HandleKeyDownEvent(App, Tab));
	return true;
}
#endif
