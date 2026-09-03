#include "Editor.h"
#include "Misc/AutomationTest.h"
#include "UI/LoadingNotifyWidget.h"
#include "UI/RunOptionsRailWidget.h"
#include "Widgets/SWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoadingNotifyOptionsRailVisibilityLifecycleTest,
	"P_RD.UI.LoadingNotify.OptionsRailVisibilityLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLoadingNotifyOptionsRailVisibilityLifecycleTest::RunTest(
	const FString& Parameters)
{
	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드"), World))
	{
		return false;
	}

	URunOptionsRailWidget* VisibleRail =
		CreateWidget<URunOptionsRailWidget>(World, URunOptionsRailWidget::StaticClass());
	URunOptionsRailWidget* HiddenRail =
		CreateWidget<URunOptionsRailWidget>(World, URunOptionsRailWidget::StaticClass());
	URunOptionsRailWidget* CollapsedRail =
		CreateWidget<URunOptionsRailWidget>(World, URunOptionsRailWidget::StaticClass());
	ULoadingNotifyWidget* Loading =
		CreateWidget<ULoadingNotifyWidget>(World, ULoadingNotifyWidget::StaticClass());
	if (!TestNotNull(TEXT("표시 옵션 레일"), VisibleRail)
		|| !TestNotNull(TEXT("숨김 옵션 레일"), HiddenRail)
		|| !TestNotNull(TEXT("접힌 옵션 레일"), CollapsedRail)
		|| !TestNotNull(TEXT("로딩 알림"), Loading))
	{
		return false;
	}

	const TSharedRef<SWidget> VisibleRailSlate = VisibleRail->TakeWidget();
	const TSharedRef<SWidget> HiddenRailSlate = HiddenRail->TakeWidget();
	const TSharedRef<SWidget> CollapsedRailSlate = CollapsedRail->TakeWidget();
	const TSharedRef<SWidget> LoadingSlate = Loading->TakeWidget();
	(void)VisibleRailSlate;
	(void)HiddenRailSlate;
	(void)CollapsedRailSlate;
	(void)LoadingSlate;

	VisibleRail->SetVisibility(ESlateVisibility::Visible);
	HiddenRail->SetVisibility(ESlateVisibility::Hidden);
	CollapsedRail->SetVisibility(ESlateVisibility::Collapsed);
	Loading->SetVisibleDurationsForTest(0.0f, 0.0f, 0.0f);

	Loading->OpenUI();
	TestEqual(TEXT("로딩 중 표시 레일 숨김"), VisibleRail->GetVisibility(),
		ESlateVisibility::Collapsed);
	TestEqual(TEXT("로딩 중 Hidden 레일 숨김"), HiddenRail->GetVisibility(),
		ESlateVisibility::Collapsed);
	TestEqual(TEXT("원래 Collapsed 레일 유지"), CollapsedRail->GetVisibility(),
		ESlateVisibility::Collapsed);

	Loading->CloseUI();
	TestEqual(TEXT("닫힘 뒤 Visible 복구"), VisibleRail->GetVisibility(),
		ESlateVisibility::Visible);
	TestEqual(TEXT("닫힘 뒤 Hidden 복구"), HiddenRail->GetVisibility(),
		ESlateVisibility::Hidden);
	TestEqual(TEXT("닫힘 뒤 기존 Collapsed 유지"), CollapsedRail->GetVisibility(),
		ESlateVisibility::Collapsed);

	return !HasAnyErrors();
}

#endif
