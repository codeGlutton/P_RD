#include "UI/TitleMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"

namespace
{
	constexpr int32 TitleMainScreenIndex = 0;

	const FText TitleTextValue = NSLOCTEXT("TitleMenuWidget", "TitleText", "Rogue The Dice");
	const FText StartTextValue = NSLOCTEXT("TitleMenuWidget", "StartText", "START");
	const FText ContinueTextValue = NSLOCTEXT("TitleMenuWidget", "ContinueText", "CONTINUE");
	const FText SettingsTextValue = NSLOCTEXT("TitleMenuWidget", "SettingsText", "SETTING");
	const FText ReadyStatusText = NSLOCTEXT("TitleMenuWidget", "ReadyStatusText", "Ready");
	const FText MainOnlyStatusText = NSLOCTEXT("TitleMenuWidget", "MainOnlyStatusText", "Title main screen only");
}

UTitleMenuWidget::UTitleMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Visible);
}

void UTitleMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (StartButton != nullptr)
	{
		StartButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleStartButtonClicked);
	}

	if (ContinueButton != nullptr)
	{
		ContinueButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleContinueButtonClicked);
	}

	if (SettingsButton != nullptr)
	{
		SettingsButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleSettingsButtonClicked);
	}

	SyncMainText();
	ShowMainScreen();
	SetStatusText(ReadyStatusText);
}

void UTitleMenuWidget::NativeDestruct()
{
	if (StartButton != nullptr)
	{
		StartButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleStartButtonClicked);
	}

	if (ContinueButton != nullptr)
	{
		ContinueButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleContinueButtonClicked);
	}

	if (SettingsButton != nullptr)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleSettingsButtonClicked);
	}

	Super::NativeDestruct();
}

void UTitleMenuWidget::ShowMainScreen() const
{
	if (ScreenSwitcher != nullptr)
	{
		ScreenSwitcher->SetActiveWidgetIndex(TitleMainScreenIndex);
	}
}

void UTitleMenuWidget::SyncMainText() const
{
	if (TitleText != nullptr)
	{
		TitleText->SetText(TitleTextValue);
	}

	if (StartButtonText != nullptr)
	{
		StartButtonText->SetText(StartTextValue);
	}

	if (ContinueButtonText != nullptr)
	{
		ContinueButtonText->SetText(ContinueTextValue);
	}

	if (SettingsButtonText != nullptr)
	{
		SettingsButtonText->SetText(SettingsTextValue);
	}
}

void UTitleMenuWidget::SetStatusText(const FText& InText) const
{
	if (StatusText != nullptr)
	{
		StatusText->SetText(InText);
	}
}

void UTitleMenuWidget::HandleStartButtonClicked()
{
	ShowMainScreen();
	SetStatusText(MainOnlyStatusText);
}

void UTitleMenuWidget::HandleContinueButtonClicked()
{
	ShowMainScreen();
	SetStatusText(MainOnlyStatusText);
}

void UTitleMenuWidget::HandleSettingsButtonClicked()
{
	ShowMainScreen();
	SetStatusText(MainOnlyStatusText);
}
