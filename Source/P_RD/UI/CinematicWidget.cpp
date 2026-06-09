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
	LifecycleState = ECinematicWidgetLifecycleState::Opening;
	if (IsInViewport() == false)
	{
		AddToViewport();
	}

	SetVisibility(ESlateVisibility::Visible);
	PlayOpenUIAnimation();
}

void UCinematicWidget::CloseUI(FOnEndUICloseAnimation Callback)
{
	if (LifecycleState == ECinematicWidgetLifecycleState::Closed || LifecycleState == ECinematicWidgetLifecycleState::Closing)
	{
		if (Callback.IsBound())
		{
			Callback.Execute(this);
		}
		return;
	}

	OnEndUICloseAnimation = MoveTemp(Callback);
	bCloseUIFinished = false;
	LifecycleState = ECinematicWidgetLifecycleState::Closing;
	PlayCloseUIAnimation();
}

bool UCinematicWidget::IsOpened() const
{
	return LifecycleState == ECinematicWidgetLifecycleState::Opening
		|| LifecycleState == ECinematicWidgetLifecycleState::Open
		|| LifecycleState == ECinematicWidgetLifecycleState::Playing;
}

void UCinematicWidget::PlayCinematic(FOnEndCinematicAnimation Callback)
{
	if (LifecycleState == ECinematicWidgetLifecycleState::Closed || LifecycleState == ECinematicWidgetLifecycleState::Closing)
	{
		return;
	}

	OnEndCinematicAnimation = MoveTemp(Callback);
	bCinematicFinished = false;
	LifecycleState = ECinematicWidgetLifecycleState::Playing;
	PlayCinematicAnimation();
}

void UCinematicWidget::FinishOpenUI()
{
	if (bOpenUIFinished || LifecycleState != ECinematicWidgetLifecycleState::Opening)
	{
		return;
	}

	bOpenUIFinished = true;
	LifecycleState = ECinematicWidgetLifecycleState::Open;
	if (OnEndUIOpenAnimation.IsBound())
	{
		OnEndUIOpenAnimation.Execute(this);
		OnEndUIOpenAnimation.Unbind();
	}
}

void UCinematicWidget::FinishCloseUI()
{
	if (bCloseUIFinished || LifecycleState != ECinematicWidgetLifecycleState::Closing)
	{
		return;
	}

	bCloseUIFinished = true;
	SetVisibility(ESlateVisibility::Collapsed);
	RemoveFromParent();
	LifecycleState = ECinematicWidgetLifecycleState::Closed;
	if (OnEndUICloseAnimation.IsBound())
	{
		OnEndUICloseAnimation.Execute(this);
		OnEndUICloseAnimation.Unbind();
	}
}

void UCinematicWidget::FinishCinematic()
{
	if (bCinematicFinished || LifecycleState != ECinematicWidgetLifecycleState::Playing)
	{
		return;
	}

	bCinematicFinished = true;
	LifecycleState = ECinematicWidgetLifecycleState::Open;
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
