#include "UI/CombatTileMapHUDWidget.h"

#include "Components/Image.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "UI/FrontendMapWidget.h"
#include "UI/SettingsPanelWidget.h"

/*
 * 패널 내비게이션 + 승리 후 월드맵 흐름 (레거시 탑바에서 이관).
 *
 * 원래 UTopMenuBarWidget이 소유하던 두 책임을 전투 HUD로 옮긴 파일이다:
 * 1) 플로팅 패널(MAP/SET/DICE/SKILL) 토글 — 네 패널은 같은 "현재 조작 중인 팝업" 자리를
 *    공유하므로 새 패널을 열기 전에 나머지를 닫는다(상호배타).
 * 2) 승리 후 다음 방 선택 월드맵 강제 흐름 — 전투 승리 시 월드맵을 방 선택 가능 상태로 열고,
 *    사용자가 지도를 닫거나 설정을 열었다 닫아도 다음 방을 고를 때까지 지도를 복원한다.
 *
 * 스킨 HUD가 유일한 전투 화면이 되면서 concept 내비 버튼(HandleNav*Clicked)이 이 로직을
 * 직접 호출한다(탑바 경유 위임 제거).
 */

/**
 * @brief 전투 모델의 전투 종료 이벤트를 구독해 승리 후 월드맵 흐름을 시작한다.
 *
 * @details
 * BindCombatUIModel 시점(BeginRoom, InitCombat 이후)에 호출되므로 전투 모델이 항상 존재한다.
 * 재바인딩 시 이중 구독을 막기 위해 기존 구독을 먼저 해제한다.
 * 새 전투 시작 시 승리 잠금도 명시적으로 초기화한다(이전 방의 잠금이 승계되지 않게).
 */
void UCombatTileMapHUDWidget::BindVictoryFlowEvents()
{
	mVictoryWorldMapLocked = false;

	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	if (CombatModel == nullptr)
	{
		return;
	}

	CombatModel->OnEndCombatUI.RemoveAll(this);
	CombatModel->OnEndCombatUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, ESRPGCombatResult Result)
	{
		HandleEndCombatUI(MoveTemp(Barrier), Result);
	});
}

/**
 * @brief 월드 서브시스템에 등록된 월드 위젯을 URDUserWidget으로 가져온다.
 */
URDUserWidget* UCombatTileMapHUDWidget::GetToggleableWorldWidget(EWorldWidgetType WorldWidgetType) const
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

/**
 * @brief 지정한 월드 위젯을 공통 CloseUI() 경로로 닫는다.
 */
void UCombatTileMapHUDWidget::CloseWorldWidget(EWorldWidgetType WorldWidgetType) const
{
	if (URDUserWidget* Widget = GetToggleableWorldWidget(WorldWidgetType))
	{
		Widget->CloseUI();
	}
}

/**
 * @brief 플로팅 패널을 하나만 남기고 닫는다.
 *
 * @details
 * 월드맵, 설정, 주사위, 스킬 패널은 서로 겹쳐 열리는 화면이 아니다.
 * 새 패널을 열기 전에 나머지를 닫아야 Back/Close 입력과 터치 대상이 명확하다.
 */
void UCombatTileMapHUDWidget::CloseFloatingPanels(EWorldWidgetType ExceptWorldWidgetType) const
{
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
 * 같은 WorldMap 위젯이라도 열린 경로에 따라 선택 가능 여부가 달라지므로, 여는 쪽이
 * SetRoomSelectionEnabled()를 정해야 위젯 내부가 게임 흐름을 추측하지 않는다.
 */
void UCombatTileMapHUDWidget::ToggleWorldMap()
{
	UFrontendMapWidget* WorldMapWidget = Cast<UFrontendMapWidget>(GetToggleableWorldWidget(EWorldWidgetType::WorldMap));
	if (WorldMapWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CombatTileMapHUDWidget: WorldMap widget is not configured."));
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
	WorldMapWidget->OnCloseRequested.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleWorldMapCloseRequested);
	WorldMapWidget->SetRoomSelectionEnabled(false);
	WorldMapWidget->ClearMapStatusOverride();
	WorldMapWidget->OpenUI();
	WorldMapWidget->RefreshMap();
}

/**
 * @brief 인게임 설정 패널을 토글한다.
 *
 * @details
 * 설정 패널은 월드맵과 동시에 열지 않는다. 승리 후 지도 잠금 중이면 설정을 닫은 뒤 월드맵을 복원한다.
 * 인게임에서 연 설정은 항상 런 액션 영역 기준으로 시작해야 하므로 여는 쪽이 모드/상태 문구를 초기화한다.
 */
void UCombatTileMapHUDWidget::ToggleSettingsPanel()
{
	USettingsPanelWidget* SettingsPanelWidget = Cast<USettingsPanelWidget>(GetToggleableWorldWidget(EWorldWidgetType::InGameSettings));
	if (SettingsPanelWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CombatTileMapHUDWidget: InGameSettings widget is not configured."));
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
	SettingsPanelWidget->OnBackRequested.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleSettingsBackRequested);
	SettingsPanelWidget->OpenUI();
	SettingsPanelWidget->SetPanelMode(ESettingsPanelMode::InGame);
	SettingsPanelWidget->RefreshPanelState(false, false);
	SettingsPanelWidget->HideAbandonConfirm();
	SettingsPanelWidget->SetStatusText(FText::GetEmpty());
}

/**
 * @brief DICE/SKILL처럼 단순히 열고 닫는 플로팅 패널을 토글한다.
 *
 * @details
 * 해당 패널의 게임 로직(주사위 사용/스킬 발동)은 각 패널이 담당하고,
 * 이 함수는 월드 위젯 등록 누락 검사와 상호배타 OpenUI/CloseUI 생명주기만 책임진다.
 */
void UCombatTileMapHUDWidget::ToggleFloatingPanel(EWorldWidgetType WorldWidgetType, const TCHAR* DebugName)
{
	URDUserWidget* FloatingPanel = GetToggleableWorldWidget(WorldWidgetType);
	if (FloatingPanel == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CombatTileMapHUDWidget: %s widget is not configured."), DebugName);
		return;
	}

	if (FloatingPanel->IsOpened())
	{
		FloatingPanel->CloseUI();
		return;
	}

	CloseFloatingPanels(WorldWidgetType);
	FloatingPanel->OpenUI();
}

/**
 * @brief 플레이어 승리 결과를 다음 방 선택 월드맵 표시로 연결한다.
 */
void UCombatTileMapHUDWidget::HandleEndCombatUI(TSharedPtr<FPresentationBarrier> Barrier, ESRPGCombatResult Result)
{
	if (Result != ESRPGCombatResult::PlayerWin)
	{
		return;
	}

	mVictoryWorldMapLocked = true;
	OpenWorldMapAfterPlayerWin(MoveTemp(Barrier));
}

/**
 * @brief 승리 후 다음 방 선택이 가능한 상태로 월드맵을 연다.
 *
 * @details
 * OpenUI 완료 후 presentation barrier를 해제해 전투 종료 연출과 다음 방 선택 UI의 경계가
 * 끊기지 않게 한다. OpenUI 애니메이션이 있어도 내용은 먼저 채워져 있어야 하므로 즉시
 * RefreshMap()을 부르고, 완료 콜백에서 한 번 더 갱신한다. 완료 콜백이 실제로 접근하는
 * UObject는 월드맵 위젯이므로 약한 참조 검사 대상도 월드맵 위젯으로 둔다.
 */
void UCombatTileMapHUDWidget::OpenWorldMapAfterPlayerWin(TSharedPtr<FPresentationBarrier> Barrier)
{
	UFrontendMapWidget* WorldMapWidget = Cast<UFrontendMapWidget>(GetToggleableWorldWidget(EWorldWidgetType::WorldMap));
	if (WorldMapWidget == nullptr)
	{
		return;
	}

	CloseWorldWidget(EWorldWidgetType::InGameSettings);
	CloseWorldWidget(EWorldWidgetType::DicePanel);
	CloseWorldWidget(EWorldWidgetType::SkillPanel);
	WorldMapWidget->OnCloseRequested.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleWorldMapCloseRequested);
	WorldMapWidget->SetRoomSelectionEnabled(true);
	WorldMapWidget->SetMapStatusOverride(NSLOCTEXT("CombatTileMapHUDWidget", "VictoryMapStatus", "승리했습니다!"));
	WorldMapWidget->OpenUI(FOnEndUIOpenAnimation::CreateWeakLambda(WorldMapWidget, [Barrier](UUserWidget* OpenedWidget) mutable
	{
		if (UFrontendMapWidget* OpenedWorldMapWidget = Cast<UFrontendMapWidget>(OpenedWidget))
		{
			OpenedWorldMapWidget->RefreshMap();
		}
		Barrier.Reset();
	}));
	WorldMapWidget->RefreshMap();
}

/**
 * @brief 승리 후 지도 잠금이 유지되는 동안 월드맵을 다시 연다.
 *
 * @details
 * 승리 후 다음 방을 고르기 전까지 월드맵은 흐름상 필수 UI다.
 * 사용자가 설정을 잠깐 열 수는 있지만, 닫힌 뒤에는 다시 다음 방 선택 지도 상태로 복원한다.
 */
void UCombatTileMapHUDWidget::RestoreVictoryWorldMap()
{
	if (!mVictoryWorldMapLocked)
	{
		return;
	}

	OpenWorldMapAfterPlayerWin(TSharedPtr<FPresentationBarrier>());
}

/**
 * @brief 설정 패널이 열려 있으면 닫힘 완료 뒤 승리 후 월드맵을 복원한다.
 *
 * @details
 * 설정 패널 닫기 애니메이션이 있다면 그 애니메이션이 끝난 뒤 월드맵을 복원해,
 * 두 팝업이 동시에 떠 있는 순간을 줄인다.
 */
void UCombatTileMapHUDWidget::CloseSettingsPanelAndRestoreVictoryWorldMap()
{
	if (URDUserWidget* SettingsPanelWidget = GetToggleableWorldWidget(EWorldWidgetType::InGameSettings);
		SettingsPanelWidget != nullptr && SettingsPanelWidget->IsOpened())
	{
		SettingsPanelWidget->CloseUI(FOnEndUICloseAnimation::CreateWeakLambda(this, [this](UUserWidget*)
		{
			RestoreVictoryWorldMap();
		}));
		return;
	}

	RestoreVictoryWorldMap();
}

void UCombatTileMapHUDWidget::UpdateTopBarBackdrop() const
{
	if (TopBar_Backdrop == nullptr)
	{
		return;
	}

	const URDUserWidget* WorldMapWidget = GetToggleableWorldWidget(EWorldWidgetType::WorldMap);
	const bool bWorldMapOpened = WorldMapWidget != nullptr && WorldMapWidget->IsOpened();
	const ESlateVisibility Desired = bWorldMapOpened ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (TopBar_Backdrop->GetVisibility() != Desired)
	{
		TopBar_Backdrop->SetVisibility(Desired);
	}
}

/**
 * @brief 월드맵 닫기 요청을 현재 지도 잠금 상태에 맞게 처리한다.
 *
 * @details
 * 일반 MAP 버튼으로 연 지도는 Close 요청을 그대로 닫고,
 * 승리 후 지도는 다음 방 선택을 끝내기 전까지 닫히면 안 되므로 복원으로 되돌린다.
 */
void UCombatTileMapHUDWidget::HandleWorldMapCloseRequested()
{
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
void UCombatTileMapHUDWidget::HandleSettingsBackRequested()
{
	if (mVictoryWorldMapLocked)
	{
		CloseSettingsPanelAndRestoreVictoryWorldMap();
		return;
	}

	CloseWorldWidget(EWorldWidgetType::InGameSettings);
}
