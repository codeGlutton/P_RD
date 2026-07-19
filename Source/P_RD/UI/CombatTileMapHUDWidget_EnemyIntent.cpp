#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "PCGStage/Stage.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/IndexedButtonWidget.h"

namespace
{
	const FLinearColor IntentCurrentColor(0.34f, 1.0f, 0.76f, 1.0f);

	FText GetIntentStateLabel(EEnemyIntentResultUI Result)
	{
		switch (Result)
		{
		case EEnemyIntentResultUI::Executing:    return NSLOCTEXT("CombatTileMapHUDWidget", "IntentStateExecuting", "실행 중");
		case EEnemyIntentResultUI::Completed:    return NSLOCTEXT("CombatTileMapHUDWidget", "IntentStateCompleted", "완료");
		case EEnemyIntentResultUI::Missed:       return NSLOCTEXT("CombatTileMapHUDWidget", "IntentStateMissed", "빗나감");
		case EEnemyIntentResultUI::Collision:    return NSLOCTEXT("CombatTileMapHUDWidget", "IntentStateCollision", "충돌");
		case EEnemyIntentResultUI::FriendlyFire: return NSLOCTEXT("CombatTileMapHUDWidget", "IntentStateFriendlyFire", "적 오사");
		case EEnemyIntentResultUI::HitPlayer:    return NSLOCTEXT("CombatTileMapHUDWidget", "IntentStateHitPlayer", "플레이어 명중");
		case EEnemyIntentResultUI::HitObstacle:  return NSLOCTEXT("CombatTileMapHUDWidget", "IntentStateHitObstacle", "장애물 충돌");
		case EEnemyIntentResultUI::Cancelled:    return NSLOCTEXT("CombatTileMapHUDWidget", "IntentStateCancelled", "취소");
		case EEnemyIntentResultUI::Planned:
		default:                                 return NSLOCTEXT("CombatTileMapHUDWidget", "IntentStatePlanned", "예고");
		}
	}

	FLinearColor GetIntentStateColor(EEnemyIntentResultUI Result, bool bWasDisplaced)
	{
		if (bWasDisplaced && Result == EEnemyIntentResultUI::Planned)
		{
			return FLinearColor(0.48f, 1.0f, 0.78f, 1.0f);
		}
		switch (Result)
		{
		case EEnemyIntentResultUI::Executing:
			return FLinearColor(1.0f, 0.86f, 0.36f, 1.0f);
		case EEnemyIntentResultUI::Missed:
		case EEnemyIntentResultUI::Collision:
		case EEnemyIntentResultUI::FriendlyFire:
		case EEnemyIntentResultUI::HitObstacle:
		case EEnemyIntentResultUI::Cancelled:
			return FLinearColor(0.44f, 1.0f, 0.70f, 1.0f);
		case EEnemyIntentResultUI::HitPlayer:
			return FLinearColor(1.0f, 0.48f, 0.42f, 1.0f);
		case EEnemyIntentResultUI::Completed:
			return FLinearColor(0.72f, 0.78f, 0.78f, 1.0f);
		case EEnemyIntentResultUI::Planned:
		default:
			return FLinearColor(0.90f, 0.97f, 0.94f, 1.0f);
		}
	}

	bool HasResolvedIntentOutcome(const FEnemyIntentUI& Intent)
	{
		return Intent.mResult != EEnemyIntentResultUI::Planned
			&& Intent.mResult != EEnemyIntentResultUI::Executing;
	}

	const FEnemyIntentUI* FindRecommendedIntent(const TArray<FEnemyIntentUI>& Intents)
	{
		return Intents.FindByPredicate([](const FEnemyIntentUI& Intent)
			{
				return Intent.mIsRecommendedInterventionTarget;
			});
	}

	const FEnemyIntentUI* FindIntentByEnemyId(const TArray<FEnemyIntentUI>& Intents, int32 EnemyUnitId)
	{
		return Intents.FindByPredicate([EnemyUnitId](const FEnemyIntentUI& Intent)
			{
				return Intent.mEnemyUnitId == EnemyUnitId;
			});
	}

	FString GetLatestIntentMessage(const FEnemyIntentUI& Intent)
	{
		if (Intent.mResultText.IsEmpty())
		{
			return GetIntentStateLabel(Intent.mResult).ToString();
		}

		TArray<FString> Lines;
		Intent.mResultText.ToString().ParseIntoArrayLines(Lines, true);
		return Lines.IsEmpty() ? GetIntentStateLabel(Intent.mResult).ToString() : Lines.Last();
	}
}

FLinearColor UCombatTileMapHUDWidget::GetEnemyIntentExecutionColor(int32 ExecutionOrder)
{
	// 전장 intent 화살표 overlay와 같은 팔레트. 5기를 넘으면 안전하게 순환한다.
	static const FLinearColor Palette[] =
	{
		FLinearColor(0.10f, 0.88f, 1.00f, 1.0f),
		FLinearColor(1.00f, 0.53f, 0.10f, 1.0f),
		FLinearColor(0.78f, 0.35f, 1.00f, 1.0f),
		FLinearColor(0.35f, 1.00f, 0.42f, 1.0f),
		FLinearColor(1.00f, 0.32f, 0.62f, 1.0f),
	};
	const int32 SafeOrder = FMath::Max(ExecutionOrder, 1);
	return Palette[(SafeOrder - 1) % UE_ARRAY_COUNT(Palette)];
}

FText UCombatTileMapHUDWidget::GetEnemyIntentWorldLabel(const FEnemyIntentUI& Intent)
{
	const int32 DisplayOrder = FMath::Max(Intent.mExecutionOrder, 1);
	const TCHAR* RecommendedPrefix = Intent.mIsRecommendedInterventionTarget ? TEXT("★ ") : TEXT("");
	const bool bHasMovement = Intent.mPlannedOrigin != FTileIndex::Invalid
		&& Intent.mPlannedDestination != FTileIndex::Invalid
		&& Intent.mPlannedDestination != Intent.mPlannedOrigin;
	const bool bHasAttack = Intent.mTargetTile != FTileIndex::Invalid
		|| Intent.mEffectTileIndexes.IsEmpty() == false;
	const int32 MoveTileCount = FMath::Max(Intent.mPathTileIndexes.Num() - 1, 0);
	const FString DisplacedAction = bHasMovement && bHasAttack
		? FString::Printf(TEXT("이동 %d → 공격"), MoveTileCount)
		: (bHasMovement
			? FString::Printf(TEXT("이동 %d"), MoveTileCount)
			: (bHasAttack ? TEXT("공격") : TEXT("대기")));
	if (HasResolvedIntentOutcome(Intent))
	{
		return FText::FromString(FString::Printf(
			TEXT("%s[%d] %s"),
			RecommendedPrefix,
			DisplayOrder,
			*GetIntentStateLabel(Intent.mResult).ToString()));
	}
	if (Intent.mPlanRevision > 0)
	{
		return FText::FromString(FString::Printf(
			TEXT("%s[%d] 대응 #%d · 이동력 -%d → %s"),
			RecommendedPrefix,
			DisplayOrder,
			Intent.mPlanRevision,
			Intent.mResponseCostSpent,
			*DisplacedAction));
	}

	const TCHAR* Flow = bHasMovement && bHasAttack
		? TEXT("이동 → 공격")
		: (bHasMovement ? TEXT("이동") : (bHasAttack ? TEXT("공격") : TEXT("대기")));
	return Intent.mResult == EEnemyIntentResultUI::Executing
		? FText::FromString(FString::Printf(TEXT("%s[%d] %s · 실행 중"), RecommendedPrefix, DisplayOrder, Flow))
		: FText::FromString(FString::Printf(TEXT("%s[%d] %s"), RecommendedPrefix, DisplayOrder, Flow));
}

void UCombatTileMapHUDWidget::RefreshEnemyIntentPanel()
{
	if (mEnemyIntentPanel == nullptr || mEnemyIntentList == nullptr || WidgetTree == nullptr)
	{
		return;
	}
	if (mCombatControlsHidden)
	{
		mEnemyIntentPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	mEnemyIntentList->ClearChildren();
	const TArray<FEnemyIntentUI>* Intents = mCombatUIModel != nullptr
		? &mCombatUIModel->GetEnemyIntentUIs()
		: nullptr;
	if (Intents == nullptr || Intents->IsEmpty())
	{
		mEnemyIntentPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	auto MakeText = [this](const FString& Text, const FLinearColor& Color, int32 FontSize)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (TextBlock == nullptr)
		{
			return TextBlock;
		}
		TextBlock->SetText(FText::FromString(Text));
		TextBlock->SetAutoWrapText(false);
		TextBlock->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = FontSize;
		TextBlock->SetFont(Font);
		return TextBlock;
	};

	if (UTextBlock* Header = MakeText(
		TEXT("적 대응 예고  ·  회색=바꾸기 전(잠시) / 색상=지금 행동"),
		IntentCurrentColor,
		16))
	{
		if (UVerticalBoxSlot* HeaderSlot = mEnemyIntentList->AddChildToVerticalBox(Header))
		{
			HeaderSlot->SetPadding(FMargin(2.0f, 0.0f, 0.0f, 4.0f));
		}
	}

	for (int32 RowIndex = 0; RowIndex < Intents->Num(); ++RowIndex)
	{
		const FEnemyIntentUI& Intent = (*Intents)[RowIndex];
		const int32 DisplayOrder = Intent.mExecutionOrder > 0 ? Intent.mExecutionOrder : RowIndex + 1;
		const bool bActive = Intent.mResult == EEnemyIntentResultUI::Planned
			|| Intent.mResult == EEnemyIntentResultUI::Executing;
		const FLinearColor ExecutionColor = GetEnemyIntentExecutionColor(DisplayOrder);
		const FLinearColor RowColor = bActive
			? ExecutionColor
			: GetIntentStateColor(Intent.mResult, Intent.mWasDisplaced);
		const bool bHasMovement = Intent.mPlannedOrigin != FTileIndex::Invalid
			&& Intent.mPlannedDestination != FTileIndex::Invalid
			&& Intent.mPlannedDestination != Intent.mPlannedOrigin;
		const int32 MoveTileCount = FMath::Max(Intent.mPathTileIndexes.Num() - 1, 0);
		const bool bHasAttack = Intent.mTargetTile != FTileIndex::Invalid || Intent.mEffectTileIndexes.IsEmpty() == false;
		const FString MoveFlow = bHasMovement
			? FString::Printf(TEXT("이동 %d"), MoveTileCount)
			: TEXT("제자리");
		const FString AttackFlow = Intent.mActionName.IsEmpty()
			? TEXT("공격")
			: Intent.mActionName.ToString();
		const FString Flow = bHasAttack
			? FString::Printf(TEXT("%s → %s"), *MoveFlow, *AttackFlow)
			: (bHasMovement ? MoveFlow : TEXT("대기"));
		FString Status = GetIntentStateLabel(Intent.mResult).ToString();
		if (Intent.mWasDisplaced && bActive)
		{
			Status = FString::Printf(TEXT("재배치 대응 #%d · 이동력 -%d"), Intent.mPlanRevision, Intent.mResponseCostSpent);
		}
		else if (Intent.mResult == EEnemyIntentResultUI::Executing)
		{
			Status = TEXT("실행");
		}
		else if (Intent.mResult == EEnemyIntentResultUI::Planned)
		{
			Status = Intent.mPlanRevision > 0
				? FString::Printf(TEXT("대응 #%d · 이동력 -%d"), Intent.mPlanRevision, Intent.mResponseCostSpent)
				: TEXT("초기 계획");
		}
		const FString RouteChange = Intent.mPreviousDestination != FTileIndex::Invalid
			? FString::Printf(
				TEXT("  ·  목적지 (%d,%d)→(%d,%d)"),
				Intent.mPreviousDestination.mX,
				Intent.mPreviousDestination.mY,
				Intent.mPlannedDestination.mX,
				Intent.mPlannedDestination.mY)
			: FString();

		UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		if (RowBorder == nullptr)
		{
			continue;
		}
		const float TintStrength = Intent.mIsRecommendedInterventionTarget ? 0.16f : (bActive ? 0.09f : 0.045f);
		RowBorder->SetBrushColor(FLinearColor(
			0.018f + RowColor.R * TintStrength,
			0.026f + RowColor.G * TintStrength,
			0.030f + RowColor.B * TintStrength,
			0.88f));
		RowBorder->SetPadding(FMargin(7.0f, 5.0f));

		const FString RowText = FString::Printf(
			TEXT("%s[%d] %s  ·  %s\n    %s  ·  %s  ·  %s%s"),
			Intent.mIsRecommendedInterventionTarget ? TEXT("★ ") : TEXT(""),
			DisplayOrder,
			*Intent.mEnemyName.ToString(),
			*Intent.mDisplacementWeightLabel.ToString(),
			*Intent.mGoalText.ToString(),
			*Flow,
			*Status,
			*RouteChange);
		if (UTextBlock* RowLine = MakeText(RowText, RowColor, 14))
		{
			RowBorder->AddChild(RowLine);
		}

		if (UVerticalBoxSlot* RowSlot = mEnemyIntentList->AddChildToVerticalBox(RowBorder))
		{
			RowSlot->SetPadding(FMargin(0.0f, 1.0f, 0.0f, RowIndex + 1 < Intents->Num() ? 3.0f : 0.0f));
		}
	}

	mEnemyIntentPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCombatTileMapHUDWidget::UpdateEnemyIntentTutorial()
{
	if (mEnemyIntentTutorialPanel == nullptr
		|| mEnemyIntentTutorialProgress == nullptr
		|| mCombatUIModel == nullptr)
	{
		return;
	}
	auto SetEndTurnTutorialHighlight = [this](bool bHighlighted)
	{
		if (EndTurnButton == nullptr)
		{
			return;
		}
		EndTurnButton->SetBackgroundColor(bHighlighted
			? FLinearColor(1.0f, 0.68f, 0.08f, 0.62f)
			: (IsDesignerSkinActive()
				? FLinearColor(1.0f, 1.0f, 1.0f, 0.01f)
				: FLinearColor(0.32f, 0.08f, 0.07f, 0.95f)));
	};

	if (mCombatControlsHidden || mEnemyIntentTutorialDismissed)
	{
		SetEndTurnTutorialHighlight(false);
		SetEnemyIntentTutorialOverlayVisible(false);
		mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// 스킬 레일 튜토리얼: 스킬을 먼저 골라 실제 사거리를 본 뒤 전장의 적을 직접 드래그한다.
	{
		const TArray<FEnemyIntentUI>& DirectIntents = mCombatUIModel->GetEnemyIntentUIs();
		const TArray<FSkillUI>& DirectSkills = mCombatUIModel->GetSkillUIs();
		const EEnemyIntentTutorialStage DirectPreviousStage = mEnemyIntentTutorialStage;
		if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::WaitingForIntent)
		{
			const ARDGameModeBase* GameMode = GetWorld() != nullptr
				? Cast<ARDGameModeBase>(GetWorld()->GetAuthGameMode())
				: nullptr;
			const URunPersistData* RunData = GameMode != nullptr ? GameMode->GetRunPersistData() : nullptr;
			const bool bIsFirstCombat = RunData != nullptr
				&& RunData->GetStage().mStageLevel == EStageLevelType::Stage1
				&& RunData->GetStage().mCurRow == 0;
			if (bIsFirstCombat == false || FindRecommendedIntent(DirectIntents) == nullptr
				|| mCombatUIModel->GetTurnUI().mRound > 1)
			{
				SetEnemyIntentTutorialOverlayVisible(false);
				mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::Collapsed);
				return;
			}
			mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::SelectPull;
		}

		if (mEnemyIntentTutorialIntervenedEnemyUnitId == INDEX_NONE)
		{
			const FEnemyIntentUI* Displaced = DirectIntents.FindByPredicate([](const FEnemyIntentUI& Intent)
			{
				return Intent.mWasDisplaced;
			});
			if (Displaced != nullptr)
			{
				mEnemyIntentTutorialIntervenedEnemyUnitId = Displaced->mEnemyUnitId;
			}
		}
		const FEnemyIntentUI* DirectIntervened = FindIntentByEnemyId(
			DirectIntents, mEnemyIntentTutorialIntervenedEnemyUnitId);
		if (mEnemyIntentTutorialPullCompleted == false
			&& mEnemyIntentTutorialInterventionSubmitted
			&& DirectIntervened != nullptr
			&& DirectIntervened->mWasDisplaced)
		{
			mEnemyIntentTutorialPullCompleted = true;
			mEnemyIntentTutorialPullPlanRevision = DirectIntervened->mPlanRevision;
		}
		const bool bThrowReplanned = mEnemyIntentTutorialThrowSubmitted
			&& DirectIntervened != nullptr
			&& DirectIntervened->mPlanRevision > mEnemyIntentTutorialPullPlanRevision;
		if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::EndTurnAndObserve
			&& DirectIntervened != nullptr
			&& HasResolvedIntentOutcome(*DirectIntervened))
		{
			mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::Complete;
		}
		else if (bThrowReplanned)
		{
			mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::EndTurnAndObserve;
		}
		else if (mEnemyIntentTutorialThrowSubmitted)
		{
			mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::ApplyingThrow;
		}
		else if (mEnemyIntentTutorialPullCompleted)
		{
			const int32 SelectedSkillIndex = mCombatUIModel->GetSelectedSkillIndex();
			const bool bThrowSelected = DirectSkills.IsValidIndex(SelectedSkillIndex)
				&& DirectSkills[SelectedSkillIndex].mIsThrowSkill;
			if (bThrowSelected)
			{
				mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::ConfirmThrow;
			}
			else
			{
				mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::SelectThrow;
			}
		}
		else if (mEnemyIntentTutorialInterventionSubmitted)
		{
			mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::ApplyingIntervention;
		}
		else
		{
			const int32 SelectedSkillIndex = mCombatUIModel->GetSelectedSkillIndex();
			const bool bPullSelected = DirectSkills.IsValidIndex(SelectedSkillIndex)
				&& DirectSkills[SelectedSkillIndex].mIsPullSkill;
			if (bPullSelected)
			{
				mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::ConfirmDestination;
			}
			else
			{
				mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::SelectPull;
			}
		}

		FString DirectTitle;
		FString DirectMessage;
		switch (mEnemyIntentTutorialStage)
		{
		case EEnemyIntentTutorialStage::SelectPull:
			DirectTitle = TEXT("1 / 4   왼쪽에서 ‘끌어당기기’를 탭하세요");
			DirectMessage = TEXT("회색으로 밝혀지는 타일이 이 스킬의 실제 사거리입니다");
			break;
		case EEnemyIntentTutorialStage::ConfirmDestination:
			DirectTitle = TEXT("2 / 4   주황색 적을 기사 주변의 원하는 빈칸으로 드래그하세요");
			DirectMessage = TEXT("적은 손가락을 따라오고, ◆와 밝은 타일이 실제 도착칸을 보여줍니다");
			break;
		case EEnemyIntentTutorialStage::ApplyingIntervention:
			DirectTitle = TEXT("좋아요!  적을 끌어오는 중");
			DirectMessage = TEXT("도착할 때까지 잠깐 보세요");
			break;
		case EEnemyIntentTutorialStage::SelectThrow:
			DirectTitle = TEXT("3 / 4   왼쪽에서 ‘밀기 · 던지기’를 탭하세요");
			DirectMessage = TEXT("인접한 적만 밝아집니다 · 주사위는 자동으로 선택됩니다");
			break;
		case EEnemyIntentTutorialStage::ConfirmThrow:
			DirectTitle = TEXT("4 / 4   당겨온 적을 원하는 방향으로 드래그하세요");
			DirectMessage = TEXT("반투명 적이 멈춘 타일에서 손을 놓으면 던집니다");
			break;
		case EEnemyIntentTutorialStage::ApplyingThrow:
			DirectTitle = TEXT("좋아요!  던진 위치에 맞춰 적이 새 길을 고르는 중");
			DirectMessage = TEXT("");
			break;
		case EEnemyIntentTutorialStage::EndTurnAndObserve:
			DirectTitle = TEXT("마지막   오른쪽 아래 ‘턴 종료’를 누르세요");
			DirectMessage = TEXT("적이 바뀐 위치와 새 경로대로 행동하는지 확인합니다");
			break;
		case EEnemyIntentTutorialStage::Complete:
			DirectTitle = TEXT("완료!  이제 적을 직접 끌고 밀어 전장을 바꿀 수 있습니다");
			DirectMessage = TEXT("스킬 선택 → 사거리 확인 → 적 드래그");
			break;
		default:
			DirectTitle = TEXT("적을 직접 움직여 보세요");
			break;
		}
		if (mEnemyIntentTutorialTitle != nullptr)
		{
			mEnemyIntentTutorialTitle->SetText(FText::FromString(DirectTitle));
		}
		if (mEnemyIntentTutorialText != nullptr)
		{
			mEnemyIntentTutorialText->SetText(FText::FromString(DirectMessage));
			mEnemyIntentTutorialText->SetVisibility(DirectMessage.IsEmpty()
				? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}
		if (mEnemyIntentTutorialContinueButton != nullptr)
		{
			mEnemyIntentTutorialContinueButton->SetVisibility(ESlateVisibility::Collapsed);
		}
		SetEndTurnTutorialHighlight(mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::EndTurnAndObserve);
		RefreshEnemyIntentTutorialProgress();
		mEnemyIntentTutorialProgress->SetVisibility(ESlateVisibility::HitTestInvisible);
		mEnemyIntentTutorialPanel->SetBrushColor(mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::Complete
			? FLinearColor(0.025f, 0.18f, 0.11f, 0.92f)
			: FLinearColor(0.018f, 0.035f, 0.040f, 0.90f));
		mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (DirectPreviousStage != mEnemyIntentTutorialStage)
		{
			mEnemyIntentTutorialStageElapsed = 0.0f;
			RefreshOwnedDiceCards();
			RefreshDiceAssignmentText();
		}
		return;
	}

}

int32 UCombatTileMapHUDWidget::GetEnemyIntentTutorialRecommendedDiceIndex() const
{
	if (mCombatUIModel == nullptr)
	{
		return INDEX_NONE;
	}

	int32 BestDiceIndex = INDEX_NONE;
	int32 BestDiceValue = MIN_int32;
	const TArray<FDiceSlotUI>& DiceUIs = mCombatUIModel->GetDiceUIs();
	for (int32 DiceIndex = 0; DiceIndex < DiceUIs.Num(); ++DiceIndex)
	{
		const FDiceSlotUI& Dice = DiceUIs[DiceIndex];
		if (Dice.mIsRolled
			&& Dice.mIsUsed == false
			&& Dice.mIsSelected == false
			&& Dice.mResultValue > BestDiceValue)
		{
			BestDiceIndex = DiceIndex;
			BestDiceValue = Dice.mResultValue;
		}
	}
	return BestDiceIndex;
}

UWidget* UCombatTileMapHUDWidget::ResolveEnemyIntentTutorialFocusWidget() const
{
	if (mCombatUIModel == nullptr)
	{
		return nullptr;
	}

	switch (mEnemyIntentTutorialStage)
	{
	case EEnemyIntentTutorialStage::SelectPull:
	case EEnemyIntentTutorialStage::SelectThrow:
	{
		const bool bFindThrow = mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::SelectThrow;
		const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
		for (int32 SkillIndex = 0; SkillIndex < Skills.Num(); ++SkillIndex)
		{
			if (Skills[SkillIndex].mIsDisplacementSkill
				&& (bFindThrow ? Skills[SkillIndex].mIsThrowSkill : Skills[SkillIndex].mIsPullSkill))
			{
				for (int32 RailSlotIndex = 0; RailSlotIndex < mSkillInputButtons.Num(); ++RailSlotIndex)
				{
					if (GetSkillDataIndexForRailSlot(RailSlotIndex) == SkillIndex
						&& mSkillInputButtons[RailSlotIndex] != nullptr)
					{
						return mSkillInputButtons[RailSlotIndex].Get();
					}
				}
				return nullptr;
			}
		}
		return nullptr;
	}

	case EEnemyIntentTutorialStage::SelectDice:
	case EEnemyIntentTutorialStage::SelectThrowDice:
	{
		const int32 DiceIndex = GetEnemyIntentTutorialRecommendedDiceIndex();
		if (mOwnedDiceCardWidgets.IsValidIndex(DiceIndex)
			&& mOwnedDiceCardWidgets[DiceIndex] != nullptr)
		{
			return mOwnedDiceCardWidgets[DiceIndex].Get();
		}
		return nullptr;
	}

	case EEnemyIntentTutorialStage::SelectTarget:
	case EEnemyIntentTutorialStage::ConfirmDestination:
	case EEnemyIntentTutorialStage::SelectThrowTarget:
	case EEnemyIntentTutorialStage::ConfirmThrow:
	{
		const TArray<FEnemyIntentUI>& Intents = mCombatUIModel->GetEnemyIntentUIs();
		const bool bFindIntervened = mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::SelectThrowTarget
			|| mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::ConfirmThrow;
		const FEnemyIntentUI* TargetIntent = bFindIntervened
			? FindIntentByEnemyId(Intents, mEnemyIntentTutorialIntervenedEnemyUnitId)
			: FindRecommendedIntent(Intents);
		if (TargetIntent == nullptr && mEnemyIntentTutorialIntervenedEnemyUnitId != INDEX_NONE)
		{
			TargetIntent = FindIntentByEnemyId(Intents, mEnemyIntentTutorialIntervenedEnemyUnitId);
		}
		if (TargetIntent == nullptr)
		{
			return nullptr;
		}

		const TArray<FUnitUI>& Units = mCombatUIModel->GetUnitUIs();
		for (int32 UnitIndex = 0; UnitIndex < Units.Num(); ++UnitIndex)
		{
			if (Units[UnitIndex].mUnitId == TargetIntent->mEnemyUnitId
				&& mUnitHpBars.IsValidIndex(UnitIndex)
				&& mUnitHpBars[UnitIndex].mRoot != nullptr)
			{
				return mUnitHpBars[UnitIndex].mRoot.Get();
			}
		}
		return nullptr;
	}

	case EEnemyIntentTutorialStage::EndTurnAndObserve:
		return EndTurnButton.Get();
	case EEnemyIntentTutorialStage::SelectThrowDestination:
		for (UTextBlock* DirectionLabel : mDisplacementDirectionLabels)
		{
			if (DirectionLabel != nullptr && DirectionLabel->IsVisible())
			{
				return DirectionLabel;
			}
		}
		return nullptr;

	case EEnemyIntentTutorialStage::WaitingForIntent:
	case EEnemyIntentTutorialStage::ApplyingIntervention:
	case EEnemyIntentTutorialStage::ApplyingThrow:
	case EEnemyIntentTutorialStage::ReviewResult:
	case EEnemyIntentTutorialStage::Complete:
	default:
		return nullptr;
	}
}

void UCombatTileMapHUDWidget::SetEnemyIntentTutorialOverlayVisible(bool bVisible) const
{
	const ESlateVisibility OverlayVisibility = bVisible
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed;
	for (UBorder* Edge : mEnemyIntentTutorialFocusEdges)
	{
		if (Edge != nullptr) { Edge->SetVisibility(OverlayVisibility); }
	}
	for (UBorder* Part : mEnemyIntentTutorialArrowParts)
	{
		if (Part != nullptr) { Part->SetVisibility(OverlayVisibility); }
	}
	for (UBorder* DimPanel : mEnemyIntentTutorialDimPanels)
	{
		if (DimPanel != nullptr) { DimPanel->SetVisibility(OverlayVisibility); }
	}
	if (mEnemyIntentTutorialPointerLabel != nullptr)
	{
		mEnemyIntentTutorialPointerLabel->SetVisibility(OverlayVisibility);
	}
}

void UCombatTileMapHUDWidget::RefreshEnemyIntentTutorialProgress() const
{
	int32 CompletedCount = 0;
	int32 ActiveStep = INDEX_NONE;
	switch (mEnemyIntentTutorialStage)
	{
	case EEnemyIntentTutorialStage::SelectPull:            ActiveStep = 0; break;
	case EEnemyIntentTutorialStage::ConfirmDestination:
	case EEnemyIntentTutorialStage::ApplyingIntervention:  CompletedCount = 1; ActiveStep = 1; break;
	case EEnemyIntentTutorialStage::SelectThrow:           CompletedCount = 2; ActiveStep = 2; break;
	case EEnemyIntentTutorialStage::ConfirmThrow:
	case EEnemyIntentTutorialStage::ApplyingThrow:         CompletedCount = 3; ActiveStep = 3; break;
	case EEnemyIntentTutorialStage::EndTurnAndObserve:
	case EEnemyIntentTutorialStage::Complete:              CompletedCount = 4; break;
	case EEnemyIntentTutorialStage::WaitingForIntent:
	default:                                               break;
	}

	for (int32 StepIndex = 0; StepIndex < mEnemyIntentTutorialProgressDots.Num(); ++StepIndex)
	{
		UBorder* Dot = mEnemyIntentTutorialProgressDots[StepIndex].Get();
		if (Dot == nullptr)
		{
			continue;
		}
		if (StepIndex >= 4)
		{
			Dot->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		Dot->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (StepIndex < CompletedCount)
		{
			Dot->SetBrushColor(FLinearColor(0.20f, 0.96f, 0.58f, 1.0f));
		}
		else if (StepIndex == ActiveStep)
		{
			Dot->SetBrushColor(FLinearColor(1.0f, 0.66f, 0.06f, 1.0f));
		}
		else
		{
			Dot->SetBrushColor(FLinearColor(0.22f, 0.28f, 0.29f, 0.95f));
		}
		Dot->SetRenderOpacity(1.0f);
	}
}

void UCombatTileMapHUDWidget::UpdateEnemyIntentTutorialVisuals(float InDeltaTime)
{
	if (mEnemyIntentTutorialPanel == nullptr || RootCanvas == nullptr)
	{
		return;
	}

	const bool bTemporarilyHidden = mCombatControlsHidden
		|| mEnemyIntentTutorialDismissed
		|| mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::WaitingForIntent
		|| mIntroDiceRollReady
		|| mIntroDiceRollActive
		|| mIntroDiceResultWaitingForDismiss
		|| mTurnChangeIntroPlaying
		|| IsSkillDetailVisible();
	if (bTemporarilyHidden)
	{
		SetEnemyIntentTutorialOverlayVisible(false);
		mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	mEnemyIntentTutorialStageElapsed += FMath::Max(InDeltaTime, 0.0f);
	mEnemyIntentTutorialPulseTime += FMath::Max(InDeltaTime, 0.0f);

	// 조작 단계는 실제 스킬/주사위/타깃 상태를 관찰해 진행한다. 마지막 성공 토스트만
	// 잠깐 보여준 뒤 자동으로 닫아 플레이를 다시 가리지 않는다.
	mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::Complete
		&& mEnemyIntentTutorialStageElapsed >= 2.6f)
	{
		HandleEnemyIntentTutorialContinue();
		return;
	}
	const bool bConfirmStage = mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::ConfirmDestination
		|| mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::ConfirmThrow;
	const float PulseSpeed = bConfirmStage ? 10.0f : 5.5f;
	const float Pulse01 = 0.5f + 0.5f * FMath::Sin(mEnemyIntentTutorialPulseTime * PulseSpeed);

	int32 ActiveStep = INDEX_NONE;
	switch (mEnemyIntentTutorialStage)
	{
	case EEnemyIntentTutorialStage::SelectPull:           ActiveStep = 0; break;
	case EEnemyIntentTutorialStage::ConfirmDestination:
	case EEnemyIntentTutorialStage::ApplyingIntervention: ActiveStep = 1; break;
	case EEnemyIntentTutorialStage::SelectThrow:          ActiveStep = 2; break;
	case EEnemyIntentTutorialStage::ConfirmThrow:
	case EEnemyIntentTutorialStage::ApplyingThrow:        ActiveStep = 3; break;
	default:                                           break;
	}
	if (mEnemyIntentTutorialProgressDots.IsValidIndex(ActiveStep)
		&& mEnemyIntentTutorialProgressDots[ActiveStep] != nullptr)
	{
		mEnemyIntentTutorialProgressDots[ActiveStep]->SetRenderOpacity(0.58f + 0.42f * Pulse01);
	}

	UWidget* FocusWidget = ResolveEnemyIntentTutorialFocusWidget();
	if (FocusWidget == nullptr || FocusWidget->IsVisible() == false)
	{
		SetEnemyIntentTutorialOverlayVisible(false);
		return;
	}

	const FGeometry& RootGeometry = RootCanvas->GetCachedGeometry();
	const FGeometry& FocusGeometry = FocusWidget->GetCachedGeometry();
	const FVector2D FocusLocalSize = FocusGeometry.GetLocalSize();
	const FVector2D RootLocalSize = RootGeometry.GetLocalSize();
	if (FocusLocalSize.X <= 1.0f || FocusLocalSize.Y <= 1.0f
		|| RootLocalSize.X <= 1.0f || RootLocalSize.Y <= 1.0f)
	{
		SetEnemyIntentTutorialOverlayVisible(false);
		return;
	}

	const FVector2D AbsoluteA = FocusGeometry.LocalToAbsolute(FVector2D::ZeroVector);
	const FVector2D AbsoluteB = FocusGeometry.LocalToAbsolute(FocusLocalSize);
	const FVector2D RootA = RootGeometry.AbsoluteToLocal(AbsoluteA);
	const FVector2D RootB = RootGeometry.AbsoluteToLocal(AbsoluteB);
	constexpr float FocusPadding = 8.0f;
	FVector2D MinPoint;
	FVector2D MaxPoint;
	const bool bFocusEnemyModel = mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::SelectTarget
		|| mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::ConfirmDestination
		|| mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::SelectThrowTarget
		|| mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::ConfirmThrow;
	if (bFocusEnemyModel)
	{
		// HP바 슬롯은 실제 유닛 투영점보다 60px 위에 놓인다(UnitBars.cpp). HP바만 감싸면
		// 클릭할 수 없는 라벨을 가리키는 것처럼 보이므로, 그 아래의 3D 모델 몸체를 직접 감싼다.
		const FVector2D HpBarBottomAbsolute = FocusGeometry.LocalToAbsolute(
			FVector2D(FocusLocalSize.X * 0.5f, FocusLocalSize.Y));
		const FVector2D HpBarBottom = RootGeometry.AbsoluteToLocal(HpBarBottomAbsolute);
		const FVector2D UnitGroundPoint = HpBarBottom + FVector2D(0.0f, 60.0f);
		const float ModelHalfWidth = FMath::Max(FMath::Abs(RootB.X - RootA.X) * 0.58f, 62.0f);
		MinPoint = UnitGroundPoint - FVector2D(ModelHalfWidth + FocusPadding, 138.0f + FocusPadding);
		MaxPoint = UnitGroundPoint + FVector2D(ModelHalfWidth + FocusPadding, 22.0f + FocusPadding);
	}
	else
	{
		MinPoint = FVector2D(
			FMath::Min(RootA.X, RootB.X) - FocusPadding,
			FMath::Min(RootA.Y, RootB.Y) - FocusPadding);
		MaxPoint = FVector2D(
			FMath::Max(RootA.X, RootB.X) + FocusPadding,
			FMath::Max(RootA.Y, RootB.Y) + FocusPadding);
	}
	MinPoint.X = FMath::Clamp(MinPoint.X, 1.0f, RootLocalSize.X - 1.0f);
	MinPoint.Y = FMath::Clamp(MinPoint.Y, 1.0f, RootLocalSize.Y - 1.0f);
	MaxPoint.X = FMath::Clamp(MaxPoint.X, 1.0f, RootLocalSize.X - 1.0f);
	MaxPoint.Y = FMath::Clamp(MaxPoint.Y, 1.0f, RootLocalSize.Y - 1.0f);
	const FVector2D FocusSize = MaxPoint - MinPoint;
	if (FocusSize.X <= 2.0f || FocusSize.Y <= 2.0f
		|| mEnemyIntentTutorialFocusEdges.Num() != 4
		|| mEnemyIntentTutorialArrowParts.Num() != 3)
	{
		SetEnemyIntentTutorialOverlayVisible(false);
		return;
	}

	if (mEnemyIntentTutorialDimPanels.Num() == 4)
	{
		auto PlaceDim = [&](int32 Index, const FVector2D& Position, const FVector2D& Size)
		{
			UBorder* DimPanel = mEnemyIntentTutorialDimPanels[Index].Get();
			if (DimPanel == nullptr) { return; }
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(DimPanel->Slot))
			{
				Slot->SetAlignment(FVector2D::ZeroVector);
				Slot->SetPosition(Position);
				Slot->SetSize(FVector2D(FMath::Max(Size.X, 0.0f), FMath::Max(Size.Y, 0.0f)));
			}
			DimPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
		};
		PlaceDim(0, FVector2D::ZeroVector, FVector2D(RootLocalSize.X, MinPoint.Y));
		PlaceDim(1, FVector2D(0.0f, MaxPoint.Y), FVector2D(RootLocalSize.X, RootLocalSize.Y - MaxPoint.Y));
		PlaceDim(2, FVector2D(0.0f, MinPoint.Y), FVector2D(MinPoint.X, FocusSize.Y));
		PlaceDim(3, FVector2D(MaxPoint.X, MinPoint.Y), FVector2D(RootLocalSize.X - MaxPoint.X, FocusSize.Y));
	}

	const FLinearColor FocusColor = mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::Complete
		? FLinearColor(0.20f, 0.96f, 0.58f, 1.0f)
		: (bConfirmStage
			? FLinearColor(1.0f, 0.82f, 0.22f, 1.0f)
			: FLinearColor(1.0f, 0.66f, 0.06f, 1.0f));
	const float OverlayOpacity = 0.62f + 0.38f * Pulse01;
	const float EdgeThickness = bConfirmStage ? 7.0f : 5.0f;
	auto PlaceEdge = [&](int32 EdgeIndex, const FVector2D& Position, const FVector2D& Size)
	{
		UBorder* Edge = mEnemyIntentTutorialFocusEdges[EdgeIndex].Get();
		if (Edge == nullptr) { return; }
		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Edge->Slot))
		{
			Slot->SetAlignment(FVector2D::ZeroVector);
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
		}
		Edge->SetRenderTransformAngle(0.0f);
		Edge->SetBrushColor(FocusColor);
		Edge->SetRenderOpacity(OverlayOpacity);
		Edge->SetVisibility(ESlateVisibility::HitTestInvisible);
	};
	PlaceEdge(0, MinPoint, FVector2D(FocusSize.X, EdgeThickness));
	PlaceEdge(1, FVector2D(MinPoint.X, MaxPoint.Y - EdgeThickness), FVector2D(FocusSize.X, EdgeThickness));
	PlaceEdge(2, MinPoint, FVector2D(EdgeThickness, FocusSize.Y));
	PlaceEdge(3, FVector2D(MaxPoint.X - EdgeThickness, MinPoint.Y), FVector2D(EdgeThickness, FocusSize.Y));

	FVector2D ArrowDirection(0.0f, 1.0f);
	FVector2D ArrowTip((MinPoint.X + MaxPoint.X) * 0.5f, MinPoint.Y - 9.0f);
	if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::EndTurnAndObserve)
	{
		ArrowDirection = FVector2D(1.0f, 0.0f);
		ArrowTip = FVector2D(MinPoint.X - 9.0f, (MinPoint.Y + MaxPoint.Y) * 0.5f);
	}
	else if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::SelectDice
		|| mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::SelectThrowDice)
	{
		ArrowDirection = FVector2D(-1.0f, 0.0f);
		ArrowTip = FVector2D(MaxPoint.X + 9.0f, (MinPoint.Y + MaxPoint.Y) * 0.5f);
	}
	else if (MinPoint.Y < 55.0f)
	{
		ArrowDirection = FVector2D(0.0f, -1.0f);
		ArrowTip = FVector2D((MinPoint.X + MaxPoint.X) * 0.5f, MaxPoint.Y + 9.0f);
	}

	// 화살표가 대상 쪽으로 살짝 왕복해 다음 클릭 위치를 설명문 없이 전달한다.
	ArrowTip -= ArrowDirection * (2.0f + 3.0f * Pulse01);
	const float ArrowAngle = FMath::RadiansToDegrees(FMath::Atan2(ArrowDirection.Y, ArrowDirection.X));
	constexpr float ShaftLength = 28.0f;
	constexpr float HeadLength = 15.0f;
	const float LineThickness = bConfirmStage ? 6.5f : 5.0f;
	auto PlaceArrowLine = [&](int32 PartIndex, const FVector2D& Center, float Length, float Angle)
	{
		UBorder* Part = mEnemyIntentTutorialArrowParts[PartIndex].Get();
		if (Part == nullptr) { return; }
		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Part->Slot))
		{
			Slot->SetAlignment(FVector2D(0.5f, 0.5f));
			Slot->SetPosition(Center);
			Slot->SetSize(FVector2D(Length, LineThickness));
		}
		Part->SetRenderTransformAngle(Angle);
		Part->SetBrushColor(FocusColor);
		Part->SetRenderOpacity(OverlayOpacity);
		Part->SetVisibility(ESlateVisibility::HitTestInvisible);
	};
	PlaceArrowLine(0, ArrowTip - ArrowDirection * 24.0f, ShaftLength, ArrowAngle);
	for (int32 HeadIndex = 0; HeadIndex < 2; ++HeadIndex)
	{
		const float HeadAngle = ArrowAngle + (HeadIndex == 0 ? -45.0f : 45.0f);
		const float HeadRadians = FMath::DegreesToRadians(HeadAngle);
		const FVector2D HeadDirection(FMath::Cos(HeadRadians), FMath::Sin(HeadRadians));
		PlaceArrowLine(
			HeadIndex + 1,
			ArrowTip - HeadDirection * (HeadLength * 0.5f),
			HeadLength,
			HeadAngle);
	}

	if (mEnemyIntentTutorialPointerLabel != nullptr)
	{
		FString PointerText = TEXT("클릭");
		switch (mEnemyIntentTutorialStage)
		{
		case EEnemyIntentTutorialStage::SelectPull:        PointerText = TEXT("끌어당기기 선택 · 사거리 보기"); break;
		case EEnemyIntentTutorialStage::SelectDice:        PointerText = TEXT("가장 큰 숫자 클릭"); break;
		case EEnemyIntentTutorialStage::SelectTarget:      PointerText = TEXT("이 적을 한 번 탭"); break;
		case EEnemyIntentTutorialStage::SelectThrow:       PointerText = TEXT("밀기 · 던지기 선택"); break;
		case EEnemyIntentTutorialStage::SelectThrowDice:   PointerText = TEXT("새 숫자 클릭"); break;
		case EEnemyIntentTutorialStage::SelectThrowTarget: PointerText = TEXT("당겨온 적을 한 번 탭"); break;
		case EEnemyIntentTutorialStage::SelectThrowDestination: PointerText = TEXT("던질 방향 클릭"); break;
		case EEnemyIntentTutorialStage::ConfirmDestination: PointerText = TEXT("같은 적을 원하는 ◆ 칸으로 드래그"); break;
		case EEnemyIntentTutorialStage::ConfirmThrow:      PointerText = TEXT("같은 적을 던질 방향으로 드래그"); break;
		case EEnemyIntentTutorialStage::EndTurnAndObserve: PointerText = TEXT("턴 종료 클릭"); break;
		default: break;
		}
		mEnemyIntentTutorialPointerLabel->SetText(FText::FromString(PointerText));
		if (UCanvasPanelSlot* PointerSlot = Cast<UCanvasPanelSlot>(mEnemyIntentTutorialPointerLabel->Slot))
		{
			FVector2D LabelPosition = ArrowTip - ArrowDirection * 58.0f;
			LabelPosition.X = FMath::Clamp(LabelPosition.X - 80.0f, 8.0f, RootLocalSize.X - 250.0f);
			LabelPosition.Y = FMath::Clamp(LabelPosition.Y - 13.0f, 8.0f, RootLocalSize.Y - 35.0f);
			PointerSlot->SetPosition(LabelPosition);
		}
		mEnemyIntentTutorialPointerLabel->SetRenderOpacity(0.72f + 0.28f * Pulse01);
		mEnemyIntentTutorialPointerLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UCombatTileMapHUDWidget::HandleEnemyIntentTutorialContinue()
{
	if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::Complete)
	{
		mEnemyIntentTutorialDismissed = true;
		mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::Collapsed);
		SetEnemyIntentTutorialOverlayVisible(false);
		if (EndTurnButton != nullptr)
		{
			EndTurnButton->SetBackgroundColor(IsDesignerSkinActive()
				? FLinearColor(1.0f, 1.0f, 1.0f, 0.01f)
				: FLinearColor(0.32f, 0.08f, 0.07f, 0.95f));
		}
		RefreshSkillRailWidgets();
		RefreshOwnedDiceCards();
		return;
	}

	if (mEnemyIntentTutorialStage != EEnemyIntentTutorialStage::ReviewResult)
	{
		return;
	}

	mEnemyIntentTutorialResultAcknowledged = true;
	UpdateEnemyIntentTutorial();
}
