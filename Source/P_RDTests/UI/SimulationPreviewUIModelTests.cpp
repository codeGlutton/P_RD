#include "UI/SimulationPreviewUIModelTests.h"

#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

/**
 * @file   SimulationPreviewUIModelTests.cpp
 * @brief  시뮬레이션 미리보기 뷰모델(USimulationPreviewUIModel)의 계약 검증.
 *
 * @details
 * 핵심 계약 셋을 못박는다.
 *  1. 격리    — 미리보기 set/clear가 실전 배치(UCombatUIModel)의 리비전·내용을 못 건드린다.
 *  2. 세대    — 낡은 세대의 배치는 버려지고, 현재 세대만 반영된다.
 *  3. 수명    — ClearPreview를 겹쳐 불러도 Cleared 알림은 한 번만 나간다.
 */

void USimulationPreviewUIModelTestListener::HandlePreviewChanged(ESimulationPreviewDomainUI Domain)
{
	++mPreviewChangedCallCount;
	mLastPreviewDomain = Domain;
}

void USimulationPreviewUIModelTestListener::HandlePreviewEventBatch(FCombatEventBatchUI Batch)
{
	++mPreviewBatchCallCount;
	mLastPreviewBatch = MoveTemp(Batch);
}

void USimulationPreviewUIModelTestListener::HandlePreviewCleared()
{
	++mPreviewClearedCallCount;
}

void USimulationPreviewUIModelTestListener::HandleLiveEventBatch(FCombatEventBatchUI Batch)
{
	++mLiveBatchCallCount;
	mLastLiveBatch = MoveTemp(Batch);
}

namespace
{
	/** @brief 표시 문구만 다른 최소 플로팅 로그 요청을 만든다. */
	FCombatFloatingLogRequest MakeSimulationPreviewTestRequest(const TCHAR* Text, const bool bIsPreview)
	{
		FCombatFloatingLogRequest Request;
		Request.mText = FText::FromString(Text);
		Request.mIconType = EFloatingLogIconType::HP;
		Request.mColorType = EFloatingLogColorType::Damage;
		Request.mIsPreview = bIsPreview;
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSimulationPreviewIsolationTest,
	"P_RD.UI.Combat.SimulationPreviewIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSimulationPreviewIsolationTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UCombatUIModel> UIModel(NewObject<UCombatUIModel>());
	TStrongObjectPtr<USimulationPreviewUIModelTestListener> Listener(NewObject<USimulationPreviewUIModelTestListener>());

	USimulationPreviewUIModel* PreviewModel = UIModel->GetSimulationPreviewUIModel();
	TestNotNull(TEXT("미리보기 모델이 지연 생성된다"), PreviewModel);
	TestTrue(TEXT("미리보기 모델은 실전 모델이 소유한다"), PreviewModel->GetOuter() == UIModel.Get());

	UIModel->OnCombatEventBatchChanged.AddDynamic(Listener.Get(), &USimulationPreviewUIModelTestListener::HandleLiveEventBatch);
	PreviewModel->OnPreviewEventBatch.AddDynamic(Listener.Get(), &USimulationPreviewUIModelTestListener::HandlePreviewEventBatch);
	PreviewModel->OnPreviewCleared.AddDynamic(Listener.Get(), &USimulationPreviewUIModelTestListener::HandlePreviewCleared);

	// 실전 로그(-40)를 먼저 내린다.
	UIModel->SetCombatEventBatch(ECombatEventDataSourceUI::LiveCombat,
		{ MakeSimulationPreviewTestRequest(TEXT("-40"), false) });
	TestEqual(TEXT("실전 배치 revision"), UIModel->GetCombatEventBatch().mRevision, 1);

	// 미리보기(-10)를 얹는다 — 실전 배치는 어떤 것도 변하면 안 된다.
	const int32 Generation = PreviewModel->BeginPreview();
	PreviewModel->SetPreviewEventBatch(Generation,
		{ MakeSimulationPreviewTestRequest(TEXT("-10"), true) });

	TestEqual(TEXT("미리보기 배치 출처"), Listener->mLastPreviewBatch.mSource, ECombatEventDataSourceUI::SimulationPreview);
	TestEqual(TEXT("미리보기 배치 로그 수"), Listener->mLastPreviewBatch.mFloatingLogs.Num(), 1);
	TestEqual(TEXT("미리보기 set 후에도 실전 revision 불변"), UIModel->GetCombatEventBatch().mRevision, 1);
	TestEqual(TEXT("미리보기 set 후에도 실전 로그 수 불변"), UIModel->GetCombatEventBatch().mFloatingLogs.Num(), 1);
	TestEqual(TEXT("미리보기 set 후에도 실전 로그 내용 불변"),
		UIModel->GetCombatEventBatch().mFloatingLogs[0].mText.ToString(), FString(TEXT("-40")));
	TestEqual(TEXT("미리보기 set이 실전 배치 알림을 내지 않는다"), Listener->mLiveBatchCallCount, 1);

	// 미리보기를 통째로 버려도 실전은 그대로다.
	PreviewModel->ClearPreview();
	TestEqual(TEXT("Clear 후에도 실전 revision 불변"), UIModel->GetCombatEventBatch().mRevision, 1);
	TestEqual(TEXT("Clear 후에도 실전 로그 수 불변"), UIModel->GetCombatEventBatch().mFloatingLogs.Num(), 1);
	TestEqual(TEXT("Clear 후에도 실전 배치 알림 수 불변"), Listener->mLiveBatchCallCount, 1);
	TestEqual(TEXT("Clear 후 미리보기 payload는 빈다"), PreviewModel->GetPreviewEventBatch().mFloatingLogs.Num(), 0);
	TestFalse(TEXT("Clear 후 미리보기는 닫힌다"), PreviewModel->IsPreviewActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSimulationPreviewGenerationTest,
	"P_RD.UI.Combat.SimulationPreviewGeneration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSimulationPreviewGenerationTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<USimulationPreviewUIModel> PreviewModel(NewObject<USimulationPreviewUIModel>());
	TStrongObjectPtr<USimulationPreviewUIModelTestListener> Listener(NewObject<USimulationPreviewUIModelTestListener>());

	PreviewModel->OnPreviewEventBatch.AddDynamic(Listener.Get(), &USimulationPreviewUIModelTestListener::HandlePreviewEventBatch);

	// 조준을 연달아 바꾸면 세대가 겹친다 — 낡은 세대의 결과는 버려져야 한다.
	const int32 StaleGeneration = PreviewModel->BeginPreview();
	const int32 CurrentGeneration = PreviewModel->BeginPreview();
	TestTrue(TEXT("BeginPreview마다 세대가 오른다"), CurrentGeneration > StaleGeneration);

	PreviewModel->SetPreviewEventBatch(StaleGeneration,
		{ MakeSimulationPreviewTestRequest(TEXT("-99"), true) });
	TestEqual(TEXT("낡은 세대 배치는 알림 없이 버려진다"), Listener->mPreviewBatchCallCount, 0);
	TestEqual(TEXT("낡은 세대 배치는 payload에 남지 않는다"), PreviewModel->GetPreviewEventBatch().mFloatingLogs.Num(), 0);

	PreviewModel->SetPreviewEventBatch(CurrentGeneration,
		{ MakeSimulationPreviewTestRequest(TEXT("-10"), true) });
	TestEqual(TEXT("현재 세대 배치는 반영된다"), Listener->mPreviewBatchCallCount, 1);
	TestEqual(TEXT("현재 세대 배치 로그 수"), PreviewModel->GetPreviewEventBatch().mFloatingLogs.Num(), 1);
	TestEqual(TEXT("현재 세대 배치 내용"),
		PreviewModel->GetPreviewEventBatch().mFloatingLogs[0].mText.ToString(), FString(TEXT("-10")));

	// Clear도 세대를 올린다 — 비행 중이던 결과가 빈 자리를 다시 채우면 안 된다.
	PreviewModel->ClearPreview();
	PreviewModel->SetPreviewEventBatch(CurrentGeneration,
		{ MakeSimulationPreviewTestRequest(TEXT("-77"), true) });
	TestEqual(TEXT("Clear 뒤 낡은 세대 배치는 버려진다"), Listener->mPreviewBatchCallCount, 1);
	TestEqual(TEXT("Clear 뒤 payload는 빈 채로 남는다"), PreviewModel->GetPreviewEventBatch().mFloatingLogs.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSimulationPreviewLifecycleTest,
	"P_RD.UI.Combat.SimulationPreviewLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSimulationPreviewLifecycleTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<USimulationPreviewUIModel> PreviewModel(NewObject<USimulationPreviewUIModel>());
	TStrongObjectPtr<USimulationPreviewUIModelTestListener> Listener(NewObject<USimulationPreviewUIModelTestListener>());

	PreviewModel->OnPreviewCleared.AddDynamic(Listener.Get(), &USimulationPreviewUIModelTestListener::HandlePreviewCleared);
	PreviewModel->OnPreviewChanged.AddDynamic(Listener.Get(), &USimulationPreviewUIModelTestListener::HandlePreviewChanged);

	// 열리지 않은 미리보기를 버려도 아무 알림이 없다.
	PreviewModel->ClearPreview();
	TestEqual(TEXT("열기 전 Clear는 알림이 없다"), Listener->mPreviewClearedCallCount, 0);

	PreviewModel->BeginPreview();
	TestTrue(TEXT("BeginPreview 후 미리보기가 열린다"), PreviewModel->IsPreviewActive());

	// 취소 경로가 겹쳐 Clear가 두 번 불려도 Cleared는 한 번만 나간다.
	PreviewModel->ClearPreview();
	PreviewModel->ClearPreview();
	TestEqual(TEXT("겹친 Clear에도 Cleared 알림은 한 번"), Listener->mPreviewClearedCallCount, 1);
	TestEqual(TEXT("Clear는 All 도메인 갱신을 알린다"), Listener->mLastPreviewDomain, ESimulationPreviewDomainUI::All);
	TestFalse(TEXT("Clear 후 미리보기는 닫힌 상태를 유지한다"), PreviewModel->IsPreviewActive());

	return true;
}
