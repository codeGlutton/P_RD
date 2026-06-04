#include "UI/TitleMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "UI/CharacterSelectWidget.h"

namespace
{
	constexpr int32 TitleMainScreenIndex = 0;
}

UTitleMenuWidget::UTitleMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, mTitleText(NSLOCTEXT("TitleMenuWidget", "TitleText", "Rogue The Dice"))
	, mStartButtonText(NSLOCTEXT("TitleMenuWidget", "StartText", "START"))
	, mContinueButtonText(NSLOCTEXT("TitleMenuWidget", "ContinueText", "CONTINUE"))
	, mSettingsButtonText(NSLOCTEXT("TitleMenuWidget", "SettingsText", "SETTING"))
	, mSettingsStatusText(NSLOCTEXT("TitleMenuWidget", "SettingsStatusText", "Settings"))
	, mMainOnlyStatusText(NSLOCTEXT("TitleMenuWidget", "MainOnlyStatusText", "Title main screen only"))
	, mCharacterSelectUnavailableText(NSLOCTEXT("TitleMenuWidget", "CharacterSelectUnavailableText", "Character select widget is not ready"))
	, mBackButtonText(NSLOCTEXT("TitleMenuWidget", "BackText", "BACK"))
{
	SetVisibility(ESlateVisibility::Visible);
}

void UTitleMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ValidateDesignerBindings();

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

	if (SettingsBackButton != nullptr)
	{
		SettingsBackButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleSettingsBackButtonClicked);
	}

	if (CharacterSelectWidget != nullptr)
	{
		CharacterSelectWidget->OnBackToMainRequested.AddUniqueDynamic(this, &UTitleMenuWidget::HandleCharacterBackToMainRequested);
	}

	SyncMainText();
	ShowMainScreen();
	SetStatusText(FText::GetEmpty());
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

	if (SettingsBackButton != nullptr)
	{
		SettingsBackButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleSettingsBackButtonClicked);
	}

	if (CharacterSelectWidget != nullptr)
	{
		CharacterSelectWidget->OnBackToMainRequested.RemoveDynamic(this, &UTitleMenuWidget::HandleCharacterBackToMainRequested);
	}

	Super::NativeDestruct();
}

void UTitleMenuWidget::ShowScreen(UWidget* Screen) const
{
	if (ScreenSwitcher == nullptr || Screen == nullptr)
	{
		if (ScreenSwitcher != nullptr && Screen == nullptr)
		{
			ScreenSwitcher->SetActiveWidgetIndex(TitleMainScreenIndex);
		}
		return;
	}

	ScreenSwitcher->SetActiveWidget(Screen);
}

void UTitleMenuWidget::ShowMainScreen() const
{
	ShowScreen(StartScreen);
}

void UTitleMenuWidget::ShowCharacterScreen()
{
	if (CharacterSelectWidget == nullptr)
	{
		SetStatusText(mCharacterSelectUnavailableText);
		ShowScreen(CharacterScreen);
		return;
	}

	CharacterSelectWidget->OpenCharacterSelect();
	ShowScreen(CharacterScreen);
}

void UTitleMenuWidget::ShowSettingsScreen()
{
	if (SettingsScreen == nullptr)
	{
		ShowMainScreen();
		SetStatusText(mMainOnlyStatusText);
		return;
	}

	ShowScreen(SettingsScreen);
	SetStatusText(mSettingsStatusText);
}

void UTitleMenuWidget::SyncMainText() const
{
	if (TitleText != nullptr)
	{
		TitleText->SetText(mTitleText);
	}

	if (StartButtonText != nullptr)
	{
		StartButtonText->SetText(mStartButtonText);
	}

	if (ContinueButtonText != nullptr)
	{
		ContinueButtonText->SetText(mContinueButtonText);
	}

	if (SettingsButtonText != nullptr)
	{
		SettingsButtonText->SetText(mSettingsButtonText);
	}

	if (SettingsBackButtonText != nullptr)
	{
		SettingsBackButtonText->SetText(mBackButtonText);
	}
}

void UTitleMenuWidget::SetStatusText(const FText& /*InText*/) const
{
	if (StatusText != nullptr)
	{
		StatusText->SetText(FText::GetEmpty());
		StatusText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTitleMenuWidget::ValidateDesignerBindings() const
{
	if (ScreenSwitcher == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: ScreenSwitcher is not connected."));
	}

	if (StartScreen == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: StartScreen is not connected."));
	}

	if (CharacterScreen == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: CharacterScreen is not connected."));
	}

	if (CharacterSelectWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: CharacterSelectWidget is not connected. Place WBP_CharacterSelect inside WBP_TitleMenu."));
	}

	if (StartButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: StartButton is not connected."));
	}

	if (ContinueButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: ContinueButton is not connected."));
	}

	if (SettingsButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: SettingsButton is not connected."));
	}

	if (TitleText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: TitleText is not connected."));
	}

	if (StartButtonText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: StartButtonText is not connected."));
	}

	if (ContinueButtonText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: ContinueButtonText is not connected."));
	}

	if (SettingsButtonText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: SettingsButtonText is not connected."));
	}

	SetStatusText(FText::GetEmpty());
}

void UTitleMenuWidget::HandleStartButtonClicked()
{
	ShowCharacterScreen();
}

void UTitleMenuWidget::HandleContinueButtonClicked()
{
	ShowMainScreen();
	SetStatusText(mMainOnlyStatusText);
}

void UTitleMenuWidget::HandleSettingsButtonClicked()
{
	ShowSettingsScreen();
}

void UTitleMenuWidget::HandleSettingsBackButtonClicked()
{
	ShowMainScreen();
	SetStatusText(FText::GetEmpty());
}

void UTitleMenuWidget::HandleCharacterBackToMainRequested()
{
	ShowMainScreen();
	SetStatusText(FText::GetEmpty());
}
