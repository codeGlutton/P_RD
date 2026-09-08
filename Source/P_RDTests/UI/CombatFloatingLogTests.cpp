#include "UI/CombatFloatingLogTests.h"

#include "Misc/AutomationTest.h"
#include "AttributeSet/CombatTargetAttributeSet.h"
#include "Simulation/Logger/EventLog.h"
#include "UObject/StrongObjectPtr.h"
#include "GameMode/CombatGameMode.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatFloatingLogAttributeConversionTest,
	"P_RD.UI.Combat.FloatingLogAttributeConversion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCombatFloatingLogAttributeConversionTest::RunTest(const FString&)
{
	FSRPGAttributeEffectEventLog Log;
	Log.mEffectAttribute = UCombatTargetAttributeSet::GetHPAttribute();
	Log.mMagnitude = -12.f;
	Log.mIsCritical = true;
	const FVector Position(10, 20, 30);
	auto Request = ACombatGameMode::BuildAttributeFloatingLogRequest(Log, Position);
	TestTrue(TEXT("Live attribute conversion preserves critical flag"), Request.mIsCritical);
	TestEqual(TEXT("HP damage uses unsigned digits"), Request.mText.ToString(), FString(TEXT("12")));
	TestEqual(TEXT("Damage icon"), Request.mIconType, EFloatingLogIconType::HP);
	TestEqual(TEXT("Damage color"), Request.mColorType, EFloatingLogColorType::Damage);
	TestEqual(TEXT("World anchor retained"), Request.mWorldLocation, Position);
	TestEqual(TEXT("Live request does not wait for a motion index"), Request.mMotionIndex, INDEX_NONE);
	Log.mIsCritical = false;
	Request = ACombatGameMode::BuildAttributeFloatingLogRequest(Log, Position);
	TestFalse(TEXT("Normal hit does not inherit critical flag"), Request.mIsCritical);
	Log.mMagnitude = 8.f;
	Request = ACombatGameMode::BuildAttributeFloatingLogRequest(Log, Position);
	TestEqual(TEXT("Healing digits"), Request.mText.ToString(), FString(TEXT("8")));
	Log.mEffectAttribute = UCombatTargetAttributeSet::GetDefenseAttribute();
	Request = ACombatGameMode::BuildAttributeFloatingLogRequest(Log, Position);
	TestEqual(TEXT("Non-HP gain retains sign"), Request.mText.ToString(), FString(TEXT("+8")));
	Log.mMagnitude = -3.f;
	Request = ACombatGameMode::BuildAttributeFloatingLogRequest(Log, Position);
	TestEqual(TEXT("Non-HP loss retains sign"), Request.mText.ToString(), FString(TEXT("-3")));
	return !HasAnyErrors();
}
