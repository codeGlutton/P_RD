#include "Misc/AutomationTest.h"
#include "Pawn/Camera/CombatCameraPawn.h"
#include "Editor.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatCameraPointerInputTest,
	"P_RD.Camera.PointerInput.MouseTouchAndModal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatCameraPointerInputTest::RunTest(const FString&)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	ACombatCameraPawn* Pawn = World->SpawnActor<ACombatCameraPawn>();
	if (!TestNotNull(TEXT("Camera pawn"), Pawn)) return false;
	int32 Drags = 0;
	int32 Pinches = 0;
	FVector2D LastDelta = FVector2D::ZeroVector;
	Pawn->OnDragging.AddLambda([&](const TArray<FTouchState>& States)
	{
		++Drags;
		LastDelta = States[0].CurTouchPos - States[0].PreTouchPos;
	});
	Pawn->OnPinching.AddLambda([&](const TArray<FTouchState>&) { ++Pinches; });
	const FTouchState Released;
	auto Mouse = [&](bool Pressed, double X)
	{
		Pawn->UpdatePointerGestures(Released, Released, Pressed, FVector2D(X, 100));
	};
	Mouse(true, 100);
	TestEqual(TEXT("Press does not jump"), Drags, 0);
	Mouse(true, 102);
	TestEqual(TEXT("Small click jitter ignored"), Drags, 0);
	Mouse(true, 104);
	TestEqual(TEXT("Slow desktop drag accumulates threshold"), Drags, 1);
	TestEqual(TEXT("Only latest delta is moved"), LastDelta, FVector2D(2, 0));
	Mouse(false, 110);
	TestEqual(TEXT("Release does not move camera"), Drags, 1);
	Mouse(true, 500);
	TestEqual(TEXT("New press resets origin"), Drags, 1);
	Mouse(true, 520);
	TestEqual(TEXT("Second drag works"), Drags, 2);
	Pawn->SetTouchGestureInputEnabled(false);
	Mouse(true, 600);
	TestEqual(TEXT("Modal blocks desktop camera"), Drags, 2);
	Pawn->SetTouchGestureInputEnabled(true);
	Mouse(true, 700);
	TestEqual(TEXT("Modal close does not apply stale delta"), Drags, 2);
	Mouse(true, 720);
	TestEqual(TEXT("Camera works after modal close"), Drags, 3);
	FTouchState First;
	First.bIsCurrentlyPressed = true;
	First.CurTouchPos = FVector2D(10, 10);
	Pawn->UpdatePointerGestures(First, Released, true, FVector2D(1000, 1000));
	TestEqual(TEXT("Mouse to touch switch does not jump"), Drags, 3);
	First.CurTouchPos.X += 20;
	Pawn->UpdatePointerGestures(First, Released, true, FVector2D(1100, 1000));
	TestEqual(TEXT("Touch takes precedence over emulated mouse"), Drags, 4);
	TestEqual(TEXT("Touch delta preserved"), LastDelta, FVector2D(20, 0));
	FTouchState Second = First;
	Second.CurTouchPos.X += 100;
	Pawn->UpdatePointerGestures(First, Second, false, FVector2D::ZeroVector);
	Second.CurTouchPos.X += 20;
	Pawn->UpdatePointerGestures(First, Second, false, FVector2D::ZeroVector);
	TestEqual(TEXT("Two-finger zoom preserved"), Pinches, 1);
	TestEqual(TEXT("Pinch does not also drag"), Drags, 4);
	Pawn->Destroy();
	return !HasAnyErrors();
}
#endif
