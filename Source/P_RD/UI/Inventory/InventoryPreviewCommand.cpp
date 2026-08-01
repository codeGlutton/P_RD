/**
 * @file InventoryPreviewCommand.cpp
 * @brief 실제 공용 인벤토리 월드 위젯을 저장 변경 없이 채워 보는 개발 명령.
 */

#include "RDMinimal.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "UI/Inventory/InventoryUIModel.h"
#include "UI/Inventory/InventoryUIWidgetBase.h"
#include "UI/Inventory/MockInventoryDriver.h"

#if !UE_BUILD_SHIPPING

namespace InventoryPreview
{
	TWeakObjectPtr<UInventoryUIWidgetBase> ShownWidget;

	void Toggle(UWorld* World)
	{
		if (ShownWidget.IsValid() && ShownWidget->IsOpened())
		{
			// 다음 정상 OpenUI가 실제 룸 스냅샷을 다시 읽도록 외부 mock 연결을 끊는다.
			ShownWidget->BindUIModel(nullptr);
			ShownWidget->CloseUI();
			ShownWidget.Reset();
			UE_LOG(LogTemp, Display, TEXT("[InventoryPreview] closed"));
			return;
		}

		if (World == nullptr || World->IsGameWorld() == false)
		{
			UE_LOG(LogTemp, Warning, TEXT("[InventoryPreview] game world is unavailable"));
			return;
		}

		UWorldWidgetSubsystem* WidgetSubsystem =
			World->GetSubsystem<UWorldWidgetSubsystem>();
		if (WidgetSubsystem == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[InventoryPreview] widget subsystem is null"));
			return;
		}

		UInventoryUIWidgetBase* InventoryWidget =
			WidgetSubsystem->GetWorldWidget<UInventoryUIWidgetBase>(
				EWorldWidgetType::Inventory);
		if (InventoryWidget == nullptr)
		{
			WidgetSubsystem->InitWorldWidget(EWorldWidgetType::Inventory);
			InventoryWidget =
				WidgetSubsystem->GetWorldWidget<UInventoryUIWidgetBase>(
					EWorldWidgetType::Inventory);
		}
		if (InventoryWidget == nullptr)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[InventoryPreview] actual Inventory world widget is unavailable"));
			return;
		}

		// ApplyOpenUI가 실제 룸을 읽은 다음 mock 모델을 바인딩해야 preview가 덮이지 않는다.
		InventoryWidget->OpenUI();

		UInventoryUIModel* PreviewModel =
			NewObject<UInventoryUIModel>(InventoryWidget);
		InventoryWidget->BindUIModel(PreviewModel);

		UMockInventoryDriver* Driver =
			NewObject<UMockInventoryDriver>(InventoryWidget);
		Driver->Start(PreviewModel);

		ShownWidget = InventoryWidget;
		UE_LOG(LogTemp, Display,
			TEXT("[InventoryPreview] opened with %d transient artifacts"),
			InventoryWidget->GetArtifactCount());
	}

	FAutoConsoleCommandWithWorld PreviewCommand(
		TEXT("RD.InventoryPreview"),
		TEXT("Toggle the actual shared Inventory widget with transient filled preview data."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Toggle));
}

#endif
