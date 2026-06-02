#include "GameMode/FrontendGameMode.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/TitleMenuWidget.h"

AFrontendGameMode::AFrontendGameMode()
{
	if (TSubclassOf<UUserWidget> TitleWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_TitleMenu.WBP_TitleMenu_C")))
	{
		mHUDClass = TitleWidgetClass;
	}
	else
	{
		mHUDClass = UTitleMenuWidget::StaticClass();
	}

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
		PlayerController->SetShowMouseCursor(true);

		FInputModeUIOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}
}
