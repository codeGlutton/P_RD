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

	// 선택된 주사위는 여러 개일 수 있다 — 개수와 합을 DTO(mIsSelected)에서 집계해 표시한다.
	int32 SelectedCount = 0;
	int32 SelectedSum = 0;
	for (const FDiceViewData& DiceView : mDiceUIs)
	{
		if (DiceView.mIsSelected)
		{
			++SelectedCount;
			SelectedSum += DiceView.mResultValue;
		}
	}

	if (SelectedCount > 0 && mSelectedSkillIndex != INDEX_NONE)
	{
		mDiceAssignmentText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "DiceAssignmentReadyFormat", "DICE x{0} PLACED (SUM {1})\nTap dice to add or remove"),
			FText::AsNumber(SelectedCount), FText::AsNumber(SelectedSum)
		));
		return;
	}

	if (SelectedCount > 0)
	{
		mDiceAssignmentText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "DiceAssignmentDiceOnlyFormat", "SELECT SKILL FIRST\nDICE x{0} (SUM {1}) waiting"),
			FText::AsNumber(SelectedCount), FText::AsNumber(SelectedSum)
		));
		return;
	}

	if (mSelectedSkillIndex != INDEX_NONE)
	{
		mDiceAssignmentText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "DiceAssignmentSkillOnlyFormat", "{0}\nTap a rolled die"),
			GetOwnedSkillLabel(mSelectedSkillIndex)
		));
		return;
	}

	// 유휴 상태: 안내 문구 없이 비워 두고 접는다("SELECT SKILL then tap a die" 삭제).
	mDiceAssignmentText->SetText(FText::GetEmpty());
	mDiceAssignmentText->SetVisibility(ESlateVisibility::Collapsed);
}
