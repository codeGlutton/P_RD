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
	First.CurTouchPos.X += 30;
	Pawn->UpdatePointerGestures(First, Released, true, FVector2D(1100, 1000));
	TestEqual(TEXT("Touch takes precedence over emulated mouse"), Drags, 4);
	TestEqual(TEXT("Touch delta preserved"), LastDelta, FVector2D(30, 0));
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatCameraSlowPinchTest,
	"P_RD.Camera.PointerInput.SlowPinchAndContactTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatCameraSlowPinchTest::RunTest(const FString&)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	ACombatCameraPawn* Pawn = World->SpawnActor<ACombatCameraPawn>();
	if (!TestNotNull(TEXT("Camera pawn"), Pawn)) return false;
	int32 Pinches = 0, Drags = 0;
	float ZoomDistance = 0.f;
	Pawn->OnPinching.AddLambda([&](const TArray<FTouchState>& States)
	{
		++Pinches;
		ZoomDistance += FVector2D::Distance(States[0].CurTouchPos, States[1].CurTouchPos)
			- FVector2D::Distance(States[0].PreTouchPos, States[1].PreTouchPos);
	});
	Pawn->OnDragging.AddLambda([&](const TArray<FTouchState>&) { ++Drags; });
	const FTouchState Released;
	auto Replay = [&](int32 FPS)
	{
		Pawn->UpdatePointerGestures(Released, Released, false, FVector2D::ZeroVector);
		FTouchState A, B;
		A.bIsCurrentlyPressed = B.bIsCurrentlyPressed = true;
		A.CurTouchPos = FVector2D(100, 100); B.CurTouchPos = FVector2D(200, 100);
		ZoomDistance = 0;
		Pawn->UpdatePointerGestures(A, B, false, FVector2D::ZeroVector);
		for (int32 Frame = 0; Frame < FPS; ++Frame)
		{
			B.CurTouchPos.X += 120.0 / FPS;
			Pawn->UpdatePointerGestures(A, B, false, FVector2D::ZeroVector);
		}
		return ZoomDistance;
	};
	const float Distance30 = Replay(30);
	const float Distance60 = Replay(60);
	TestTrue(TEXT("Slow 2px/frame pinch zooms instead of being discarded"), Distance60 > 110.f);
	TestTrue(TEXT("30/60 FPS differ only by the initial dead zone"), FMath::Abs(Distance30 - Distance60) <= 3.f);
	FTouchState First;
	First.bIsCurrentlyPressed = true; First.CurTouchPos = FVector2D(150, 100);
	Pawn->UpdatePointerGestures(First, Released, false, FVector2D::ZeroVector);
	First.CurTouchPos.X += 80;
	Pawn->UpdatePointerGestures(First, Released, false, FVector2D::ZeroVector);
	TestEqual(TEXT("Remaining pinch finger does not start a new pan"), Drags, 0);
	Pawn->UpdatePointerGestures(Released, Released, false, FVector2D::ZeroVector);
	Pawn->UpdatePointerGestures(First, Released, false, FVector2D::ZeroVector);
	First.CurTouchPos.X += 10;
	Pawn->UpdatePointerGestures(First, Released, false, FVector2D::ZeroVector);
	TestEqual(TEXT("Touch tap slop does not move the camera"), Drags, 0);
	First.CurTouchPos.X += 20;
	Pawn->UpdatePointerGestures(First, Released, false, FVector2D::ZeroVector);
	TestEqual(TEXT("New single touch pans after the board threshold"), Drags, 1);
	First.CurTouchPos.X -= 30;
	Pawn->UpdatePointerGestures(First, Released, false, FVector2D::ZeroVector);
	TestEqual(TEXT("Active pan continues when returning to its origin"), Drags, 2);
	Pawn->SetTouchGestureInputEnabled(false);
	TestFalse(TEXT("Modal disables the camera"), Pawn->IsTouchGestureInputEnabled());
	Pawn->Destroy();
	return !HasAnyErrors();
}
#endif
