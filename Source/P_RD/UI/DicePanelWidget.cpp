#include "UI/DicePanelWidget.h"

#include "Components/Widget.h"
#include "InputCoreTypes.h"

UDicePanelWidget::UDicePanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FReply UDicePanelWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	if (!StartDiceDrag(InMouseEvent.GetScreenSpacePosition()))
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UDicePanelWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}

	if (!mDraggingDice)
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}

	FinishDiceDrag();
	return FReply::Handled().ReleaseMouseCapture();
}

FReply UDicePanelWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!mDraggingDice)
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	UpdateDiceDrag(InMouseEvent.GetScreenSpacePosition());
	return FReply::Handled();
}

void UDicePanelWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	FinishDiceDrag();
	Super::NativeOnMouseLeave(InMouseEvent);
}

FReply UDicePanelWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (!StartDiceDrag(InGestureEvent.GetScreenSpacePosition()))
	{
		return Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
	}

	return FReply::Handled();
}

FReply UDicePanelWidget::NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (!mDraggingDice)
	{
		return Super::NativeOnTouchMoved(InGeometry, InGestureEvent);
	}

	UpdateDiceDrag(InGestureEvent.GetScreenSpacePosition());
	return FReply::Handled();
}

FReply UDicePanelWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (!mDraggingDice)
	{
		return Super::NativeOnTouchEnded(InGeometry, InGestureEvent);
	}

	FinishDiceDrag();
	return FReply::Handled();
}

float UDicePanelWidget::GetCarouselItemAngle(int32 ItemIndex) const
{
	return ItemIndex == GetSelectedCarouselIndex() ? mDiceAngle : 0.0f;
}

void UDicePanelWidget::HandleCarouselSelectionChanged(int32 PreviousIndex, int32 NewIndex)
{
	mDiceAngle = 0.0f;
}

bool UDicePanelWidget::StartDiceDrag(const FVector2D& ScreenPosition)
{
	if (!IsCarouselActivated() || !IsPointerOverSelectedCarouselItem(ScreenPosition))
	{
		return false;
	}

	mDraggingDice = true;
	mLastPointerPosition = ScreenPosition;
	return true;
}

void UDicePanelWidget::UpdateDiceDrag(const FVector2D& ScreenPosition)
{
	if (!mDraggingDice)
	{
		return;
	}

	const FVector2D Delta = ScreenPosition - mLastPointerPosition;
	if (!Delta.IsNearlyZero())
	{
		mDiceAngle = FMath::Fmod(mDiceAngle + Delta.X * 1.25f + Delta.Y * 0.85f, 360.0f);
		ApplyDiceRotation();
	}

	mLastPointerPosition = ScreenPosition;
}

void UDicePanelWidget::FinishDiceDrag()
{
	mDraggingDice = false;
}

void UDicePanelWidget::ApplyDiceRotation() const
{
	if (UWidget* SelectedItem = GetSelectedCarouselItem())
	{
		SelectedItem->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		SelectedItem->SetRenderTransformAngle(mDiceAngle);
	}
}
