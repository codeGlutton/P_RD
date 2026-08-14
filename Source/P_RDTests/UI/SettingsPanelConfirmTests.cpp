#include "UI/SettingsPanelConfirmTests.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Editor.h"
#include "Misc/AutomationTest.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/SettingsPanelWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettingsPanelConfirmFlowTest,
	"P_RD.UI.Settings.ConfirmFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSettingsPanelConfirmFlowTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	UClass* SettingsClass = LoadClass<USettingsPanelWidget>(nullptr,
		TEXT("/Game/UI/WBP_SettingsPanel.WBP_SettingsPanel_C"));
	if (!TestNotNull(TEXT("설정 WBP"), SettingsClass)
		|| !TestNotNull(TEXT("테스트 월드"), World))
	{
		return false;
	}

	USettingsPanelWidget* Panel = CreateWidget<USettingsPanelWidget>(World, SettingsClass);
	TStrongObjectPtr<USettingsPanelConfirmTestListener> Listener(
		NewObject<USettingsPanelConfirmTestListener>());
	if (!TestNotNull(TEXT("설정 인스턴스"), Panel)
		|| !TestNotNull(TEXT("확인 이벤트 수신기"), Listener.Get()))
	{
		return false;
	}
	const TSharedRef<SWidget> PanelSlate = Panel->TakeWidget();
	Panel->SetPanelMode(ESettingsPanelMode::InGame);
	Panel->RefreshPanelState(true, true);
	Panel->OnSaveAndExitRequested.AddDynamic(
		Listener.Get(), &USettingsPanelConfirmTestListener::HandleSave);
	Panel->OnAbandonRunConfirmed.AddDynamic(
		Listener.Get(), &USettingsPanelConfirmTestListener::HandleAbandon);

	UButton* SaveButton = Cast<UButton>(Panel->WidgetTree->FindWidget(TEXT("SaveAndExitButton")));
	UButton* ConfirmButton = Cast<UButton>(Panel->WidgetTree->FindWidget(TEXT("ConfirmAbandonButton")));
	UButton* CancelButton = Cast<UButton>(Panel->WidgetTree->FindWidget(TEXT("CancelAbandonButton")));
	UWidget* ConfirmPanel = Panel->WidgetTree->FindWidget(TEXT("AbandonConfirmPanel"));
	UWidget* ConfirmViewportLayer = Panel->WidgetTree->FindWidget(
		TEXT("RunConfirmViewportLayer"));
	UTextBlock* Title = Cast<UTextBlock>(Panel->WidgetTree->FindWidget(TEXT("AbandonConfirmTitleText")));
	UTextBlock* Header = Cast<UTextBlock>(Panel->WidgetTree->FindWidget(TEXT("RunConfirmHeaderText")));
	if (!TestNotNull(TEXT("저장 후 종료 버튼"), SaveButton)
		|| !TestNotNull(TEXT("공용 확인 버튼"), ConfirmButton)
		|| !TestNotNull(TEXT("공용 취소 버튼"), CancelButton)
		|| !TestNotNull(TEXT("공용 확인 패널"), ConfirmPanel)
		|| !TestNotNull(TEXT("전체 viewport 확인 레이어"), ConfirmViewportLayer)
		|| !TestNotNull(TEXT("공용 확인 제목"), Title)
		|| !TestNotNull(TEXT("공용 확인 명패"), Header))
	{
		return false;
	}

	Panel->ShowSaveAndExitConfirm();
	TestEqual(TEXT("확인창 표시는 저장을 실행하지 않음"), Listener->SaveCount, 0);
	TestEqual(TEXT("저장 확인창 노출"), ConfirmPanel->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("저장 확인 시 viewport dim 레이어 노출"),
		ConfirmViewportLayer->GetVisibility(), ESlateVisibility::Visible);
	TestTrue(TEXT("저장 확인 문구"), Title->GetText().ToString().Contains(TEXT("저장"))
		|| Title->GetText().ToString().Contains(TEXT("Save")));
	TestTrue(TEXT("저장 액션 명패"), Header->GetText().ToString().Contains(TEXT("저장"))
		|| Header->GetText().ToString().Contains(TEXT("SAVE")));
	CancelButton->OnClicked.Broadcast();
	TestEqual(TEXT("취소는 저장 요청 없음"), Listener->SaveCount, 0);
	TestEqual(TEXT("취소 뒤 확인창 닫힘"), ConfirmPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("취소 뒤 viewport dim 레이어 닫힘"),
		ConfirmViewportLayer->GetVisibility(), ESlateVisibility::Collapsed);

	Panel->ShowSaveAndExitConfirm();
	ConfirmButton->OnClicked.Broadcast();
	TestEqual(TEXT("저장 확인 시 1회 요청"), Listener->SaveCount, 1);
	TestEqual(TEXT("저장 확인은 포기하지 않음"), Listener->AbandonCount, 0);
	TestEqual(TEXT("저장 확인 뒤 모달 닫힘"), ConfirmPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("저장 확인 뒤 viewport dim 레이어 닫힘"),
		ConfirmViewportLayer->GetVisibility(), ESlateVisibility::Collapsed);

	Panel->ShowAbandonConfirm();
	TestTrue(TEXT("포기 액션 명패"), Header->GetText().ToString().Contains(TEXT("포기"))
		|| Header->GetText().ToString().Contains(TEXT("ABANDON")));
	ConfirmButton->OnClicked.Broadcast();
	TestEqual(TEXT("포기 확인 시 1회 요청"), Listener->AbandonCount, 1);
	TestEqual(TEXT("포기 확인은 저장을 재요청하지 않음"), Listener->SaveCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettingsPanelBackLifecycleTest,
	"P_RD.UI.Settings.BackLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSettingsPanelBackLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* World = nullptr;
	if (GEngine != nullptr)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() != nullptr)
			{
				World = Context.World();
				break;
			}
		}
	}
	UClass* SettingsClass = LoadClass<USettingsPanelWidget>(nullptr,
		TEXT("/Game/UI/WBP_SettingsPanel.WBP_SettingsPanel_C"));
	if (!TestNotNull(TEXT("Back 테스트 월드"), World)
		|| !TestNotNull(TEXT("설정 WBP"), SettingsClass))
	{
		return false;
	}

	// RD.SettingsPreview.InGame uses this exact no-listener sequence. Back must
	// close the panel by itself because the preview intentionally mutates no owner.
	USettingsPanelWidget* PreviewPanel =
		CreateWidget<USettingsPanelWidget>(World, SettingsClass);
	if (!TestNotNull(TEXT("인게임 프리뷰 설정 인스턴스"), PreviewPanel))
	{
		return false;
	}
	const TSharedRef<SWidget> PreviewSlate = PreviewPanel->TakeWidget();
	(void)PreviewSlate;
	PreviewPanel->SetPanelMode(ESettingsPanelMode::InGame);
	PreviewPanel->RefreshPanelState(true, true);
	PreviewPanel->HideAbandonConfirm();
	PreviewPanel->SetStatusText(FText::GetEmpty());
	PreviewPanel->OpenUI();
	UButton* PreviewBack = Cast<UButton>(
		PreviewPanel->WidgetTree->FindWidget(TEXT("BackButton")));
	if (!TestNotNull(TEXT("프리뷰 Back 버튼"), PreviewBack))
	{
		return false;
	}
	TestFalse(TEXT("프리뷰는 외부 Back 수신자 없이 연다"),
		PreviewPanel->OnBackRequested.IsBound());
	// Editor commandlet에는 game viewport가 없으므로 IsOpened()는 false다.
	// 하지만 OpenUI 생명주기와 실제 표시 상태는 동일하게 진행되어 Back의
	// CloseUI 결과(Collapsed)를 헤드리스 환경에서도 정확히 관찰할 수 있다.
	TestEqual(TEXT("프리뷰 OpenUI가 패널을 표시함"),
		PreviewPanel->GetVisibility(), ESlateVisibility::Visible);
	PreviewBack->OnClicked.Broadcast();
	TestEqual(TEXT("프리뷰 Back 뒤 Collapsed"), PreviewPanel->GetVisibility(),
		ESlateVisibility::Collapsed);

	// Actual game route: combat gear -> shared world settings -> Back. This also
	// proves the HUD owner remains subscribed for its surrounding navigation work.
	UWorldWidgetSubsystem* WidgetSubsystem =
		World->GetSubsystem<UWorldWidgetSubsystem>();
	if (!TestNotNull(TEXT("월드 위젯 서브시스템"), WidgetSubsystem))
	{
		return false;
	}
	// 커맨드렛에는 PlayerController가 없어서 InitWorldWidget()은 쓸 수 없다.
	// 테스트 인스턴스를 실제 레지스트리에 넣고, 이하에서는 제품의 톱니 버튼
	// 핸들러가 GetWorldWidget()으로 찾아 여는 경로를 그대로 통과시킨다.
	WidgetSubsystem->SetWorldWidgetForTest(
		EWorldWidgetType::InGameSettings, PreviewPanel);
	UClass* HUDClass = LoadClass<UCombatLayoutHUDWidget>(nullptr,
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
	UCombatLayoutHUDWidget* HUD = HUDClass != nullptr
		? CreateWidget<UCombatLayoutHUDWidget>(World, HUDClass) : nullptr;
	if (!TestNotNull(TEXT("전투 HUD"), HUD))
	{
		return false;
	}
	const TSharedRef<SWidget> HUDSlate = HUD->TakeWidget();
	(void)HUDSlate;
	UButton* GearButton = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("MenuButton_3")));
	if (!TestNotNull(TEXT("실제 설정 톱니 버튼"), GearButton))
	{
		return false;
	}
	GearButton->OnClicked.Broadcast();
	USettingsPanelWidget* GearPanel =
		WidgetSubsystem->GetWorldWidget<USettingsPanelWidget>(
			EWorldWidgetType::InGameSettings);
	if (!TestNotNull(TEXT("톱니가 연 공용 설정창"), GearPanel))
	{
		return false;
	}
	TestTrue(TEXT("톱니 경로가 Back 수신자를 연결함"),
		GearPanel->OnBackRequested.IsBound());
	TestEqual(TEXT("톱니로 설정창이 열림"), GearPanel->GetVisibility(),
		ESlateVisibility::Visible);
	UButton* GearBack = Cast<UButton>(
		GearPanel->WidgetTree->FindWidget(TEXT("BackButton")));
	if (!TestNotNull(TEXT("톱니 경로 Back 버튼"), GearBack))
	{
		return false;
	}
	GearBack->OnClicked.Broadcast();
	TestEqual(TEXT("톱니 경로 Back 뒤 Collapsed"), GearPanel->GetVisibility(),
		ESlateVisibility::Collapsed);
	WidgetSubsystem->SetWorldWidgetForTest(
		EWorldWidgetType::InGameSettings, nullptr);
	return true;
}

#endif
