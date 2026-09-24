#include "Misc/AutomationTest.h"
#include "UI/RealTimeLongPressTimer.h"
#include "UI/Reward/RewardUIModel.h"
#include "HAL/PlatformProcess.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealTimeLongPressTimerTest,
	"P_RD.UI.LongPress.RealTimeClock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealTimeLongPressTimerTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<URewardUIModel> Owner(NewObject<URewardUIModel>());
	FRealTimeLongPressTimer Timer;
	int32 Fired = 0;
	Timer.Start(Owner.Get(), 0.08, FSimpleDelegate::CreateLambda([&Fired]() { ++Fired; }));
	FTSTicker::GetCoreTicker().Tick(4.f); // Simulated fast game ticks must not expire a real-time hold.
	TestEqual(TEXT("Accelerated tick does not trigger long press"), Fired, 0);
	Timer.Cancel();
	FPlatformProcess::Sleep(0.1f);
	FTSTicker::GetCoreTicker().Tick(0.f);
	TestEqual(TEXT("Canceled hold stays canceled"), Fired, 0);
	Timer.Start(Owner.Get(), 0.08, FSimpleDelegate::CreateLambda([&Fired]() { ++Fired; }));
	FPlatformProcess::Sleep(0.1f);
	FTSTicker::GetCoreTicker().Tick(0.f);
	TestEqual(TEXT("Real elapsed time triggers hold once"), Fired, 1);
	TestFalse(TEXT("Expired hold is no longer pending"), Timer.IsPending());
	return true;
}
#endif
