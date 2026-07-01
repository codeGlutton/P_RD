#include "UI/CombatTileMapHUDWidget.h"

#include "UI/Combat/CombatUIModel.h"

/** @brief 스킬 버튼 해제 시 짧은 탭은 선택, 롱프레스는 상세 표시로 소모한다. */
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

/** @brief 상세 카드가 열린 상태에서 새 스킬을 누르면 먼저 닫고 새 누름 상태로 갈아탄다. */
void UCombatTileMapHUDWidget::HandleSkillButtonPressed(int32 SkillIndex)
{
	if (IsSkillDetailVisible() == true)
	{
		HideSkillDetail();
	}

	BeginSkillPress(SkillIndex);
}

/** @brief 스킬 레일 입력의 시작점을 저장한다. INDEX_NONE은 히트 테스트 실패를 뜻하는 sentinel이다. */
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

/** @brief NativeTick에서 누름 시간을 누적해 탭/롱프레스 분기를 프레임 독립적으로 처리한다. */
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

/** @brief 스킬 선택은 UI 강조와 의도 전달까지만 담당하고, 스킬 유효성/실행은 전투 계층에 맡긴다. */
void UCombatTileMapHUDWidget::SelectSkillForAssignment(int32 SkillIndex)
{
	if (SkillIndex == INDEX_NONE)
	{
		return;
	}

	if (mSelectedSkillIndex != SkillIndex)
	{
		ClearOwnedDiceSelectionHighlight();
	}
	mSelectedSkillIndex = SkillIndex;

	// 뷰모델 연결 시 선택은 의도로만 보낸다(실행/검증은 게임플레이). 시각 강조는 로컬 유지.
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->RequestSelectSkill(SkillIndex);
	}

	RefreshSkillRailWidgets();
	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
}
