#include "UI/SettingsRunActionTests.h"

#include "Misc/AutomationTest.h"
#include "UI/Combat/CombatUIModel.h"
#include "UObject/StrongObjectPtr.h"

void USettingsRunActionTestListener::HandleSaveAndExitRequested()
{
	++SaveRequestCount;
}

void USettingsRunActionTestListener::HandleAbandonRequested()
{
	++AbandonRequestCount;
}

void USettingsRunActionTestListener::HandleSaveAndExitCompleted(const bool bSuccess)
{
	++CompletionCount;
	bLastCompletionSucceeded = bSuccess;
}

void USettingsRunActionTestListener::HandleAbandonRunCompleted(const bool bSuccess)
{
	++AbandonCompletionCount;
	bLastAbandonCompletionSucceeded = bSuccess;
}

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettingsRunActionModelContractTest,
	"P_RD.UI.Settings.RunActionModelContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSettingsRunActionModelContractTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UCombatUIModel> Model(NewObject<UCombatUIModel>());
	TStrongObjectPtr<USettingsRunActionTestListener> Listener(
		NewObject<USettingsRunActionTestListener>());
	if (!TestNotNull(TEXT("전투 UIModel"), Model.Get())
		|| !TestNotNull(TEXT("설정 런 액션 수신기"), Listener.Get()))
	{
		return false;
	}

	Model->OnSaveAndExitRun.AddDynamic(
		Listener.Get(), &USettingsRunActionTestListener::HandleSaveAndExitRequested);
	Model->OnAbandonRun.AddDynamic(
		Listener.Get(), &USettingsRunActionTestListener::HandleAbandonRequested);
	Model->OnSaveAndExitCompleted.AddDynamic(
		Listener.Get(), &USettingsRunActionTestListener::HandleSaveAndExitCompleted);
	Model->OnAbandonRunCompleted.AddDynamic(
		Listener.Get(), &USettingsRunActionTestListener::HandleAbandonRunCompleted);

	Model->RequestSaveAndExitRun();
	TestEqual(TEXT("저장 후 종료 요청 1회"), Listener->SaveRequestCount, 1);
	TestEqual(TEXT("저장 요청은 포기를 발생시키지 않음"), Listener->AbandonRequestCount, 0);
	TestEqual(TEXT("저장 요청만으로 완료 알림 없음"), Listener->CompletionCount, 0);

	Model->RequestAbandonRun();
	TestEqual(TEXT("포기 요청 1회"), Listener->AbandonRequestCount, 1);
	TestEqual(TEXT("포기 요청은 저장 요청을 재발행하지 않음"), Listener->SaveRequestCount, 1);
	TestEqual(TEXT("포기 요청만으로 완료 알림 없음"), Listener->AbandonCompletionCount, 0);

	Model->NotifyAbandonRunCompleted(false);
	TestEqual(TEXT("포기 실패 결과 1회"), Listener->AbandonCompletionCount, 1);
	TestFalse(TEXT("포기 실패 결과값"), Listener->bLastAbandonCompletionSucceeded);
	TestEqual(TEXT("포기 실패는 요청을 재발행하지 않음"), Listener->AbandonRequestCount, 1);

	Model->NotifyAbandonRunCompleted(true);
	TestEqual(TEXT("포기 성공 결과까지 총 2회"), Listener->AbandonCompletionCount, 2);
	TestTrue(TEXT("포기 성공 결과값"), Listener->bLastAbandonCompletionSucceeded);
	TestEqual(TEXT("포기 성공도 요청을 재발행하지 않음"), Listener->AbandonRequestCount, 1);

	Model->NotifySaveAndExitCompleted(false);
	TestEqual(TEXT("저장 실패 결과 1회"), Listener->CompletionCount, 1);
	TestFalse(TEXT("저장 실패 결과값"), Listener->bLastCompletionSucceeded);
	TestEqual(TEXT("실패 결과는 저장 요청을 재발행하지 않음"), Listener->SaveRequestCount, 1);
	TestEqual(TEXT("실패 결과는 포기 요청을 재발행하지 않음"), Listener->AbandonRequestCount, 1);

	Model->NotifySaveAndExitCompleted(true);
	TestEqual(TEXT("저장 성공 결과까지 총 2회"), Listener->CompletionCount, 2);
	TestTrue(TEXT("저장 성공 결과값"), Listener->bLastCompletionSucceeded);
	TestEqual(TEXT("성공 결과도 저장 요청을 재발행하지 않음"), Listener->SaveRequestCount, 1);
	TestEqual(TEXT("성공 결과도 포기 요청을 재발행하지 않음"), Listener->AbandonRequestCount, 1);

	return true;
}

#endif
