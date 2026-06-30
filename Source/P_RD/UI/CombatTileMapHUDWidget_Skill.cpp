#include "UI/CombatTileMapHUDWidget.h"

#include "Components/TextBlock.h"
#include "GameMode/CombatGameMode.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"

using namespace RDCombatHUD;

void UCombatTileMapHUDWidget::HandleEndTurnButtonClicked()
{
	if (ACombatGameMode* CombatGameMode = GetWorld()->GetAuthGameMode<ACombatGameMode>())
	{
		const bool bHandled = CombatGameMode->EndTurn();
		UE_LOG(LogRD, Log, TEXT("Combat HUD end turn clicked. Handled=%s"), bHandled ? TEXT("true") : TEXT("false"));
		return;
	}

	UE_LOG(LogRD, Warning, TEXT("Combat HUD end turn clicked, but CombatGameMode is not available."));
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
