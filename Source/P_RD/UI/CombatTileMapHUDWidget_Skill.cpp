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

void UCombatTileMapHUDWidget::HandleDiceSelectionChanged(const TArray<FPrimaryAssetId>& /*DiceIds*/, const TArray<int32>& Values)
{
	// 어댑터/목이 넣기·취소마다 보내주는 '현재 올린 주사위'의 눈금값. 배치 안내 텍스트를 다시 그린다.
	// (id는 후속 표시용으로 받지만 현재 텍스트는 개수·합계만 쓴다.)
	mSelectedDiceValues = Values;
	RefreshDiceAssignmentText();
}

void UCombatTileMapHUDWidget::RefreshDiceAssignmentText() const
{
	if (mDiceAssignmentText == nullptr)
	{
		return;
	}

	// 실제 배치 상태일 때만 보이게 한다(유휴 안내문구는 표시하지 않음).
	mDiceAssignmentText->SetVisibility(ESlateVisibility::HitTestInvisible);

	// 올린 주사위는 여러 개·순서 무관. 개수와 눈금 합으로 표시한다(단일 선택 가정 제거).
	const int32 SelectedCount = mSelectedDiceValues.Num();
	int32 SelectedSum = 0;
	for (int32 Value : mSelectedDiceValues)
	{
		SelectedSum += Value;
	}

	if (SelectedCount > 0 && mSelectedSkillIndex != INDEX_NONE)
	{
		mDiceAssignmentText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "DiceAssignmentReadyFormat", "DICE x{0} PLACED  (sum {1})\nTap a die to add or remove"),
			FText::AsNumber(SelectedCount),
			FText::AsNumber(SelectedSum)
		));
		return;
	}

	if (SelectedCount > 0)
	{
		mDiceAssignmentText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "DiceAssignmentDiceOnlyFormat", "SELECT SKILL FIRST\nDICE x{0} waiting"),
			FText::AsNumber(SelectedCount)
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
