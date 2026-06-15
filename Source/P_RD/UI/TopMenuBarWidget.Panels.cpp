#include "UI/TopMenuBarWidget.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/FrontendMapWidget.h"
#include "UI/SettingsPanelWidget.h"

/**
 * @brief 월드 서브시스템에 등록된 월드 위젯을 URDUserWidget으로 가져온다.
 */
URDUserWidget* UTopMenuBarWidget::GetToggleableWorldWidget(EWorldWidgetType WorldWidgetType) const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	const UWorldWidgetSubsystem* WorldWidgetSubsystem = World->GetSubsystem<UWorldWidgetSubsystem>();
	if (WorldWidgetSubsystem == nullptr)
	{
		return nullptr;
	}

	return Cast<URDUserWidget>(WorldWidgetSubsystem->GetWorldWidget(WorldWidgetType));
}

void UTopMenuBarWidget::UnbindPanelEvents()
{
	if (UFrontendMapWidget* WorldMapWidget = Cast<UFrontendMapWidget>(GetToggleableWorldWidget(EWorldWidgetType::WorldMap)))
	{
		WorldMapWidget->OnCloseRequested.RemoveDynamic(this, &UTopMenuBarWidget::HandleWorldMapCloseRequested);
	}

	if (USettingsPanelWidget* SettingsPanelWidget = Cast<USettingsPanelWidget>(GetToggleableWorldWidget(EWorldWidgetType::InGameSettings)))
	{
		SettingsPanelWidget->OnBackRequested.RemoveDynamic(this, &UTopMenuBarWidget::HandleSettingsBackRequested);
	}
}

/**
 * @brief 지정한 월드 위젯을 공통 CloseUI() 경로로 닫는다.
 */
void UTopMenuBarWidget::CloseWorldWidget(EWorldWidgetType WorldWidgetType) const
{
	if (URDUserWidget* Widget = GetToggleableWorldWidget(WorldWidgetType))
	{
		Widget->CloseUI();
	}
}

/**
 * @brief 탑바 플로팅 패널을 하나만 남기고 닫는다.
 *
 * @details
 * 월드맵, 설정, 주사위, 스킬 패널은 서로 겹쳐 열리는 화면이 아니다.
 * 새 패널을 열기 전에 나머지를 닫아야 Back/Close 입력과 터치 대상이 명확하다.
 */
void UTopMenuBarWidget::CloseFloatingPanels(EWorldWidgetType ExceptWorldWidgetType) const
{
	/*
	 * 탑바에서 여는 네 패널은 모두 같은 "현재 조작 중인 팝업" 자리를 공유한다.
	 * 예를 들어 MAP 위에 DICE가 겹치면 아래 지도 버튼이 눌리는지, 위 주사위가 눌리는지 애매해진다.
	 * 그래서 새 패널을 열기 전에 나머지 패널을 명시적으로 닫는다.
	 */
	constexpr EWorldWidgetType FloatingPanels[] = {
		EWorldWidgetType::WorldMap,
		EWorldWidgetType::InGameSettings,
		EWorldWidgetType::DicePanel,
		EWorldWidgetType::SkillPanel
	};

	for (const EWorldWidgetType FloatingPanel : FloatingPanels)
	{
		if (FloatingPanel != ExceptWorldWidgetType)
		{
			CloseWorldWidget(FloatingPanel);
		}
	}
}

/**
 * @brief 일반 맵 조회와 승리 후 다음 방 선택 맵을 구분해 월드맵을 토글한다.
 *
 * @details
 * 사용자가 MAP 버튼으로 연 지도는 조회용이므로 방 선택을 막는다.
 * 승리 후 지도 잠금 상태에서는 다음 방 선택을 유지해야 하므로 닫기 대신 복원 흐름으로 보낸다.
 *
 * 왜 탑바가 지도 모드를 정하는가:
 * 같은 WorldMap 위젯이라도 MAP 버튼에서 열렸는지, 전투 승리 후 열렸는지에 따라 선택 가능 여부가 달라진다.
 * 열린 경로를 알고 있는 탑바가 SetRoomSelectionEnabled()를 정해야 위젯 내부가 게임 흐름을 추측하지 않는다.
 */
void UTopMenuBarWidget::ToggleWorldMap()
{
	RefreshRoomInfo();

	UFrontendMapWidget* WorldMapWidget = Cast<UFrontendMapWidget>(GetToggleableWorldWidget(EWorldWidgetType::WorldMap));
	if (WorldMapWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TopMenuBarWidget: WorldMap widget is not configured."));
		return;
	}

	if (mVictoryWorldMapLocked)
	{
		CloseSettingsPanelAndRestoreVictoryWorldMap();
		return;
	}

	if (WorldMapWidget->IsOpened())
	{
		WorldMapWidget->CloseUI();
		return;
	}

	CloseFloatingPanels(EWorldWidgetType::WorldMap);
	WorldMapWidget->OnCloseRequested.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleWorldMapCloseRequested);
	WorldMapWidget->SetRoomSelectionEnabled(false);
	WorldMapWidget->ClearMapStatusOverride();
	WorldMapWidget->OpenUI();
	WorldMapWidget->RefreshMap();
	ApplyInputPassThrough();
}

/**
 * @brief 인게임 설정 패널을 토글한다.
 *
 * @details
 * 설정 패널은 월드맵과 동시에 열지 않는다. 승리 후 지도 선택이 잠겨 있으면 설정을 닫은 뒤 월드맵을 다시 띄운다.
 *
 * 왜 설정 패널 상태를 여기서 초기화하는가:
 * 인게임 SET 버튼에서 열린 설정은 항상 런 액션 영역을 기준으로 시작해야 한다.
 * TopMenuBar가 모드와 상태 문구를 정리해주면 SettingsPanelWidget은 어느 화면에서 열렸는지 직접 추측하지 않아도 된다.
 */
void UTopMenuBarWidget::ToggleSettingsPanel()
{
	RefreshRoomInfo();

	USettingsPanelWidget* SettingsPanelWidget = Cast<USettingsPanelWidget>(GetToggleableWorldWidget(EWorldWidgetType::InGameSettings));
	if (SettingsPanelWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TopMenuBarWidget: InGameSettings widget is not configured."));
		return;
	}

	if (SettingsPanelWidget->IsOpened())
	{
		if (mVictoryWorldMapLocked)
		{
			CloseSettingsPanelAndRestoreVictoryWorldMap();
			return;
		}

		SettingsPanelWidget->CloseUI();
		return;
	}

	CloseFloatingPanels(EWorldWidgetType::InGameSettings);
	SettingsPanelWidget->OnBackRequested.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleSettingsBackRequested);
	SettingsPanelWidget->OpenUI();
	SettingsPanelWidget->SetPanelMode(ESettingsPanelMode::InGame);
	SettingsPanelWidget->RefreshPanelState(false, false);
	SettingsPanelWidget->HideAbandonConfirm();
	SettingsPanelWidget->SetStatusText(FText::GetEmpty());
	ApplyInputPassThrough();
}

/**
 * @brief 단순 플로팅 패널을 토글한다.
 *
 * @details
 * DicePanel/SkillPanel은 현재 단계에서 별도 게임 로직을 실행하지 않는다.
 * 하지만 WBP가 존재하므로 버튼을 누르면 실제 패널이 OpenUI/CloseUI 흐름으로 열리고 닫혀야 한다.
 */
void UTopMenuBarWidget::ToggleFloatingPanel(EWorldWidgetType WorldWidgetType, const TCHAR* DebugName)
{
	/*
	 * DICE/SKILL은 지금 단계에서 "버튼을 누르면 해당 WBP 패널이 열린다"까지만 담당한다.
	 * 실제 주사위 굴림이나 스킬 발동은 아직 연결하지 않는다.
	 * 이 함수가 하는 일은 월드 위젯 등록 누락을 검사하고, 다른 팝업을 닫은 뒤 OpenUI/CloseUI 생명주기를 태우는 것이다.
	 */
	RefreshRoomInfo();

	URDUserWidget* FloatingPanel = GetToggleableWorldWidget(WorldWidgetType);
	if (FloatingPanel == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TopMenuBarWidget: %s widget is not configured."), DebugName);
		return;
	}

	if (FloatingPanel->IsOpened())
	{
		FloatingPanel->CloseUI();
		return;
	}

	CloseFloatingPanels(WorldWidgetType);
	FloatingPanel->OpenUI();
	ApplyInputPassThrough();
}

/**
 * @brief MAP 버튼 클릭을 월드맵 토글로 연결한다.
 */
void UTopMenuBarWidget::HandleMapButtonClicked()
{
	ToggleWorldMap();
}

/**
 * @brief SET 버튼 클릭을 설정 패널 토글로 연결한다.
 */
void UTopMenuBarWidget::HandleSettingsButtonClicked()
{
	ToggleSettingsPanel();
}

void UTopMenuBarWidget::HandleDiceButtonClicked()
{
	/*
	 * 주사위 버튼은 WBP_DicePanel 표시만 담당한다.
	 * 주사위 수량/사용 가능 여부가 붙으면 이 버튼은 패널을 여는 역할을 유지하고, 실제 검증은 DicePanel 또는 별도 주사위 시스템으로 위임한다.
	 */
	ToggleFloatingPanel(EWorldWidgetType::DicePanel, TEXT("DicePanel"));
}

void UTopMenuBarWidget::HandleSkillButtonClicked()
{
	/*
	 * 스킬 버튼은 WBP_SkillPanel 표시만 담당한다.
	 * 아직 "스킬 사용"이 아니라 "스킬 패널 UI를 열 수 있는지"를 검증하는 단계라서 버튼 라벨도 SKILL 0으로 고정한다.
	 */
	ToggleFloatingPanel(EWorldWidgetType::SkillPanel, TEXT("SkillPanel"));
}

/**
 * @brief 월드맵 닫기 요청을 현재 지도 잠금 상태에 맞게 처리한다.
 */
void UTopMenuBarWidget::HandleWorldMapCloseRequested()
{
	/*
	 * 일반 MAP 버튼으로 연 지도는 Close 요청을 그대로 닫는다.
	 * 승리 후 지도는 다음 방 선택을 끝내기 전까지 닫히면 안 되므로 RestoreVictoryWorldMap()으로 되돌린다.
	 */
	if (mVictoryWorldMapLocked)
	{
		RestoreVictoryWorldMap();
		return;
	}

	CloseWorldWidget(EWorldWidgetType::WorldMap);
}

/**
 * @brief 설정 패널의 Back 요청을 현재 지도 잠금 상태에 맞게 처리한다.
 */
void UTopMenuBarWidget::HandleSettingsBackRequested()
{
	/*
	 * 설정 패널 Back도 현재 흐름에 따라 처리한다.
	 * 일반 방 상태에서는 설정만 닫고, 승리 후 지도 잠금 상태에서는 설정을 닫은 뒤 다음 방 선택 지도를 다시 보여준다.
	 */
	if (mVictoryWorldMapLocked)
	{
		CloseSettingsPanelAndRestoreVictoryWorldMap();
		return;
	}

	CloseWorldWidget(EWorldWidgetType::InGameSettings);
}
