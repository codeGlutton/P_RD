#include "UI/TitleMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "GameMode/FrontendGameMode.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/CharacterSelectWidget.h"
#include "UI/SettingsPanelWidget.h"

namespace
{
	constexpr int32 TitleMainScreenIndex = 0;
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
 * 캐릭터 선택이나 공용 설정 패널의 Back/Close 요청은 모두 이 함수로 모인다.
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
 * 설정 기능은 타이틀과 인게임에서 같은 WBP_SettingsPanel 월드 위젯을 공유한다.
 * 타이틀에서는 저장 후 종료/포기하기 같은 런 액션이 보이면 안 되므로 PanelMode를 Title로 바꾸고 Run 액션 상태를 비활성화한다.
 *
 * 왜 타이틀 내부 ScreenSwitcher로 보여주지 않는가:
 * 설정 패널은 HUD 하위 화면이 아니라 공용 팝업이다. InGameSettings 월드 위젯을 OpenUI()로 열어야
 * 타이틀/인게임 모두 AddToViewport, ZOrder, Back/Close 처리 규칙을 같은 경로로 검증할 수 있다.
 */
void UTitleMenuWidget::OpenSettingsPanel()
{
	USettingsPanelWidget* TitleSettingsPanel = GetTitleSettingsPanel();
	if (TitleSettingsPanel == nullptr)
	{
		ShowMainScreen();
		SetStatusText(mMainOnlyStatusText);
		return;
	}

	TitleSettingsPanel->OnBackRequested.AddUniqueDynamic(this, &UTitleMenuWidget::HandleSettingsPanelBackRequested);
	TitleSettingsPanel->SetPanelMode(ESettingsPanelMode::Title);
	TitleSettingsPanel->RefreshPanelState(false, false);
	TitleSettingsPanel->HideAbandonConfirm();
	TitleSettingsPanel->SetStatusText(FText::GetEmpty());

	ShowMainScreen();
	TitleSettingsPanel->OpenUI();
	SetStatusText(FText::GetEmpty());
}

/**
 * @brief 현재 활성 Run 여부에 따라 START/NEW START/CONTINUE 버튼 상태를 갱신한다.
 *
 * @details
 * 세이브 데이터 로드는 Intro 단계에서 끝난다는 전제를 사용한다.
 * 타이틀은 디스크를 다시 읽지 않고, FrontendGameMode가 현재 RunPersistData 요약을 만들 수 있는지만 본다.
 *
 * 활성 Run이 없으면 Continue는 숨기고 START 문구를 보여준다.
 * 활성 Run이 있으면 Continue를 보여주고, 새로 시작은 NEW START 문구로 바꿔 기존 런과 별도 행동임을 드러낸다.
 */
void UTitleMenuWidget::RefreshMainMenuState() const
{
	const bool bCanContinueRun = CanContinueRun();

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

	RefreshTitleButtonBackgroundVisibility(bCanContinueRun);
}

/**
 * @brief 현재 프론트엔드 GameMode가 이어가기 가능한 활성 Run을 갖는지 확인한다.
 *
 * @details
 * 여기서 SaveGame 파일을 직접 읽지는 않는다.
 * Intro에서 복구된 PersistentData에 활성 Run이 있는지만 확인해 CONTINUE 버튼 표시 여부를 정한다.
 * 현재 방 위치, 난이도, 레벨 같은 방 안 UI 표시는 실제 방에 들어간 뒤 RoomGameMode가 처리한다.
 */
bool UTitleMenuWidget::CanContinueRun() const
{
	if (AFrontendGameMode* FrontendGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<AFrontendGameMode>() : nullptr)
	{
		return FrontendGameMode->HasActiveRun();
	}

	return false;
}

/**
 * @brief 타이틀 설정에 사용할 공용 SettingsPanelWidget 월드 위젯을 찾는다.
 *
 * @details
 * 타이틀 WBP 안에 설정 패널을 직접 소유하지 않는다.
 * WorldWidgetSubsystem이 준비한 InGameSettings 월드 위젯만 받아와 OpenUI()/CloseUI() 생명주기를 통일한다.
 */
USettingsPanelWidget* UTitleMenuWidget::GetTitleSettingsPanel() const
{
	UWorld* World = GetWorld();
	UWorldWidgetSubsystem* WorldWidgetSubsystem = World != nullptr ? World->GetSubsystem<UWorldWidgetSubsystem>() : nullptr;
	return WorldWidgetSubsystem != nullptr
		? WorldWidgetSubsystem->GetWorldWidget<USettingsPanelWidget>(EWorldWidgetType::InGameSettings)
		: nullptr;
}

/**
 * @brief START 버튼 입력을 GameMode의 새 런 시작 흐름으로 전달한다.
 *
 * @details
 * GameMode가 준비되어 있으면 RequestCharacterSelectFromTitle()을 통해 캐릭터 선택 화면을 열고,
 * 테스트/프리뷰처럼 GameMode가 없는 경우에는 WBP 화면 전환만 수행한다.
 */
void UTitleMenuWidget::HandleStartButtonClicked()
{
	if (AFrontendGameMode* FrontendGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<AFrontendGameMode>() : nullptr)
	{
		if (FrontendGameMode->RequestCharacterSelectFromTitle())
		{
			return;
		}
	}

	ShowCharacterScreen();
}

/**
 * @brief CONTINUE 버튼 입력으로 현재 활성 Run의 방에 바로 들어간다.
 *
 * @details
 * 버튼은 RefreshMainMenuState()에서 활성 Run이 있을 때만 보이지만,
 * 클릭 순간에도 다시 검사해 Run 데이터가 비었거나 방 전환을 시작할 수 없으면 메인 화면으로 되돌린다.
 * 지도 조회/다음 방 선택은 방에 들어간 뒤 RoomGameMode가 준비한 WorldMap 위젯에서만 처리한다.
 */
void UTitleMenuWidget::HandleContinueButtonClicked()
{
	if (AFrontendGameMode* FrontendGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<AFrontendGameMode>() : nullptr)
	{
		if (FrontendGameMode->ContinueRunFromTitle())
		{
			return;
		}
	}

	ShowMainScreen();
	SetStatusText(mMainOnlyStatusText);
}

/**
 * @brief SETTING 버튼 입력으로 공용 설정 패널을 연다.
 */
void UTitleMenuWidget::HandleSettingsButtonClicked()
{
	OpenSettingsPanel();
}

/**
 * @brief 설정 화면 Back 요청을 타이틀 메인 복귀로 처리한다.
 *
 * @details
 * 타이틀 설정도 공용 InGameSettings 월드 위젯으로 열리므로 CloseUI()까지 호출해 팝업 상태를 정리한다.
 */
void UTitleMenuWidget::HandleSettingsPanelBackRequested()
{
	if (USettingsPanelWidget* TitleSettingsPanel = GetTitleSettingsPanel())
	{
		TitleSettingsPanel->CloseUI();
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
