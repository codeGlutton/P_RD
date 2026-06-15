#include "UI/CombatTileMapHUDWidget.h"

#include "UI/Combat/CombatViewModel.h"

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

	// 뷰모델 연결 시 선택은 의도로만 보낸다(실행/검증은 게임플레이). 시각 강조는 로컬 유지.
	if (mCombatViewModel != nullptr)
	{
		mCombatViewModel->RequestSelectSkill(SkillIndex);
	}

	RefreshSkillRailWidgets();
	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
}
