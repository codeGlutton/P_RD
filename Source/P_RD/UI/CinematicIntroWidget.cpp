#include "UI/CinematicIntroWidget.h"

UCinematicIntroWidget::UCinematicIntroWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UCinematicIntroWidget::OpenIntro()
{
	bIntroFinished = false;
	SetVisibility(ESlateVisibility::Visible);
	OnIntroOpened();
	PlayIntro();
}

void UCinematicIntroWidget::PlayIntro()
{
	if (bIntroFinished)
	{
		return;
	}

	PlayIntroAnimation();
}

void UCinematicIntroWidget::SkipIntro()
{
	FinishIntro();
}

void UCinematicIntroWidget::CloseIntro()
{
	SetVisibility(ESlateVisibility::Collapsed);
	OnIntroClosed();
}

void UCinematicIntroWidget::FinishIntro()
{
	if (bIntroFinished)
	{
		return;
	}

	bIntroFinished = true;
	OnIntroFinished.Broadcast();
}

void UCinematicIntroWidget::PlayIntroAnimation_Implementation()
{
	FinishIntro();
}

void UCinematicIntroWidget::HandleWorldWidgetOpened_Implementation(EWorldWidgetType WorldWidgetType)
{
	if (WorldWidgetType != EWorldWidgetType::IntroCinematic)
	{
		return;
	}

	OpenIntro();
}

bool UCinematicIntroWidget::HandleWorldWidgetCloseRequested_Implementation(EWorldWidgetType WorldWidgetType)
{
	(void)WorldWidgetType;
	return false;
}

void UCinematicIntroWidget::HandleWorldWidgetClosed_Implementation(EWorldWidgetType WorldWidgetType)
{
	if (WorldWidgetType != EWorldWidgetType::IntroCinematic)
	{
		return;
	}

	CloseIntro();
}
