#include "UI/ToggleableWidget.h"

UToggleableWidget::UToggleableWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UToggleableWidget::OpenUI(FOnEndUIOpenAnimation Callback)
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
	mLifecycleState = EToggleableWidgetLifecycleState::Opening;
	ApplyOpenUI();
	PlayOpenUIAnimation();
}

void UToggleableWidget::CloseUI(FOnEndUICloseAnimation Callback)
{
	if (mLifecycleState == EToggleableWidgetLifecycleState::Closed)
	{
		if (Callback.IsBound())
		{
			Callback.Execute(this);
		}
		return;
	}

	if (mLifecycleState == EToggleableWidgetLifecycleState::Closing)
	{
		return;
	}

	OnEndUICloseAnimation = MoveTemp(Callback);
	mLifecycleState = EToggleableWidgetLifecycleState::Closing;
	PlayCloseUIAnimation();
}

bool UToggleableWidget::IsOpened() const
{
	return (mLifecycleState == EToggleableWidgetLifecycleState::Opening
			|| mLifecycleState == EToggleableWidgetLifecycleState::Open)
		&& IsInViewport()
		&& IsVisible();
}

void UToggleableWidget::FinishOpenUI()
{
	if (mLifecycleState != EToggleableWidgetLifecycleState::Opening)
	{
		return;
	}

	mLifecycleState = EToggleableWidgetLifecycleState::Open;
	if (OnEndUIOpenAnimation.IsBound())
	{
		OnEndUIOpenAnimation.Execute(this);
		OnEndUIOpenAnimation.Unbind();
	}
}

void UToggleableWidget::FinishCloseUI()
{
	if (mLifecycleState != EToggleableWidgetLifecycleState::Closing)
	{
		return;
	}

	ApplyCloseUI();
	mLifecycleState = EToggleableWidgetLifecycleState::Closed;
	if (OnEndUICloseAnimation.IsBound())
	{
		OnEndUICloseAnimation.Execute(this);
		OnEndUICloseAnimation.Unbind();
	}
}

void UToggleableWidget::PlayOpenUIAnimation_Implementation()
{
	FinishOpenUI();
}

void UToggleableWidget::PlayCloseUIAnimation_Implementation()
{
	FinishCloseUI();
}

void UToggleableWidget::ApplyOpenUI()
{
	if (IsInViewport() == false)
	{
		AddToViewport(GetViewportZOrder());
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UToggleableWidget::ApplyCloseUI()
{
	if (ShouldRemoveFromParentOnClose())
	{
		RemoveFromParent();
		return;
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

int32 UToggleableWidget::GetViewportZOrder() const
{
	return mViewportZOrder;
}

bool UToggleableWidget::ShouldRemoveFromParentOnClose() const
{
	return mRemoveFromParentOnClose;
}
