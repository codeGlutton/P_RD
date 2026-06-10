#include "UI/RDUserWidget.h"

URDUserWidget::URDUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void URDUserWidget::OpenUI(FOnEndUIOpenAnimation Callback)
{
	if (IsOpened())
	{
		if (Callback.IsBound())
		{
			Callback.Execute(this);
		}
		return;
	}

	OnEndUIOpenAnimation = MoveTemp(Callback);
	mLifecycleState = ERDUserWidgetLifecycleState::Opening;
	ApplyOpenUI();
	PlayOpenUIAnimation();
}

void URDUserWidget::CloseUI(FOnEndUICloseAnimation Callback)
{
	if (mLifecycleState == ERDUserWidgetLifecycleState::Closed)
	{
		if (Callback.IsBound())
		{
			Callback.Execute(this);
		}
		return;
	}

	if (mLifecycleState == ERDUserWidgetLifecycleState::Closing)
	{
		return;
	}

	OnEndUICloseAnimation = MoveTemp(Callback);
	mLifecycleState = ERDUserWidgetLifecycleState::Closing;
	PlayCloseUIAnimation();
}

bool URDUserWidget::IsOpened() const
{
	return (mLifecycleState == ERDUserWidgetLifecycleState::Opening
			|| mLifecycleState == ERDUserWidgetLifecycleState::Open)
		&& IsInViewport()
		&& IsVisible();
}

void URDUserWidget::FinishOpenUI()
{
	if (mLifecycleState != ERDUserWidgetLifecycleState::Opening)
	{
		return;
	}

	mLifecycleState = ERDUserWidgetLifecycleState::Open;
	if (OnEndUIOpenAnimation.IsBound())
	{
		OnEndUIOpenAnimation.Execute(this);
		OnEndUIOpenAnimation.Unbind();
	}
}

void URDUserWidget::FinishCloseUI()
{
	if (mLifecycleState != ERDUserWidgetLifecycleState::Closing)
	{
		return;
	}

	ApplyCloseUI();
	mLifecycleState = ERDUserWidgetLifecycleState::Closed;
	if (OnEndUICloseAnimation.IsBound())
	{
		OnEndUICloseAnimation.Execute(this);
		OnEndUICloseAnimation.Unbind();
	}
}

void URDUserWidget::PlayOpenUIAnimation_Implementation()
{
	FinishOpenUI();
}

void URDUserWidget::PlayCloseUIAnimation_Implementation()
{
	FinishCloseUI();
}

void URDUserWidget::ApplyOpenUI()
{
	if (IsInViewport() == false)
	{
		AddToViewport(GetViewportZOrder());
	}

	SetVisibility(ESlateVisibility::Visible);
}

void URDUserWidget::ApplyCloseUI()
{
	if (ShouldRemoveFromParentOnClose())
	{
		RemoveFromParent();
		return;
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

int32 URDUserWidget::GetViewportZOrder() const
{
	return mViewportZOrder;
}

bool URDUserWidget::ShouldRemoveFromParentOnClose() const
{
	return mRemoveFromParentOnClose;
}
