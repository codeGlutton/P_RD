#include "UI/CombatTileMapHUDWidget.h"

#include "Components/TextBlock.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"

#define LOCTEXT_NAMESPACE "CombatTileMapHUDWidget_Skill"

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
	if (mCombatControlsHidden)
	{
		mDiceAssignmentText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const TArray<FSkillUI>* Skills = mCombatUIModel != nullptr ? &mCombatUIModel->GetSkillUIs() : nullptr;
	const int32 SelectedSkillIndex = mCombatUIModel != nullptr
		? mCombatUIModel->GetSelectedSkillIndex()
		: mSelectedSkillIndex;
	if (Skills == nullptr || Skills->IsValidIndex(SelectedSkillIndex) == false)
	{
		mDiceAssignmentText->SetText(NSLOCTEXT(
			"CombatTileMapHUDWidget", "DiceAssignmentAutomatic", "드래그 세기에 맞춰 자동 사용"));
		mDiceAssignmentText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.90f, 0.96f, 1.0f)));
		mDiceAssignmentText->SetVisibility(ESlateVisibility::HitTestInvisible);
		return;
	}

	const FSkillUI& Skill = (*Skills)[SelectedSkillIndex];
	const int32 RequiredDiceCount = FMath::Max(Skill.mDiceCost, 0);
	int32 SelectedDiceCount = mCombatUIModel != nullptr
		? mCombatUIModel->GetSelectedDiceIndices().Num()
		: 0;
	if (mCombatUIModel == nullptr)
	{
		for (const FDiceViewData& Dice : mDiceUIs)
		{
			SelectedDiceCount += Dice.mIsSelected ? 1 : 0;
		}
	}
	const bool bGripFamily = Skill.mIsPullSkill || Skill.mIsThrowSkill || Skill.mIsSwapSkill;
	const FString Instruction = bGripFamily
		? TEXT("손아귀 · 결과에 맞춰 주사위 자동")
		: FString::Printf(TEXT("%s · 주사위 자동"), *Skill.mName.ToString());
	mDiceAssignmentText->SetText(FText::FromString(Instruction));
	mDiceAssignmentText->SetColorAndOpacity(FSlateColor(
		SelectedDiceCount >= RequiredDiceCount
			? FLinearColor(0.28f, 1.0f, 0.62f, 1.0f)
			: FLinearColor(1.0f, 0.84f, 0.34f, 1.0f)));
	mDiceAssignmentText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

#undef LOCTEXT_NAMESPACE
