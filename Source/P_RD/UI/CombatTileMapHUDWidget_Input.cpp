#include "UI/CombatTileMapHUDWidget.h"

#include "InputCoreTypes.h"

FReply UCombatTileMapHUDWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
	if (IsScreenPositionOverLevelValue(ScreenPosition))
	{
		mLevelValueTouched = true;
		SetExpHoldPanelVisible(true);
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	const bool bClosedSkillDetail = HideSkillDetailIfClickedOutside(ScreenPosition);
	const int32 SkillIndex = FindSkillRailIndexAtScreenPosition(ScreenPosition);
	if (SkillIndex == INDEX_NONE)
	{
		if (bClosedSkillDetail == true)
		{
			return FReply::Handled();
		}
		// 월드 입력은 CombatCameraPawn이 담당한다. Unhandled로 내려 포인터 입력이 Pawn까지 도달하게 한다.
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	BeginSkillPress(SkillIndex);
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UCombatTileMapHUDWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}

	if (mLevelValueTouched)
	{
		mLevelValueTouched = false;
		SetExpHoldPanelVisible(false);
		return FReply::Handled().ReleaseMouseCapture();
	}

	if (mSkillPressing == false)
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}

	if (mSkillDetailOpenedFromPress == false)
	{
		SelectSkillForAssignment(mPressedSkillIndex);
	}

	mSkillPressing = false;
	mPressedSkillIndex = INDEX_NONE;
	mSkillPressElapsed = 0.0f;
	mSkillDetailOpenedFromPress = false;
	return FReply::Handled().ReleaseMouseCapture();
}

FReply UCombatTileMapHUDWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	const FVector2D ScreenPosition = InGestureEvent.GetScreenSpacePosition();
	if (IsScreenPositionOverLevelValue(ScreenPosition))
	{
		mLevelValueTouched = true;
		SetExpHoldPanelVisible(true);
		return FReply::Handled();
	}

	const bool bClosedSkillDetail = HideSkillDetailIfClickedOutside(ScreenPosition);
	const int32 SkillIndex = FindSkillRailIndexAtScreenPosition(ScreenPosition);
	if (SkillIndex == INDEX_NONE)
	{
		if (bClosedSkillDetail == true)
		{
			return FReply::Handled();
		}
		// 월드 입력은 CombatCameraPawn이 담당한다. UI 컨트롤이 아닌 터치는 Pawn 입력으로 통과시킨다.
		return Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
	}

	BeginSkillPress(SkillIndex);
	return FReply::Handled();
}

FReply UCombatTileMapHUDWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (mLevelValueTouched)
	{
		mLevelValueTouched = false;
		SetExpHoldPanelVisible(false);
		return FReply::Handled();
	}

	if (mSkillPressing == false)
	{
		return Super::NativeOnTouchEnded(InGeometry, InGestureEvent);
	}

	if (mSkillDetailOpenedFromPress == false)
	{
		SelectSkillForAssignment(mPressedSkillIndex);
	}

	mSkillPressing = false;
	mPressedSkillIndex = INDEX_NONE;
	mSkillPressElapsed = 0.0f;
	mSkillDetailOpenedFromPress = false;
	return FReply::Handled();
}
