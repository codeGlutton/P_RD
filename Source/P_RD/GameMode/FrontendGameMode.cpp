#include "GameMode/FrontendGameMode.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"

AFrontendGameMode::AFrontendGameMode()
{
	mWorldWidgets.Empty();
}

void AFrontendGameMode::InitializeCommonRoom()
{
}

void AFrontendGameMode::BeginRoom()
{
	if (UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>())
	{
		if (UUserWidget* TitleHUD = WorldWidgetSubsystem->GetHUD())
		{
			TitleHUD->AddToViewport();
			TitleHUD->SetVisibility(ESlateVisibility::Visible);
		}
	}

	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		PlayerController->ActivateTouchInterface(nullptr);
		PlayerController->SetShowMouseCursor(true);

		FInputModeUIOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}
}
