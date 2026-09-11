#include "Misc/AutomationTest.h"
#include "Advertising/EntryAdGate.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntryAdGateTest, "P_RD.Ads.EntryContinuation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEntryAdGateTest::RunTest(const FString&)
{
	FEntryAdGate Gate;
	int32 Entries = 0;
	const auto Callback = [&Entries]() { return FSimpleDelegate::CreateLambda([&Entries]() { ++Entries; }); };
	const int32 First = Gate.Begin(Callback());
	TestTrue(TEXT("First entry starts"), First > 0);
	TestEqual(TEXT("Double tap ignored"), Gate.Begin(Callback()), 0);
	Gate.Complete(First);
	Gate.Complete(First);
	TestEqual(TEXT("Dismiss and failure callbacks cannot enter twice"), Entries, 1);
	const int32 Second = Gate.Begin(Callback());
	TestTrue(TEXT("Another new/continue entry can show another ad"), Second > First);
	Gate.Complete(First);
	TestEqual(TEXT("Stale completion cannot release another entry"), Entries, 1);
	Gate.Complete(Second);
	TestEqual(TEXT("Second entry proceeds"), Entries, 2);
	const int32 Cancelled = Gate.Begin(Callback());
	Gate.Cancel();
	Gate.Complete(Cancelled);
	TestEqual(TEXT("Closed title cannot enter from late callback"), Entries, 2);
	const int32 NoAd = Gate.Begin(Callback());
	Gate.Complete(NoAd);
	TestEqual(TEXT("No-fill can continue immediately"), Entries, 3);
	return true;
}
