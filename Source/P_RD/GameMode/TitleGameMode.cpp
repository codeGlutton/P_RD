#include "GameMode/TitleGameMode.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/TitleMenuWidget.h"

ATitleGameMode::ATitleGameMode()
{
	if (TSubclassOf<UUserWidget> TitleWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_TitleMenu.WBP_TitleMenu_C")))
	{
		mHUDClass = TitleWidgetClass;
	}
	else
	{
		mHUDClass = UTitleMenuWidget::StaticClass();
	}

	mWorldWidgets = { EWorldWidgetType::TopMenuBar, EWorldWidgetType::MsgNotify, EWorldWidgetType::SaveNotify };
}

void ATitleGameMode::InitializeCommonRoom()
{
}

void ATitleGameMode::BeginRoom()
{
	if (UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>())
	{
		WorldWidgetSubsystem->OpenHUD();
	}

	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		PlayerController->SetShowMouseCursor(true);
		FInputModeUIOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}
}
