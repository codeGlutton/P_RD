#include "UI/CombatTileMapHUDWidget.h"

void UCombatTileMapHUDWidget::HandleSkillButtonReleased()
{
	if (mSkillPressing == false)
	{
		return;
	}

	if (mSkillDetailOpenedFromPress == false)
	{
		SelectSkillForAssignment(mPressedSkillIndex);
	}

	mSkillPressing = false;
	mPressedSkillIndex = INDEX_NONE;
	mSkillPressElapsed = 0.0f;
	mSkillDetailOpenedFromPress = false;
}

void UCombatTileMapHUDWidget::HandleSkillButtonPressed(int32 SkillIndex)
{
	if (IsSkillDetailVisible() == true)
	{
		HideSkillDetail();
	}

	BeginSkillPress(SkillIndex);
}

void UCombatTileMapHUDWidget::BeginSkillPress(int32 SkillIndex)
{
	if (SkillIndex == INDEX_NONE)
	{
		return;
	}

	mPressedSkillIndex = SkillIndex;
	mSkillPressing = true;
	mSkillDetailOpenedFromPress = false;
	mSkillPressElapsed = 0.0f;
}

void UCombatTileMapHUDWidget::UpdateSkillPress(float InDeltaTime)
{
	if (mSkillPressing == false || mSkillDetailOpenedFromPress == true)
	{
		return;
	}

	mSkillPressElapsed += InDeltaTime;
	if (mSkillPressElapsed >= mSkillLongPressThreshold)
	{
		ShowSkillDetail(mPressedSkillIndex);
		mSkillDetailOpenedFromPress = true;
	}
}

void UCombatTileMapHUDWidget::SelectSkillForAssignment(int32 SkillIndex)
{
	if (SkillIndex == INDEX_NONE)
	{
		return;
	}

	if (mSelectedSkillIndex != SkillIndex)
	{
		mSelectedDiceIndex = INDEX_NONE;
	}
	mSelectedSkillIndex = SkillIndex;

	RefreshSkillRailWidgets();
	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
}
