#include "UI/CombatTileMapHUDWidget.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/FrontendMapWidget.h"
#include "UI/SettingsPanelWidget.h"

URDUserWidget* UCombatTileMapHUDWidget::GetCombatFloatingPanel(EWorldWidgetType PanelType) const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	const UWorldWidgetSubsystem* WorldWidgetSubsystem = World->GetSubsystem<UWorldWidgetSubsystem>();
	return WorldWidgetSubsystem != nullptr
		? WorldWidgetSubsystem->GetWorldWidget<URDUserWidget>(PanelType)
		: nullptr;
}

void UCombatTileMapHUDWidget::CloseCombatFloatingPanel(EWorldWidgetType PanelType) const
{
	if (URDUserWidget* PanelWidget = GetCombatFloatingPanel(PanelType))
	{
		PanelWidget->CloseUI();
	}
}

void UCombatTileMapHUDWidget::CloseCombatFloatingPanels(EWorldWidgetType ExceptPanelType) const
{
	constexpr EWorldWidgetType FloatingPanels[] = {
		EWorldWidgetType::WorldMap,
		EWorldWidgetType::InGameSettings,
		EWorldWidgetType::DicePanel,
		EWorldWidgetType::SkillPanel
	};

	for (const EWorldWidgetType PanelType : FloatingPanels)
	{
		if (PanelType != ExceptPanelType)
		{
			CloseCombatFloatingPanel(PanelType);
		}
	}
}

void UCombatTileMapHUDWidget::ToggleWorldMapPanel()
{
	UFrontendMapWidget* WorldMapWidget = Cast<UFrontendMapWidget>(GetCombatFloatingPanel(EWorldWidgetType::WorldMap));
	if (WorldMapWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("Combat HUD: WorldMap widget is not configured."));
		return;
	}

	if (WorldMapWidget->IsOpened())
	{
		WorldMapWidget->CloseUI();
		return;
	}

	CloseCombatFloatingPanels(EWorldWidgetType::WorldMap);
	WorldMapWidget->OnCloseRequested.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleWorldMapCloseRequested);
	WorldMapWidget->SetRoomSelectionEnabled(false);
	WorldMapWidget->ClearMapStatusOverride();
	WorldMapWidget->OpenUI();
	WorldMapWidget->RefreshMap();
}

void UCombatTileMapHUDWidget::ToggleSimpleCombatPanel(EWorldWidgetType PanelType, const TCHAR* PanelLogName)
{
	URDUserWidget* PanelWidget = GetCombatFloatingPanel(PanelType);
	if (PanelWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("Combat HUD: %s widget is not configured."), PanelLogName);
		return;
	}

	if (PanelWidget->IsOpened())
	{
		PanelWidget->CloseUI();
		return;
	}

	CloseCombatFloatingPanels(PanelType);
	PanelWidget->OpenUI();
}

void UCombatTileMapHUDWidget::ToggleSettingsPanel()
{
	USettingsPanelWidget* SettingsPanelWidget = Cast<USettingsPanelWidget>(GetCombatFloatingPanel(EWorldWidgetType::InGameSettings));
	if (SettingsPanelWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("Combat HUD: InGameSettings widget is not configured."));
		return;
	}

	if (SettingsPanelWidget->IsOpened())
	{
		SettingsPanelWidget->CloseUI();
		return;
	}

	CloseCombatFloatingPanels(EWorldWidgetType::InGameSettings);
	SettingsPanelWidget->OnBackRequested.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleSettingsPanelBackRequested);
	SettingsPanelWidget->OpenUI();
	SettingsPanelWidget->SetPanelMode(ESettingsPanelMode::InGame);
	SettingsPanelWidget->RefreshPanelState(false, false);
	SettingsPanelWidget->HideAbandonConfirm();
	SettingsPanelWidget->SetStatusText(FText::GetEmpty());
}

void UCombatTileMapHUDWidget::HandleMapNavButtonClicked()
{
	ToggleWorldMapPanel();
}

void UCombatTileMapHUDWidget::HandleDiceNavButtonClicked()
{
	ToggleSimpleCombatPanel(EWorldWidgetType::DicePanel, TEXT("DicePanel"));
}

void UCombatTileMapHUDWidget::HandleSkillNavButtonClicked()
{
	ToggleSimpleCombatPanel(EWorldWidgetType::SkillPanel, TEXT("SkillPanel"));
}

void UCombatTileMapHUDWidget::HandleSettingsNavButtonClicked()
{
	ToggleSettingsPanel();
}

void UCombatTileMapHUDWidget::HandleWorldMapCloseRequested()
{
	CloseCombatFloatingPanel(EWorldWidgetType::WorldMap);
}

void UCombatTileMapHUDWidget::HandleSettingsPanelBackRequested()
{
	CloseCombatFloatingPanel(EWorldWidgetType::InGameSettings);
}
