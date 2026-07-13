#include "UI/CombatFloatingLogTests.h"

#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

void UCombatFloatingLogTestListener::HandleFloatingLog(FCombatFloatingLogRequest Request)
{
	++mFloatingLogCallCount;
	mLastRequest = MoveTemp(Request);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatFloatingLogIndexContractTest,
	"P_RD.UI.Combat.FloatingLogIndexContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FCombatFloatingLogIndexContractTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UCombatUIModel> UIModel(NewObject<UCombatUIModel>());
	TStrongObjectPtr<UCombatFloatingLogTestListener> Listener(NewObject<UCombatFloatingLogTestListener>());

	UIModel->OnCombatFloatingLog.AddDynamic(Listener.Get(), &UCombatFloatingLogTestListener::HandleFloatingLog);

	FCombatFloatingLogRequest Request;
	Request.mTurnIndex = 1;
	Request.mActionIndex = 2;
	Request.mMotionIndex = 3;
	UIModel->NotifyCombatFloatingLog(Request);

	TestEqual(TEXT("플로팅 로그가 한 번 전달된다"), Listener->mFloatingLogCallCount, 1);
	TestEqual(TEXT("TurnIndex가 유지된다"), Listener->mLastRequest.mTurnIndex, 1);
	TestEqual(TEXT("ActionIndex가 유지된다"), Listener->mLastRequest.mActionIndex, 2);
	TestEqual(TEXT("MotionIndex가 유지된다"), Listener->mLastRequest.mMotionIndex, 3);

	return true;
}
