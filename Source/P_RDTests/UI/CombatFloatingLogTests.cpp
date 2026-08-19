#include "UI/CombatFloatingLogTests.h"

#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

void UCombatFloatingLogTestListener::HandleFloatingLog(FCombatFloatingLogRequest Request)
{
	++mFloatingLogCallCount;
	mLastRequest = MoveTemp(Request);
}

void UCombatFloatingLogTestListener::HandleEventBatch(FCombatEventBatchUI Batch)
{
	++mEventBatchCallCount;
	mLastBatch = MoveTemp(Batch);
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
	UIModel->OnCombatEventBatchChanged.AddDynamic(Listener.Get(), &UCombatFloatingLogTestListener::HandleEventBatch);

	FCombatFloatingLogRequest Request;
	Request.mTurnIndex = 1;
	Request.mActionIndex = 2;
	Request.mMotionIndex = 3;
	UIModel->NotifyCombatFloatingLog(Request);

	TestEqual(TEXT("플로팅 로그가 한 번 전달된다"), Listener->mFloatingLogCallCount, 1);
	TestEqual(TEXT("TurnIndex가 유지된다"), Listener->mLastRequest.mTurnIndex, 1);
	TestEqual(TEXT("ActionIndex가 유지된다"), Listener->mLastRequest.mActionIndex, 2);
	TestEqual(TEXT("MotionIndex가 유지된다"), Listener->mLastRequest.mMotionIndex, 3);

	TArray<FCombatFloatingLogRequest> BatchRequests{ Request };
	UIModel->SetCombatEventBatch(ECombatEventDataSourceUI::SimulationPreview, BatchRequests);
	TestEqual(TEXT("예측 배치 출처가 유지된다"),
		Listener->mLastBatch.mSource, ECombatEventDataSourceUI::SimulationPreview);
	TestEqual(TEXT("예측 배치 revision"), Listener->mLastBatch.mRevision, 1);
	TestEqual(TEXT("예측 배치 로그 수"), Listener->mLastBatch.mFloatingLogs.Num(), 1);

	UIModel->SetCombatEventBatch(ECombatEventDataSourceUI::LiveCombat, BatchRequests);
	TestEqual(TEXT("실전 배치 출처가 유지된다"),
		Listener->mLastBatch.mSource, ECombatEventDataSourceUI::LiveCombat);
	TestEqual(TEXT("실전 배치 revision"), Listener->mLastBatch.mRevision, 2);
	TestEqual(TEXT("예측/실전 배치 알림 수"), Listener->mEventBatchCallCount, 2);

	UIModel->SetFocusScreenAnchor(FVector2D(-0.25f, 1.5f));
	TestEqual(TEXT("초점 앵커 X가 위젯 생성 등록 범위로 제한된다"),
		UIModel->GetFocusScreenAnchor().X, 0.0);
	TestEqual(TEXT("초점 앵커 Y가 위젯 생성 등록 범위로 제한된다"),
		UIModel->GetFocusScreenAnchor().Y, 1.0);

	return true;
}
