#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"

#define LOCTEXT_NAMESPACE "CombatTileMapHUDWidget_Skill"

using namespace RDCombatHUD;

void UCombatTileMapHUDWidget::HandleEndTurnButtonClicked()
{
	if (mCombatUIModel != nullptr)
	{
		const bool IsBuildActive = mCombatUIModel->GetTurnUI().mPhase != ECombatBuildPhaseUI::None;
		if (IsBuildActive)
		{
			mCombatUIModel->RequestConfirm();
		}
		else
		{
			mCombatUIModel->RequestEndTurn();
		}
		return;
	}

	UE_LOG(LogRD, Log, TEXT("END TURN button clicked. Combat turn API is not connected yet."));
}

void UCombatTileMapHUDWidget::HandleCancelActionButtonClicked()
{
	if (mCombatUIModel != nullptr
		&& mCombatUIModel->GetTurnUI().mPhase != ECombatBuildPhaseUI::None)
	{
		mCombatUIModel->RequestCancel();
	}
}

void UCombatTileMapHUDWidget::RefreshActionControls() const
{
	const ECombatBuildPhaseUI Phase = mCombatUIModel != nullptr
		? mCombatUIModel->GetTurnUI().mPhase
		: ECombatBuildPhaseUI::None;
	const bool IsBuildActive = Phase != ECombatBuildPhaseUI::None;
	const bool CanConfirm = Phase == ECombatBuildPhaseUI::Preview;
	const FText PrimaryLabel = IsBuildActive
		? NSLOCTEXT("CombatTileMapHUDWidget", "ConfirmAction", "확정")
		: NSLOCTEXT("CombatTileMapHUDWidget", "EndTurnLabel", "END\nTURN");

	if (EndTurnButton != nullptr)
	{
		EndTurnButton->SetIsEnabled(IsBuildActive == false || CanConfirm);
	}
	if (mPrimaryActionButtonText != nullptr)
	{
		mPrimaryActionButtonText->SetText(PrimaryLabel);
	}
	if (WidgetTree != nullptr)
	{
		if (UTextBlock* SkinLabel = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("HUD_M_btn_end_turn_label"))))
		{
			SkinLabel->SetText(PrimaryLabel);
			SkinLabel->SetJustification(ETextJustify::Center);
			SkinLabel->SetColorAndOpacity(FSlateColor(CanConfirm || IsBuildActive == false
				? FLinearColor::White
				: FLinearColor(0.52f, 0.52f, 0.55f, 1.0f)));
		}
	}

	if (mCancelActionButton != nullptr)
	{
		mCancelActionButton->SetVisibility(IsBuildActive && mCombatControlsHidden == false
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
		mCancelActionButton->SetIsEnabled(IsBuildActive);
	}
}

void UCombatTileMapHUDWidget::RefreshDiceAssignmentText() const
{
	if (mDiceAssignmentText == nullptr)
	{
		return;
	}

	const bool IsMovementBuild = mCombatUIModel != nullptr
		&& mCombatUIModel->GetSelectedSkillIndex() == CombatMovementSkillDataIndex
		&& mCombatUIModel->GetTurnUI().mPhase != ECombatBuildPhaseUI::None;
	if (IsMovementBuild == false || mCombatControlsHidden)
	{
		mDiceAssignmentText->SetText(FText::GetEmpty());
		mDiceAssignmentText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const int32 SelectedDiceCount = mCombatUIModel->GetSelectedDiceIndices().Num();
	if (SelectedDiceCount < 1)
	{
		mDiceAssignmentText->SetText(
			NSLOCTEXT("CombatTileMapHUDWidget", "MovementSelectDiceHint", "주사위를 1개 이상 선택"));
	}
	else
	{
		mDiceAssignmentText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "MovementRangeHint", "이동 가능 거리 {0}칸"),
			FText::AsNumber(mCombatUIModel->GetTurnUI().mMoveRange)));
	}
	mDiceAssignmentText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

#undef LOCTEXT_NAMESPACE
