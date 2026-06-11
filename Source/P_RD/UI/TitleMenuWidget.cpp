#include "UI/TitleMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Frontend/FrontendViewTypes.h"
#include "GameMode/FrontendGameMode.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/CharacterSelectWidget.h"
#include "UI/FrontendMapWidget.h"
#include "UI/SettingsPanelWidget.h"

namespace
{
	constexpr int32 TitleMainScreenIndex = 0;
}

UTitleMenuWidget::UTitleMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, mTitleText(NSLOCTEXT("TitleMenuWidget", "TitleText", "Rogue The Dice"))
	, mStartButtonText(NSLOCTEXT("TitleMenuWidget", "StartText", "START"))
	, mNewStartButtonText(NSLOCTEXT("TitleMenuWidget", "NewStartText", "NEW START"))
	, mContinueButtonText(NSLOCTEXT("TitleMenuWidget", "ContinueText", "CONTINUE"))
	, mSettingsButtonText(NSLOCTEXT("TitleMenuWidget", "SettingsText", "SETTING"))
	, mSettingsStatusText(NSLOCTEXT("TitleMenuWidget", "SettingsStatusText", "Settings"))
	, mMainOnlyStatusText(NSLOCTEXT("TitleMenuWidget", "MainOnlyStatusText", "Title main screen only"))
	, mCharacterSelectUnavailableText(NSLOCTEXT("TitleMenuWidget", "CharacterSelectUnavailableText", "Character select widget is not ready"))
	, mBackButtonText(NSLOCTEXT("TitleMenuWidget", "BackText", "BACK"))
{
	/*
	 * 타이틀 HUD는 방 진입 직후 바로 보이는 메인 UI다.
	 * 기본 Visibility를 Visible로 맞춰두되, 실제 AddToViewport/표시 생명주기는 RDUserWidget::OpenUI()가 처리한다.
	 */
	SetVisibility(ESlateVisibility::Visible);
}

/**
 * @brief 캐릭터 선택 화면이 GameMode 기준 후보 목록을 다시 읽게 한다.
 *
 * @details
 * 타이틀 메뉴는 캐릭터 데이터를 직접 만들지 않는다.
 * CharacterSelectWidget에게 갱신 요청만 전달해, 캐릭터 카드 구성 책임이 해당 위젯에 남도록 한다.
 */
void UTitleMenuWidget::RefreshCharacterOptionsFromGameMode()
{
	if (CharacterSelectWidget != nullptr)
	{
		CharacterSelectWidget->RefreshCharacterOptionsFromGameMode();
	}
}

/**
 * @brief GameMode가 요청한 타이틀 내부 화면 전환을 실제 WBP 화면 전환으로 수행한다.
 *
 * @details
 * AFrontendGameMode::CreateNewRunFromTitle()은 런 생성 대신 이 함수를 통해 캐릭터 선택 화면만 연다.
 * 이렇게 하면 START 버튼 입력 경로와 GameMode API 경로가 모두 ShowCharacterScreen()으로 합쳐진다.
 */
void UTitleMenuWidget::OpenCharacterSelectFromTitle()
{
	ShowCharacterScreen();
}

/**
 * @brief WBP 바인딩을 검증하고 타이틀 화면에서 필요한 버튼/하위 위젯 이벤트를 연결한다.
 *
 * @details
 * WBP_TitleMenu는 StartScreen, CharacterScreen, MapScreen, SettingsScreen을 ScreenSwitcher 안에 직접 배치한다.
 * C++은 해당 화면을 새로 만들지 않고, BindWidget으로 들어온 인스턴스를 연결만 한다.
 *
 * 왜 여기서 설정 패널/지도/캐릭터 선택 이벤트까지 연결하는가:
 * 각 하위 위젯은 "뒤로 가기"나 "닫기 요청"만 외부로 알린다.
 * 실제로 타이틀 메인 화면으로 돌아갈지, 지도 화면을 갱신할지는 바깥 화면 전환을 알고 있는 TitleMenuWidget이 결정한다.
 */
void UTitleMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ValidateDesignerBindings();

	if (StartButton != nullptr)
	{
		StartButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleStartButtonClicked);
	}

	if (ContinueButton != nullptr)
	{
		ContinueButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleContinueButtonClicked);
	}

	if (SettingsButton != nullptr)
	{
		SettingsButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleSettingsButtonClicked);
	}

	if (SettingsBackButton != nullptr)
	{
		SettingsBackButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleSettingsBackButtonClicked);
	}

	if (USettingsPanelWidget* TitleSettingsPanel = GetTitleSettingsPanel())
	{
		TitleSettingsPanel->OnBackRequested.AddUniqueDynamic(this, &UTitleMenuWidget::HandleSettingsBackButtonClicked);
	}

	if (CharacterSelectWidget != nullptr)
	{
		CharacterSelectWidget->OnBackToMainRequested.AddUniqueDynamic(this, &UTitleMenuWidget::HandleCharacterBackToMainRequested);
	}

	if (FrontendMapWidget != nullptr)
	{
		FrontendMapWidget->OnCloseRequested.AddUniqueDynamic(this, &UTitleMenuWidget::HandleMapBackRequested);
	}

	SyncMainText();
	RefreshMainMenuState();
	ShowMainScreen();
	SetStatusText(FText::GetEmpty());
}

/**
 * @brief Construct에서 붙인 이벤트를 제거해 재Construct 시 중복 호출을 막는다.
 *
 * @details
 * UUserWidget은 OpenUI/CloseUI나 레벨 전환 과정에서 다시 Construct될 수 있다.
 * AddUniqueDynamic을 사용하더라도 명시적으로 해제해두면 WBP 교체/하위 위젯 재생성 시 이벤트 잔류를 피할 수 있다.
 */
void UTitleMenuWidget::NativeDestruct()
{
	if (StartButton != nullptr)
	{
		StartButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleStartButtonClicked);
	}

	if (ContinueButton != nullptr)
	{
		ContinueButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleContinueButtonClicked);
	}

	if (SettingsButton != nullptr)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleSettingsButtonClicked);
	}

	if (SettingsBackButton != nullptr)
	{
		SettingsBackButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleSettingsBackButtonClicked);
	}

	if (USettingsPanelWidget* TitleSettingsPanel = GetTitleSettingsPanel())
	{
		TitleSettingsPanel->OnBackRequested.RemoveDynamic(this, &UTitleMenuWidget::HandleSettingsBackButtonClicked);
	}

	if (CharacterSelectWidget != nullptr)
	{
		CharacterSelectWidget->OnBackToMainRequested.RemoveDynamic(this, &UTitleMenuWidget::HandleCharacterBackToMainRequested);
	}

	if (FrontendMapWidget != nullptr)
	{
		FrontendMapWidget->OnCloseRequested.RemoveDynamic(this, &UTitleMenuWidget::HandleMapBackRequested);
	}

	Super::NativeDestruct();
}

/**
 * @brief ScreenSwitcher의 표시 대상을 바꾼다.
 *
 * @details
 * 하위 위젯들이 ScreenSwitcher를 직접 만지지 않게 하기 위한 공통 진입점이다.
 * Screen이 nullptr인 경우에는 타이틀 메인 화면으로 돌려, WBP 연결 누락 시에도 사용자가 빈 화면에 갇히지 않게 한다.
 */
void UTitleMenuWidget::ShowScreen(UWidget* Screen) const
{
	if (ScreenSwitcher == nullptr || Screen == nullptr)
	{
		if (ScreenSwitcher != nullptr && Screen == nullptr)
		{
			ScreenSwitcher->SetActiveWidgetIndex(TitleMainScreenIndex);
		}
		return;
	}

	ScreenSwitcher->SetActiveWidget(Screen);
}

/**
 * @brief 타이틀 메인 화면으로 복귀한다.
 *
 * @details
 * 캐릭터 선택, 설정, 지도 화면의 Back/Close 요청은 모두 이 함수로 모인다.
 * 타이틀의 "현재 화면" 정책을 한 곳에서 관리하기 위한 래퍼다.
 */
void UTitleMenuWidget::ShowMainScreen() const
{
	ShowScreen(StartScreen);
}

/**
 * @brief 캐릭터 선택 화면을 열고 캐릭터 카드 상태를 초기화한다.
 *
 * @details
 * CharacterSelectWidget이 준비되어 있으면 OpenCharacterSelect()를 호출해 GameMode에서 캐릭터 후보를 다시 받아온다.
 * 위젯 연결이 빠진 경우에는 캐릭터 화면 슬롯으로 넘어가되 상태 문구를 남겨 WBP 바인딩 문제를 찾을 수 있게 한다.
 */
void UTitleMenuWidget::ShowCharacterScreen()
{
	if (CharacterSelectWidget == nullptr)
	{
		SetStatusText(mCharacterSelectUnavailableText);
		ShowScreen(CharacterScreen);
		return;
	}

	CharacterSelectWidget->OpenCharacterSelect();
	ShowScreen(CharacterScreen);
}

/**
 * @brief 타이틀에서 공용 설정 패널을 Title 모드로 열어 보여준다.
 *
 * @details
 * 설정 기능은 타이틀과 인게임에서 같은 WBP_SettingsPanel을 공유한다.
 * 타이틀에서는 저장 후 종료/포기하기 같은 런 액션이 보이면 안 되므로 PanelMode를 Title로 바꾸고 Run 액션 상태를 비활성화한다.
 *
 * WBP_TitleMenu 안에 SettingsPanelWidget이 직접 있으면 ScreenSwitcher의 SettingsScreen으로 보여주고,
 * 없으면 FrontendGameMode가 월드 위젯으로 준비한 InGameSettings를 OpenUI()로 띄운다.
 */
void UTitleMenuWidget::ShowSettingsScreen()
{
	USettingsPanelWidget* TitleSettingsPanel = GetTitleSettingsPanel();
	if (TitleSettingsPanel == nullptr)
	{
		ShowMainScreen();
		SetStatusText(mMainOnlyStatusText);
		return;
	}

	TitleSettingsPanel->OnBackRequested.AddUniqueDynamic(this, &UTitleMenuWidget::HandleSettingsBackButtonClicked);
	TitleSettingsPanel->SetPanelMode(ESettingsPanelMode::Title);
	TitleSettingsPanel->RefreshPanelState(false, false);
	TitleSettingsPanel->HideAbandonConfirm();
	TitleSettingsPanel->SetStatusText(FText::GetEmpty());

	if (SettingsPanelWidget != nullptr && SettingsScreen != nullptr)
	{
		TitleSettingsPanel->SetVisibility(ESlateVisibility::Visible);
		ShowScreen(SettingsScreen);
	}
	else
	{
		ShowMainScreen();
		TitleSettingsPanel->OpenUI();
	}
	SetStatusText(mSettingsStatusText);
}

/**
 * @brief 타이틀 내부 지도 화면을 사용할 수 있는지 확인한다.
 *
 * @details
 * Continue 지도는 WBP_TitleMenu 안에 MapScreen과 WBP_FrontendMap이 직접 배치되어야 한다.
 * C++ fallback으로 임시 지도를 만들지 않는 이유는 WBP 디자인과 런타임 화면이 갈라지는 문제를 막기 위해서다.
 */
bool UTitleMenuWidget::EnsureMapScreen()
{
	if (MapScreen == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: MapScreen is not connected. Place the map screen inside WBP_TitleMenu."));
		return false;
	}

	if (FrontendMapWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: FrontendMapWidget is not connected. Place WBP_FrontendMap inside WBP_TitleMenu."));
		return false;
	}

	FrontendMapWidget->OnCloseRequested.AddUniqueDynamic(this, &UTitleMenuWidget::HandleMapBackRequested);
	return true;
}

/**
 * @brief Continue 지도 화면을 열고 현재 Run 기준으로 지도를 다시 그린다.
 *
 * @details
 * 지도 화면은 열릴 때마다 RefreshMap()을 호출한다.
 * 타이틀 메인에서 Continue 버튼 상태를 확인한 뒤 실제 지도 화면에 들어오기까지 Run 상태가 바뀔 수 있으므로,
 * 화면 진입 시점에 한 번 더 최신 View DTO를 받아온다.
 */
void UTitleMenuWidget::ShowMapScreen()
{
	if (!EnsureMapScreen())
	{
		ShowMainScreen();
		SetStatusText(mMainOnlyStatusText);
		return;
	}

	ShowScreen(MapScreen);
	FrontendMapWidget->RefreshMap();
	SetStatusText(FText::GetEmpty());
}

/**
 * @brief 생성자/에디터 기본값으로 준비된 문구를 실제 WBP TextBlock에 반영한다.
 *
 * @details
 * WBP는 레이아웃과 폰트/색을 담당하고, C++은 버튼 의미에 맞는 텍스트만 넣는다.
 * 텍스트 동기화를 한 함수로 모아두면 저장 슬롯/불러오기 버튼이 추가될 때 문구 갱신 지점이 분산되지 않는다.
 */
void UTitleMenuWidget::SyncMainText() const
{
	if (TitleText != nullptr)
	{
		TitleText->SetText(mTitleText);
	}

	if (StartButtonText != nullptr)
	{
		StartButtonText->SetText(mStartButtonText);
	}

	if (ContinueButtonText != nullptr)
	{
		ContinueButtonText->SetText(mContinueButtonText);
	}

	if (SettingsButtonText != nullptr)
	{
		SettingsButtonText->SetText(mSettingsButtonText);
	}

	if (SettingsBackButtonText != nullptr)
	{
		SettingsBackButtonText->SetText(mBackButtonText);
	}
}

/**
 * @brief 현재 활성 Run 여부에 따라 START/NEW START/CONTINUE 버튼 상태를 갱신한다.
 *
 * @details
 * 세이브 데이터 로드는 Intro 단계에서 끝난다는 전제를 사용한다.
 * 타이틀은 디스크를 다시 읽지 않고, FrontendGameMode가 현재 RunPersistData로 지도 View를 만들 수 있는지만 본다.
 *
 * 활성 Run이 없으면 Continue는 숨기고 START 문구를 보여준다.
 * 활성 Run이 있으면 Continue를 보여주고, 새로 시작은 NEW START 문구로 바꿔 기존 런과 별도 행동임을 드러낸다.
 */
void UTitleMenuWidget::RefreshMainMenuState() const
{
	const bool bCanContinueRun = TryLoadRunForMapScreen();

	if (StartButton != nullptr)
	{
		StartButton->SetVisibility(ESlateVisibility::Visible);
		StartButton->SetIsEnabled(true);
	}

	if (StartButtonText != nullptr)
	{
		StartButtonText->SetText(bCanContinueRun ? mNewStartButtonText : mStartButtonText);
	}

	if (ContinueButton != nullptr)
	{
		ContinueButton->SetVisibility(bCanContinueRun ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		ContinueButton->SetIsEnabled(bCanContinueRun);
	}

	if (ContinueButtonText != nullptr)
	{
		ContinueButtonText->SetText(mContinueButtonText);
	}

	if (SettingsButton != nullptr)
	{
		SettingsButton->SetVisibility(ESlateVisibility::Visible);
		SettingsButton->SetIsEnabled(true);
	}
}

/**
 * @brief 현재 프론트엔드 GameMode가 Continue 지도 데이터를 제공할 수 있는지 확인한다.
 *
 * @details
 * 함수 이름은 "Load"지만 여기서 SaveGame 파일을 직접 읽지는 않는다.
 * 이미 복구된 PersistentData를 지도 View로 만들 수 있는지만 확인하는 얇은 검사다.
 */
bool UTitleMenuWidget::TryLoadRunForMapScreen() const
{
	TArray<FFrontendMapRoomView> Rooms;
	if (AFrontendGameMode* FrontendGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<AFrontendGameMode>() : nullptr)
	{
		return FrontendGameMode->GetMapRoomViews(OUT Rooms);
	}

	return false;
}

/**
 * @brief 타이틀 설정에 사용할 SettingsPanelWidget 인스턴스를 찾는다.
 *
 * @details
 * 우선 WBP_TitleMenu 안에 직접 배치된 패널을 사용한다.
 * 아직 WBP가 완전히 정리되지 않은 경우에는 FrontendGameMode가 미리 만든 InGameSettings 월드 위젯을 대신 사용한다.
 * 두 경로 모두 같은 WBP_SettingsPanel 클래스를 바라보게 해 타이틀/인게임 설정 디자인이 갈라지지 않게 한다.
 */
USettingsPanelWidget* UTitleMenuWidget::GetTitleSettingsPanel() const
{
	if (SettingsPanelWidget != nullptr)
	{
		return SettingsPanelWidget;
	}

	UWorld* World = GetWorld();
	UWorldWidgetSubsystem* WorldWidgetSubsystem = World != nullptr ? World->GetSubsystem<UWorldWidgetSubsystem>() : nullptr;
	return WorldWidgetSubsystem != nullptr
		? WorldWidgetSubsystem->GetWorldWidget<USettingsPanelWidget>(EWorldWidgetType::InGameSettings)
		: nullptr;
}

/**
 * @brief 예전 상태 문구 영역을 항상 숨긴다.
 *
 * @details
 * 새 타이틀 WBP는 별도 상태 텍스트를 사용하지 않는다.
 * 기존 C++ 호출부 호환을 위해 함수는 남겨두지만, 어떤 텍스트가 들어와도 화면에는 표시하지 않는다.
 */
void UTitleMenuWidget::SetStatusText(const FText& /*InText*/) const
{
	if (StatusText != nullptr)
	{
		StatusText->SetText(FText::GetEmpty());
		StatusText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

/**
 * @brief WBP_TitleMenu가 C++이 기대하는 BindWidget 이름을 제공하는지 로그로 확인한다.
 *
 * @details
 * UI 담당자가 WBP 디자인을 고칠 때 가장 흔한 문제가 이름 변경으로 인한 BindWidget nullptr이다.
 * 여기서 누락된 이름을 한 번에 로그로 남겨야, 화면이 안 넘어가는 문제가 C++ 로직인지 WBP 연결 문제인지 빠르게 구분할 수 있다.
 */
void UTitleMenuWidget::ValidateDesignerBindings() const
{
	if (ScreenSwitcher == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: ScreenSwitcher is not connected."));
	}

	if (StartScreen == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: StartScreen is not connected."));
	}

	if (CharacterScreen == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: CharacterScreen is not connected."));
	}

	if (CharacterSelectWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: CharacterSelectWidget is not connected. Place WBP_CharacterSelect inside WBP_TitleMenu."));
	}

	if (MapScreen == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: MapScreen is not connected. Place the map screen in WBP_TitleMenu."));
	}

	if (FrontendMapWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: FrontendMapWidget is not connected. Place WBP_FrontendMap in the map screen."));
	}

	if (StartButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: StartButton is not connected."));
	}

	if (ContinueButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: ContinueButton is not connected."));
	}

	if (SettingsButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: SettingsButton is not connected."));
	}

	if (TitleText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: TitleText is not connected."));
	}

	if (StartButtonText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: StartButtonText is not connected."));
	}

	if (ContinueButtonText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: ContinueButtonText is not connected."));
	}

	if (SettingsButtonText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: SettingsButtonText is not connected."));
	}

	if (GetTitleSettingsPanel() == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: SettingsPanelWidget is not available. Place WBP_SettingsPanel in the title WBP or configure InGameSettings world widget."));
	}

	SetStatusText(FText::GetEmpty());
}

/**
 * @brief START 버튼 입력을 GameMode의 새 런 시작 흐름으로 전달한다.
 *
 * @details
 * GameMode가 준비되어 있으면 CreateNewRunFromTitle()을 통해 캐릭터 선택 화면을 열고,
 * 테스트/프리뷰처럼 GameMode가 없는 경우에는 WBP 화면 전환만 수행한다.
 */
void UTitleMenuWidget::HandleStartButtonClicked()
{
	if (AFrontendGameMode* FrontendGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<AFrontendGameMode>() : nullptr)
	{
		if (FrontendGameMode->CreateNewRunFromTitle())
		{
			return;
		}
	}

	ShowCharacterScreen();
}

/**
 * @brief CONTINUE 버튼 입력으로 현재 활성 Run의 지도 화면을 연다.
 *
 * @details
 * 버튼은 RefreshMainMenuState()에서 활성 Run이 있을 때만 보이지만,
 * 클릭 순간에도 다시 검사해 Run 데이터가 비었거나 지도 View를 만들 수 없으면 메인 화면으로 되돌린다.
 */
void UTitleMenuWidget::HandleContinueButtonClicked()
{
	if (TryLoadRunForMapScreen())
	{
		ShowMapScreen();
		return;
	}

	ShowMainScreen();
	SetStatusText(mMainOnlyStatusText);
}

/**
 * @brief SETTING 버튼 입력으로 타이틀 설정 화면을 연다.
 */
void UTitleMenuWidget::HandleSettingsButtonClicked()
{
	ShowSettingsScreen();
}

/**
 * @brief 설정 화면 Back 요청을 타이틀 메인 복귀로 처리한다.
 *
 * @details
 * 설정 패널이 WBP_TitleMenu 내부에 있으면 ScreenSwitcher만 메인으로 돌린다.
 * 월드 위젯 fallback으로 열었다면 CloseUI()까지 호출해 팝업 상태를 정리한다.
 */
void UTitleMenuWidget::HandleSettingsBackButtonClicked()
{
	if (SettingsPanelWidget == nullptr)
	{
		if (USettingsPanelWidget* TitleSettingsPanel = GetTitleSettingsPanel())
		{
			TitleSettingsPanel->CloseUI();
		}
	}

	ShowMainScreen();
	SetStatusText(FText::GetEmpty());
}

/**
 * @brief 캐릭터 선택 화면의 Back 요청을 타이틀 메인 복귀로 처리한다.
 */
void UTitleMenuWidget::HandleCharacterBackToMainRequested()
{
	ShowMainScreen();
	SetStatusText(FText::GetEmpty());
}

/**
 * @brief 지도 화면의 Close 요청을 타이틀 메인 복귀로 처리한다.
 *
 * @details
 * 지도 화면을 닫은 뒤에는 Continue 버튼 표시 여부를 다시 계산한다.
 * 지도 안에서 런 포기나 상태 변경이 들어올 수 있으므로 메인으로 돌아가기 전 메뉴 상태를 갱신한다.
 */
void UTitleMenuWidget::HandleMapBackRequested()
{
	RefreshMainMenuState();
	ShowMainScreen();
	SetStatusText(FText::GetEmpty());
}
