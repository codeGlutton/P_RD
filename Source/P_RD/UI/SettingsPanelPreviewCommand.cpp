/*****************************************************************//**
 * @file   SettingsPanelPreviewCommand.cpp
 * @brief  저장 데이터에 손대지 않고 공용 설정 패널의 타이틀/인게임 모드를 띄우는 개발용 명령.
 *********************************************************************/

#include "RDMinimal.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/SettingsPanelWidget.h"

#if !UE_BUILD_SHIPPING

namespace SettingsPanelPreview
{
	USettingsPanelWidget* GetOrInitSettingsPanel(UWorld* World)
	{
		if (World == nullptr || World->IsGameWorld() == false)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.SettingsPreview: 게임 월드를 찾지 못했습니다."));
			return nullptr;
		}

		UWorldWidgetSubsystem* WorldWidgetSubsystem =
			World->GetSubsystem<UWorldWidgetSubsystem>();
		if (WorldWidgetSubsystem == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.SettingsPreview: WorldWidgetSubsystem을 찾지 못했습니다."));
			return nullptr;
		}

		USettingsPanelWidget* Settings =
			WorldWidgetSubsystem->GetWorldWidget<USettingsPanelWidget>(
				EWorldWidgetType::InGameSettings);
		if (Settings == nullptr)
		{
			WorldWidgetSubsystem->InitWorldWidget(EWorldWidgetType::InGameSettings);
			Settings = WorldWidgetSubsystem->GetWorldWidget<USettingsPanelWidget>(
				EWorldWidgetType::InGameSettings);
		}

		if (Settings == nullptr)
		{
			UE_LOG(LogRD, Warning,
				TEXT("RD.SettingsPreview: InGameSettings 월드 위젯을 초기화하지 못했습니다."));
		}
		return Settings;
	}

	void Show(
		UWorld* World,
		ESettingsPanelMode PanelMode,
		bool bCanSaveRun,
		bool bCanAbandonRun,
		const TCHAR* ModeLabel)
	{
		USettingsPanelWidget* Settings = GetOrInitSettingsPanel(World);
		if (Settings == nullptr)
		{
			return;
		}

		// 타이틀 메뉴/전투 HUD의 실제 열기 순서와 같은 표시 상태만 적용한다.
		// 저장 및 런 진행 델리게이트는 연결하지 않아 프리뷰 명령 자체는 데이터를 변경하지 않는다.
		Settings->SetPanelMode(PanelMode);
		Settings->RefreshPanelState(bCanSaveRun, bCanAbandonRun);
		Settings->HideAbandonConfirm();
		Settings->SetStatusText(FText::GetEmpty());
		Settings->OpenUI();

		UE_LOG(LogRD, Display,
			TEXT("RD.SettingsPreview.%s: 저장 데이터 변경 없이 설정 패널을 열었습니다."),
			ModeLabel);
	}

	void ShowTitle(UWorld* World)
	{
		Show(World, ESettingsPanelMode::Title, false, false, TEXT("Title"));
	}

	void ShowInGame(UWorld* World)
	{
		Show(World, ESettingsPanelMode::InGame, true, true, TEXT("InGame"));
	}

	FAutoConsoleCommandWithWorld ShowTitleCommand(
		TEXT("RD.SettingsPreview.Title"),
		TEXT("저장 데이터 변경 없이 공용 설정 패널을 타이틀 모드로 연다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&ShowTitle));

	FAutoConsoleCommandWithWorld ShowInGameCommand(
		TEXT("RD.SettingsPreview.InGame"),
		TEXT("저장 데이터 변경 없이 공용 설정 패널을 인게임 모드로 연다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&ShowInGame));
}

#endif
