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
#include "Internationalization/Internationalization.h"
#include "Misc/AutomationTest.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/RunOptionsRailWidget.h"
#include "UI/SettingsPanelWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettingsPanelWbpTextDefaultsTest,
	"P_RD.UI.Settings.WBPTextDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSettingsPanelWbpTextDefaultsTest::RunTest(const FString& Parameters)
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

	struct FCultureRestore
	{
		FString OriginalCulture =
			FInternationalization::Get().GetCurrentCulture()->GetName();
		~FCultureRestore()
		{
			FInternationalization::Get().SetCurrentCulture(OriginalCulture);
		}
	} CultureRestore;

	struct FExpectedText
	{
		const TCHAR* WidgetName;
		const TCHAR* Korean;
		const TCHAR* English;
	};
	const FExpectedText Expected[] = {
		{ TEXT("SettingsTitleText"), TEXT("설정 장부"), TEXT("SETTINGS LEDGER") },
		{ TEXT("AudioSectionHeader"), TEXT("오디오"), TEXT("Audio") },
		{ TEXT("MasterVolumeRow_Label"), TEXT("전체"), TEXT("Master") },
		{ TEXT("BGMVolumeRow_Label"), TEXT("배경음"), TEXT("BGM") },
		{ TEXT("SFXVolumeRow_Label"), TEXT("효과음"), TEXT("SFX") },
		{ TEXT("UIVolumeRow_Label"), TEXT("UI"), TEXT("UI") },
		{ TEXT("DisplaySectionHeader"), TEXT("화면"), TEXT("Display") },
		{ TEXT("BrightnessRow_Label"), TEXT("밝기"), TEXT("Brightness") },
		{ TEXT("ScreenShakeRow_Label"), TEXT("화면 흔들림"), TEXT("Screen Shake") },
		{ TEXT("EffectsRow_Label"), TEXT("이펙트"), TEXT("Effects") },
		{ TEXT("VibrationRow_Label"), TEXT("진동"), TEXT("Vibration") },
		{ TEXT("QualityRow_Label"), TEXT("품질"), TEXT("Quality") },
		{ TEXT("FpsRow_Label"), TEXT("FPS"), TEXT("FPS") },
		{ TEXT("LowQualityButtonText"), TEXT("낮음"), TEXT("LOW") },
		{ TEXT("MediumQualityButtonText"), TEXT("중간"), TEXT("MID") },
		{ TEXT("HighQualityButtonText"), TEXT("높음"), TEXT("HIGH") },
		{ TEXT("FpsThirtyButtonText"), TEXT("30"), TEXT("30") },
		{ TEXT("FpsSixtyButtonText"), TEXT("60"), TEXT("60") },
		{ TEXT("GameplaySectionHeader"), TEXT("게임플레이"), TEXT("Gameplay") },
		{ TEXT("FastModeRow_Label"), TEXT("빠른 모드"), TEXT("Fast Mode") },
		{ TEXT("SkipAnimationRow_Label"), TEXT("연출 스킵"), TEXT("Skip Animation") },
		{ TEXT("AutoEndTurnRow_Label"), TEXT("자동 턴 종료"), TEXT("Auto End Turn") },
		{ TEXT("LanguageRow_Label"), TEXT("언어"), TEXT("Language") },
		{ TEXT("LanguageKoreanButtonText"), TEXT("한국어"), TEXT("한국어") },
		{ TEXT("LanguageEnglishButtonText"), TEXT("English"), TEXT("English") },
		{ TEXT("CreditsOpenButtonText"), TEXT("열기"), TEXT("Open") },
		{ TEXT("LicenseOpenButtonText"), TEXT("열기"), TEXT("Open") },
		{ TEXT("BackButtonText"), TEXT("뒤로가기"), TEXT("Back") },
		{ TEXT("SaveAndExitButtonText"), TEXT("저장 후 종료"), TEXT("Save and Exit") },
		{ TEXT("AbandonRunButtonText"), TEXT("런 포기"), TEXT("Abandon Run") },
		{ TEXT("ResetButtonText"), TEXT("초기화"), TEXT("Reset") },
		{ TEXT("AbandonConfirmTitleText"), TEXT("이 런을 포기하시겠습니까?"), TEXT("Abandon this run?") },
		{ TEXT("AbandonConfirmBodyText"), TEXT("현재 진행 상황이 삭제되고 타이틀로 돌아갑니다."),
			TEXT("Current progress will be deleted and you will return to the title.") },
		{ TEXT("ConfirmAbandonButtonText"), TEXT("포기"), TEXT("Abandon") },
		{ TEXT("CancelAbandonButtonText"), TEXT("취소"), TEXT("Cancel") },
		{ TEXT("RunConfirmHeaderText"), TEXT("런 포기"), TEXT("ABANDON RUN") },
	};

	const auto CheckCulture = [this, World, SettingsClass, &Expected](
		const TCHAR* Culture, const bool bKorean)
	{
		if (!FInternationalization::Get().SetCurrentCulture(Culture))
		{
			AddError(FString::Printf(TEXT("문화 변경 실패: %s"), Culture));
			return;
		}
		// TakeWidget/NativeConstruct를 호출하지 않아 C++ SyncText가 값을
		// 덮어쓰지 않는 순수 WBP 기본값을 검증한다.
		USettingsPanelWidget* Panel =
			CreateWidget<USettingsPanelWidget>(World, SettingsClass);
		if (!TestNotNull(FString::Printf(TEXT("%s WBP 인스턴스"), Culture), Panel))
		{
			return;
		}
		USettingsPanelWidget* RuntimePanel =
			CreateWidget<USettingsPanelWidget>(World, SettingsClass);
		if (!TestNotNull(FString::Printf(TEXT("%s 런타임 인스턴스"), Culture),
			RuntimePanel))
		{
			return;
		}
		RuntimePanel->ApplyValueModel(RuntimePanel->GetValueModel());
		for (const FExpectedText& Entry : Expected)
		{
			UTextBlock* Text = Cast<UTextBlock>(
				Panel->WidgetTree->FindWidget(FName(Entry.WidgetName)));
			if (TestNotNull(FString::Printf(TEXT("%s/%s"), Culture, Entry.WidgetName), Text))
			{
				TestEqual(FString::Printf(TEXT("%s/%s 문구"), Culture, Entry.WidgetName),
					Text->GetText().ToString(),
					FString(bKorean ? Entry.Korean : Entry.English));
				if (UTextBlock* RuntimeText = Cast<UTextBlock>(
					RuntimePanel->WidgetTree->FindWidget(FName(Entry.WidgetName))))
				{
					TestEqual(FString::Printf(TEXT("%s/%s WBP-C++ 일치"),
						Culture, Entry.WidgetName), Text->GetText().ToString(),
						RuntimeText->GetText().ToString());
				}
			}
		}
	};

	CheckCulture(TEXT("ko"), true);
	CheckCulture(TEXT("en"), false);
	return !HasAnyErrors();
}

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
	// 이 시험은 설정 입력 계층만 검증한다. 헤드리스 환경에서 별도의
	// 가짜 전투까지 시작하면 프리뷰용 GameplayTag 구성에 결과가 종속된다.
	HUD->mUsePreviewData = false;
	const TSharedRef<SWidget> HUDSlate = HUD->TakeWidget();
	(void)HUDSlate;
	UButton* GearButton = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("MenuButton_3")));
	if (!TestNotNull(TEXT("실제 설정 톱니 버튼"), GearButton))
	{
		return false;
	}
	UOverlay* OptionsRailFrameMount = Cast<UOverlay>(
		HUD->WidgetTree->FindWidget(TEXT("OptionsRailFrameMount")));
	if (TestNotNull(TEXT("전투 메뉴 입력 Overlay"), OptionsRailFrameMount))
	{
		TestEqual(TEXT("설정 톱니도 앞의 세 버튼과 같은 입력 계층"),
			GearButton->GetParent(),
			static_cast<UPanelWidget*>(OptionsRailFrameMount));
		if (UOverlaySlot* GearSlot = Cast<UOverlaySlot>(GearButton->Slot))
		{
			const FVector2D GearPosition = RunOptionsRail::ButtonPosition(3);
			const FVector2D GearSize = RunOptionsRail::ButtonSize();
			TestEqual(TEXT("설정 톱니 런타임 여백"), GearSlot->GetPadding(),
				FMargin(GearPosition.X, GearPosition.Y,
					RunOptionsRail::Width - GearPosition.X - GearSize.X,
					RunOptionsRail::Height - GearPosition.Y - GearSize.Y));
		}
		else
		{
			AddError(TEXT("설정 톱니가 Overlay 슬롯을 받지 못했다."));
		}
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
