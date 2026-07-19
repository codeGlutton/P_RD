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
		mDiceAssignmentText->SetText(NSLOCTEXT("CombatTileMapHUDWidget", "DiceAssignmentChooseSkill", "① 왼쪽에서 스킬을 먼저 선택하세요"));
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
	FString Instruction;
	if (RequiredDiceCount <= 0)
	{
		Instruction = FString::Printf(TEXT("%s · 주사위 필요 없음 → 대상을 선택하세요"), *Skill.mName.ToString());
	}
	else if (SelectedDiceCount < RequiredDiceCount)
	{
		Instruction = FString::Printf(
			TEXT("② %s · 주사위 %d/%d 선택%s"),
			*Skill.mName.ToString(),
			SelectedDiceCount,
			RequiredDiceCount,
			Skill.mIsDisplacementSkill ? TEXT(" · 눈 합 = 밀기 거리") : TEXT(""));
	}
	else
	{
		Instruction = FString::Printf(TEXT("✓ %s · 주사위 %d/%d 완료 → 전장의 대상을 선택하세요"),
			*Skill.mName.ToString(), SelectedDiceCount, RequiredDiceCount);
	}
	mDiceAssignmentText->SetText(FText::FromString(Instruction));
	mDiceAssignmentText->SetColorAndOpacity(FSlateColor(
		SelectedDiceCount >= RequiredDiceCount
			? FLinearColor(0.28f, 1.0f, 0.62f, 1.0f)
			: FLinearColor(1.0f, 0.84f, 0.34f, 1.0f)));
	mDiceAssignmentText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

#undef LOCTEXT_NAMESPACE
