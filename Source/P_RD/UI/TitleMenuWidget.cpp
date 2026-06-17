#include "UI/TitleMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/CharacterSelectWidget.h"
#include "UI/SettingsPanelWidget.h"

UTitleMenuWidget::UTitleMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, mTitleText(NSLOCTEXT("TitleMenuWidget", "TitleText", "Rogue The Dice"))
	, mStartButtonText(NSLOCTEXT("TitleMenuWidget", "StartText", "START"))
	, mNewStartButtonText(NSLOCTEXT("TitleMenuWidget", "NewStartText", "NEW START"))
	, mContinueButtonText(NSLOCTEXT("TitleMenuWidget", "ContinueText", "CONTINUE"))
	, mSettingsButtonText(NSLOCTEXT("TitleMenuWidget", "SettingsText", "SETTING"))
	, mMainOnlyStatusText(NSLOCTEXT("TitleMenuWidget", "MainOnlyStatusText", "Title main screen only"))
	, mCharacterSelectUnavailableText(NSLOCTEXT("TitleMenuWidget", "CharacterSelectUnavailableText", "Character select widget is not ready"))
{
	/*
	 * 타이틀 HUD는 방 진입 직후 바로 보이는 메인 UI다.
	 * 기본 Visibility를 Visible로 맞춰두되, 실제 AddToViewport/표시 생명주기는 RDUserWidget::OpenUI()가 처리한다.
	 */
	SetVisibility(ESlateVisibility::Visible);

	// 타이틀 아트(로고/버튼 텍스처)와 배치는 이제 WBP_TitleMenu가 보유한다.
	// C++ 생성자는 버튼/타이틀 텍스트 기본 문구만 정한다(위 초기화 리스트).
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
 * AFrontendGameMode::RequestCharacterSelectFromTitle()은 런 생성 대신 이 함수를 통해 캐릭터 선택 화면만 연다.
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
 * WBP_TitleMenu는 StartScreen과 CharacterScreen을 ScreenSwitcher 안에 직접 배치한다.
 * C++은 해당 화면을 새로 만들지 않고, BindWidget으로 들어온 인스턴스를 연결만 한다.
 *
 * 왜 여기서 공용 설정 패널/캐릭터 선택 이벤트까지 연결하는가:
 * 각 하위 위젯은 "뒤로 가기"나 "닫기 요청"만 외부로 알린다.
 * 실제로 타이틀 메인 화면으로 돌아갈지는 바깥 화면 전환을 알고 있는 TitleMenuWidget이 결정한다.
 */
void UTitleMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ValidateDesignerBindings();
	StartTitleBackgroundVideo();

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

	if (USettingsPanelWidget* TitleSettingsPanel = GetTitleSettingsPanel())
	{
		TitleSettingsPanel->OnBackRequested.AddUniqueDynamic(this, &UTitleMenuWidget::HandleSettingsPanelBackRequested);
	}

	if (CharacterSelectWidget != nullptr)
	{
		CharacterSelectWidget->OnBackToMainRequested.AddUniqueDynamic(this, &UTitleMenuWidget::HandleCharacterBackToMainRequested);
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
	StopTitleBackgroundVideo();

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

	if (USettingsPanelWidget* TitleSettingsPanel = GetTitleSettingsPanel())
	{
		TitleSettingsPanel->OnBackRequested.RemoveDynamic(this, &UTitleMenuWidget::HandleSettingsPanelBackRequested);
	}

	if (CharacterSelectWidget != nullptr)
	{
		CharacterSelectWidget->OnBackToMainRequested.RemoveDynamic(this, &UTitleMenuWidget::HandleCharacterBackToMainRequested);
	}

	Super::NativeDestruct();
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

}

/**
 * @brief 현재 타이틀 화면에서 사용하지 않는 상태 문구 영역을 항상 숨긴다.
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
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: InGameSettings world widget is not configured."));
	}

	SetStatusText(FText::GetEmpty());
}
