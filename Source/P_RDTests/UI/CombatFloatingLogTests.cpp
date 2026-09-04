#include "UI/CombatFloatingLogTests.h"

#include "Misc/AutomationTest.h"
#include "AttributeSet/CombatTargetAttributeSet.h"
#include "Simulation/Logger/EventLog.h"
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
	Request.mIsCritical = true;
	UIModel->NotifyCombatFloatingLog(Request);

	TestEqual(TEXT("플로팅 로그가 한 번 전달된다"), Listener->mFloatingLogCallCount, 1);
	TestEqual(TEXT("TurnIndex가 유지된다"), Listener->mLastRequest.mTurnIndex, 1);
	TestEqual(TEXT("ActionIndex가 유지된다"), Listener->mLastRequest.mActionIndex, 2);
	TestEqual(TEXT("MotionIndex가 유지된다"), Listener->mLastRequest.mMotionIndex, 3);
	TestTrue(TEXT("치명타 플래그가 유지된다"), Listener->mLastRequest.mIsCritical);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatFloatingLogCriticalAggregationKeyTest,
	"P_RD.UI.Combat.FloatingLogCriticalAggregationKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FCombatFloatingLogCriticalAggregationKeyTest::RunTest(const FString& Parameters)
{
	FSRPGAttributeEffectEventLog Normal;
	Normal.mEffectAttribute = UCombatTargetAttributeSet::GetHPAttribute();
	Normal.mMagnitude = -10.0f;

	FSRPGAttributeEffectEventLog Critical = Normal;
	Critical.mIsCritical = true;

	TSet<FSRPGAttributeEffectEventLog> Logs;
	Logs.Add(Normal);
	Logs.Add(Critical);
	TestEqual(TEXT("같은 모션의 일반 피해와 치명타 피해는 별도 로그로 보존된다"),
		Logs.Num(), 2);
	return true;
}
