#include "UI/CombatTileMapHUDWidget.h"

#include "Components/TextBlock.h"
#include "UI/Combat/CombatViewModel.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"

using namespace RDCombatHUD;

void UCombatTileMapHUDWidget::HandleEndTurnButtonClicked()
{
	// 뷰모델 연결 시 턴 종료는 의도로 보낸다. 미연결 시 기존처럼 로그만.
	if (mCombatViewModel != nullptr)
	{
		mCombatViewModel->RequestEndTurn();
		return;
	}

	UE_LOG(LogRD, Log, TEXT("END TURN button clicked. Combat turn API is not connected yet."));
}

void UCombatTileMapHUDWidget::RefreshDiceAssignmentText() const
{
	if (mDiceAssignmentText == nullptr)
	{
		return;
	}

	if (mSelectedDiceIndex != INDEX_NONE && mDiceViews.IsValidIndex(mSelectedDiceIndex) && mSelectedSkillIndex != INDEX_NONE)
	{
		mDiceAssignmentText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "DiceAssignmentReadyFormat", "DICE {0} PLACED\nTap another die to replace"),
			FText::AsNumber(mDiceViews[mSelectedDiceIndex].mResultValue)
		));
		return;
	}

	if (mSelectedDiceIndex != INDEX_NONE && mDiceViews.IsValidIndex(mSelectedDiceIndex))
	{
		mDiceAssignmentText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "DiceAssignmentDiceOnlyFormat", "SELECT SKILL FIRST\nDICE {0} is waiting"),
			FText::AsNumber(mDiceViews[mSelectedDiceIndex].mResultValue)
		));
		return;
	}

	if (mSelectedSkillIndex != INDEX_NONE)
	{
		mDiceAssignmentText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "DiceAssignmentSkillOnlyFormat", "{0}\nTap a rolled die"),
			GetCombatSkillLabel(mSelectedSkillIndex)
		));
		return;
	}

	mDiceAssignmentText->SetText(NSLOCTEXT("CombatTileMapHUDWidget", "DiceAssignmentDefaultText", "SELECT SKILL\nthen tap a die"));
}
