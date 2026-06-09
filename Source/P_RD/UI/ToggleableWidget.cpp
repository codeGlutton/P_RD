#include "UI/ToggleableWidget.h"

UToggleableWidget::UToggleableWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UToggleableWidget::OpenUI(FOnEndUIOpenAnimation Callback)
{
	if (LifecycleState == EToggleableWidgetLifecycleState::Open && IsInViewport() && IsVisible())
	{
		if (Callback.IsBound())
		{
			Callback.Execute(this);
		}
		return;
	}

	OnEndUIOpenAnimation = MoveTemp(Callback);
	LifecycleState = EToggleableWidgetLifecycleState::Opening;
	ApplyOpenUI();
	PlayOpenUIAnimation();
}

void UToggleableWidget::CloseUI(FOnEndUICloseAnimation Callback)
{
	if (LifecycleState == EToggleableWidgetLifecycleState::Closed)
	{
		if (Callback.IsBound())
		{
			Callback.Execute(this);
		}
		return;
	}

	if (LifecycleState == EToggleableWidgetLifecycleState::Closing)
	{
		return;
	}

	OnEndUICloseAnimation = MoveTemp(Callback);
	LifecycleState = EToggleableWidgetLifecycleState::Closing;
	PlayCloseUIAnimation();
}

bool UToggleableWidget::IsOpened() const
{
	return LifecycleState == EToggleableWidgetLifecycleState::Open && IsInViewport() && IsVisible();
}

void UToggleableWidget::FinishOpenUI()
{
	if (LifecycleState != EToggleableWidgetLifecycleState::Opening)
	{
		return;
	}

	LifecycleState = EToggleableWidgetLifecycleState::Open;
	if (OnEndUIOpenAnimation.IsBound())
	{
		OnEndUIOpenAnimation.Execute(this);
		OnEndUIOpenAnimation.Unbind();
	}
}

void UToggleableWidget::FinishCloseUI()
{
	if (LifecycleState != EToggleableWidgetLifecycleState::Closing)
	{
		return;
	}

	ApplyCloseUI();
	LifecycleState = EToggleableWidgetLifecycleState::Closed;
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
	return bRemoveFromParentOnClose;
}
