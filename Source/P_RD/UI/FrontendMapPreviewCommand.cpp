/**
 * @file FrontendMapPreviewCommand.cpp
 * @brief 저장 상태를 바꾸지 않고 현재 런 지도를 조회 모드로 여는 개발 명령.
 */

#include "RDMinimal.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "UI/FrontendMapWidget.h"

#if !UE_BUILD_SHIPPING

namespace FrontendMapPreview
{
	TWeakObjectPtr<UFrontendMapWidget> ShownWidget;

	void Toggle(UWorld* World)
	{
		if (ShownWidget.IsValid() && ShownWidget->IsOpened())
		{
			ShownWidget->CloseUI();
			ShownWidget.Reset();
			UE_LOG(LogRD, Display, TEXT("RD.MapPreview: 지도를 닫았습니다."));
			return;
		}

		if (World == nullptr || World->IsGameWorld() == false)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.MapPreview: 게임 월드를 찾지 못했습니다."));
			return;
		}

		UWorldWidgetSubsystem* WidgetSubsystem =
			World->GetSubsystem<UWorldWidgetSubsystem>();
		if (WidgetSubsystem == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.MapPreview: 월드 위젯 서브시스템이 없습니다."));
			return;
		}

		UFrontendMapWidget* MapWidget =
			WidgetSubsystem->GetWorldWidget<UFrontendMapWidget>(
				EWorldWidgetType::WorldMap);
		if (MapWidget == nullptr)
		{
			WidgetSubsystem->InitWorldWidget(EWorldWidgetType::WorldMap);
			MapWidget = WidgetSubsystem->GetWorldWidget<UFrontendMapWidget>(
				EWorldWidgetType::WorldMap);
		}
		if (MapWidget == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.MapPreview: 지도 월드 위젯을 만들지 못했습니다."));
			return;
		}

		MapWidget->SetRoomSelectionEnabled(false);
		MapWidget->ClearMapStatusOverride();
		MapWidget->OpenUI();
		MapWidget->RefreshMap();
		ShownWidget = MapWidget;

		UE_LOG(LogRD, Display,
			TEXT("RD.MapPreview: 저장 변경 없이 현재 런 지도를 열었습니다."));
	}

	FAutoConsoleCommandWithWorld PreviewCommand(
		TEXT("RD.MapPreview"),
		TEXT("Toggle the current run map in read-only preview mode."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Toggle));
}

#endif
