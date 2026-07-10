#include "UI/SkillPanelWidget.h"

#include "Components/Button.h"
#include "InputCoreTypes.h"

namespace
{
	constexpr float SkillDetailSwipeThreshold = 88.0f;
	constexpr float SkillDetailSwipeVerticalTolerance = 150.0f;
}

FReply USkillPanelWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && BeginSkillDetailSwipe(InMouseEvent.GetScreenSpacePosition()))
	{
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply USkillPanelWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && mTrackingSkillDetailSwipe == true)
	{
		FinishSkillDetailSwipe(InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply USkillPanelWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (BeginSkillDetailSwipe(InGestureEvent.GetScreenSpacePosition()))
	{
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
}

FReply USkillPanelWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (mTrackingSkillDetailSwipe == true)
	{
		FinishSkillDetailSwipe(InGestureEvent.GetScreenSpacePosition());
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnTouchEnded(InGeometry, InGestureEvent);
}

bool USkillPanelWidget::BeginSkillDetailSwipe(const FVector2D& ScreenPosition)
{
	if (mShowingSkillDetail == false || IsPointerOverSkillDetailControl(ScreenPosition) == true)
	{
		return false;
	}

	mTrackingSkillDetailSwipe = true;
	mSkillDetailSwipeStartPosition = ScreenPosition;
	return true;
}

bool USkillPanelWidget::FinishSkillDetailSwipe(const FVector2D& ScreenPosition)
{
	const FVector2D Delta = ScreenPosition - mSkillDetailSwipeStartPosition;
	const bool bIsHorizontalSwipe =
		FMath::Abs(Delta.X) >= SkillDetailSwipeThreshold &&
		FMath::Abs(Delta.Y) <= SkillDetailSwipeVerticalTolerance &&
		FMath::Abs(Delta.X) > FMath::Abs(Delta.Y);

	if (bIsHorizontalSwipe)
	{
		MoveSkillDetail(Delta.X < 0.0f ? 1 : -1);
	}

	mTrackingSkillDetailSwipe = false;
	mSkillDetailSwipeStartPosition = FVector2D::ZeroVector;
	return bIsHorizontalSwipe;
}

bool USkillPanelWidget::IsPointerOverSkillDetailControl(const FVector2D& ScreenPosition) const
{
	const UButton* DetailButtons[] =
	{
		mSkillDetailPreviousButton.Get(),
		mSkillDetailNextButton.Get(),
		mSkillDetailBackButton.Get(),
	};

	for (const UButton* DetailButton : DetailButtons)
	{
		if (DetailButton != nullptr && DetailButton->GetVisibility() != ESlateVisibility::Collapsed && DetailButton->GetCachedGeometry().IsUnderLocation(ScreenPosition))
		{
			return true;
		}
	}

	return false;
}
