#include "UI/CinematicWidget.h"

UCinematicWidget::UCinematicWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UCinematicWidget::OpenUI(FOnEndUIOpenAnimation Callback)
{
	OnEndUIOpenAnimation = MoveTemp(Callback);
	bOpenUIFinished = false;
	SetVisibility(ESlateVisibility::Visible);
	PlayOpenUIAnimation();
}

void UCinematicWidget::CloseUI(FOnEndUICloseAnimation Callback)
{
	OnEndUICloseAnimation = MoveTemp(Callback);
	bCloseUIFinished = false;
	PlayCloseUIAnimation();
}

void UCinematicWidget::PlayCinematic(FOnEndCinematicAnimation Callback)
{
	OnEndCinematicAnimation = MoveTemp(Callback);
	bCinematicFinished = false;
	PlayCinematicAnimation();
}

void UCinematicWidget::FinishOpenUI()
{
	if (bOpenUIFinished)
	{
		return;
	}

	bOpenUIFinished = true;
	if (OnEndUIOpenAnimation.IsBound())
	{
		OnEndUIOpenAnimation.Execute(this);
		OnEndUIOpenAnimation.Unbind();
	}
}

void UCinematicWidget::FinishCloseUI()
{
	if (bCloseUIFinished)
	{
		return;
	}

	bCloseUIFinished = true;
	SetVisibility(ESlateVisibility::Collapsed);
	if (OnEndUICloseAnimation.IsBound())
	{
		OnEndUICloseAnimation.Execute(this);
		OnEndUICloseAnimation.Unbind();
	}
}

void UCinematicWidget::FinishCinematic()
{
	if (bCinematicFinished)
	{
		return;
	}

	bCinematicFinished = true;
	if (OnEndCinematicAnimation.IsBound())
	{
		OnEndCinematicAnimation.Execute(this);
		OnEndCinematicAnimation.Unbind();
	}
}

void UCinematicWidget::PlayOpenUIAnimation_Implementation()
{
	FinishOpenUI();
}

void UCinematicWidget::PlayCloseUIAnimation_Implementation()
{
	FinishCloseUI();
}

void UCinematicWidget::PlayCinematicAnimation_Implementation()
{
	FinishCinematic();
}
