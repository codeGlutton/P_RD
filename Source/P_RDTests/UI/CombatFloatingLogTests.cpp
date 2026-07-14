#include "UI/CombatFloatingLogTests.h"

#include "Misc/AutomationTest.h"
#include "Input/WorldPressGesture.h"
#include "UObject/StrongObjectPtr.h"

void UCombatFloatingLogTestListener::HandleFloatingLog(FCombatFloatingLogRequest Request)
{
	++mFloatingLogCallCount;
	mLastRequest = MoveTemp(Request);
}

void UCombatFloatingLogTestListener::HandleUnitDetail(FUnitDetailUI Detail)
{
	++mUnitDetailCallCount;
	mLastUnitDetail = MoveTemp(Detail);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatUnitDetailReadyContractTest,
	"P_RD.UI.Combat.UnitDetailReadyContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FCombatUnitDetailReadyContractTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UCombatUIModel> UIModel(NewObject<UCombatUIModel>());
	TStrongObjectPtr<UCombatFloatingLogTestListener> Listener(NewObject<UCombatFloatingLogTestListener>());

	UIModel->OnUnitDetailReady.AddDynamic(Listener.Get(), &UCombatFloatingLogTestListener::HandleUnitDetail);

	FUnitDetailUI Detail;
	Detail.mUnitId = 42;
	Detail.mName = FText::FromString(TEXT("Target"));
	Detail.mLevel = 7;
	UIModel->SetUnitDetail(Detail);

	TestEqual(TEXT("상세 준비 알림이 한 번 전달된다"), Listener->mUnitDetailCallCount, 1);
	TestEqual(TEXT("UnitId가 유지된다"), Listener->mLastUnitDetail.mUnitId, 42);
	TestEqual(TEXT("캐시에도 동일한 UnitId가 저장된다"), UIModel->GetUnitDetail().mUnitId, 42);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldPressGestureContractTest,
	"P_RD.Input.WorldPressGestureContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FWorldPressGestureContractTest::RunTest(const FString& Parameters)
{
	constexpr float LongPressThreshold = 0.45f;
	constexpr float MoveTolerance = 24.0f;

	FWorldPressGestureTracker TapTracker;
	TapTracker.Begin(FVector2D(100.0f, 100.0f));
	TestTrue(TEXT("짧게 뗀 입력은 Tap이다"),
		TapTracker.End(FVector2D(102.0f, 101.0f), MoveTolerance) == EWorldPressGestureResult::Tap);

	FWorldPressGestureTracker HoldTracker;
	HoldTracker.Begin(FVector2D(100.0f, 100.0f));
	TestTrue(TEXT("임계시간 전에는 결과가 없다"),
		HoldTracker.Update(FVector2D(100.0f, 100.0f), 0.30f, LongPressThreshold, MoveTolerance)
			== EWorldPressGestureResult::None);
	TestTrue(TEXT("임계시간을 넘으면 LongPress가 한 번 발생한다"),
		HoldTracker.Update(FVector2D(100.0f, 100.0f), 0.15f, LongPressThreshold, MoveTolerance)
			== EWorldPressGestureResult::LongPress);
	TestTrue(TEXT("롱프레스 뒤 해제는 Tap을 추가로 만들지 않는다"),
		HoldTracker.End(FVector2D(100.0f, 100.0f), MoveTolerance) == EWorldPressGestureResult::None);

	FWorldPressGestureTracker DragTracker;
	DragTracker.Begin(FVector2D(100.0f, 100.0f));
	DragTracker.Update(FVector2D(140.0f, 100.0f), 0.10f, LongPressThreshold, MoveTolerance);
	TestFalse(TEXT("허용 거리를 넘긴 드래그는 제스처를 취소한다"), DragTracker.IsActive());
	TestTrue(TEXT("취소된 드래그 해제는 Tap이 아니다"),
		DragTracker.End(FVector2D(140.0f, 100.0f), MoveTolerance) == EWorldPressGestureResult::None);

	return true;
}
