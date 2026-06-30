#include "UI/CombatTileMapHUDWidget.h"

#include "GameMode/CombatGameMode.h"
#include "InputCoreTypes.h"

FReply UCombatTileMapHUDWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
	const int32 SkillIndex = FindSkillRailIndexAtScreenPosition(ScreenPosition);
	if (SkillIndex == INDEX_NONE)
	{
		// 스킬 레일 밖 = 월드(타일/유닛) 영역.
		BeginWorldPress();
		return FReply::Handled().CaptureMouse(TakeWidget());
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

	if (mWorldPressing == true)
	{
		return EndWorldPress(true);
	}

	if (mSkillPressing == false)
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}

	if (mSkillLongPressConsumed == false)
	{
		SelectSkillForAssignment(mPressedSkillIndex);
	}

	mSkillPressing = false;
	mPressedSkillIndex = INDEX_NONE;
	mSkillPressElapsed = 0.0f;
	mSkillLongPressConsumed = false;
	return FReply::Handled().ReleaseMouseCapture();
}

FReply UCombatTileMapHUDWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	const FVector2D ScreenPosition = InGestureEvent.GetScreenSpacePosition();
	const int32 SkillIndex = FindSkillRailIndexAtScreenPosition(ScreenPosition);
	if (SkillIndex == INDEX_NONE)
	{
		// 스킬 레일 밖 = 월드(타일/유닛) 영역.
		BeginWorldPress();
		return FReply::Handled();
	}

	BeginSkillPress(SkillIndex);
	return FReply::Handled();
}

FReply UCombatTileMapHUDWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (mSkillPressing == false)
	{
		if (mWorldPressing == true)
		{
			return EndWorldPress(false);
		}
		return Super::NativeOnTouchEnded(InGeometry, InGestureEvent);
	}

	if (mSkillLongPressConsumed == false)
	{
		SelectSkillForAssignment(mPressedSkillIndex);
	}

	mSkillPressing = false;
	mPressedSkillIndex = INDEX_NONE;
	mSkillPressElapsed = 0.0f;
	mSkillLongPressConsumed = false;
	return FReply::Handled();
}
