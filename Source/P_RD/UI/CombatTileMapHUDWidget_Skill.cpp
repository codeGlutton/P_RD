#include "UI/CombatTileMapHUDWidget.h"

#include "Components/TextBlock.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"

using namespace RDCombatHUD;

void UCombatTileMapHUDWidget::HandleEndTurnButtonClicked()
{
	// 뷰모델 연결 시 턴 종료는 의도로 보낸다. 미연결 시 기존처럼 로그만.
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->RequestEndTurn();
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

	// 실제 배치 상태일 때만 보이게 한다(유휴 안내문구는 표시하지 않음).
	mDiceAssignmentText->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (mSelectedDiceIndex != INDEX_NONE && mDiceUIs.IsValidIndex(mSelectedDiceIndex) && mSelectedSkillIndex != INDEX_NONE)
	{
		mDiceAssignmentText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "DiceAssignmentReadyFormat", "DICE {0} PLACED\nTap another die to replace"),
			FText::AsNumber(mDiceUIs[mSelectedDiceIndex].mResultValue)
		));
		return;
	}

	if (mSelectedDiceIndex != INDEX_NONE && mDiceUIs.IsValidIndex(mSelectedDiceIndex))
	{
		mDiceAssignmentText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "DiceAssignmentDiceOnlyFormat", "SELECT SKILL FIRST\nDICE {0} is waiting"),
			FText::AsNumber(mDiceUIs[mSelectedDiceIndex].mResultValue)
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

	// 유휴 상태: 안내 문구 없이 비워 두고 접는다("SELECT SKILL then tap a die" 삭제).
	mDiceAssignmentText->SetText(FText::GetEmpty());
	mDiceAssignmentText->SetVisibility(ESlateVisibility::Collapsed);
}
