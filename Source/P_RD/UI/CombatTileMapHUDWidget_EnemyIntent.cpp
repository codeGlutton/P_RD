#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "PCGStage/Stage.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Combat/CombatUIModel.h"

namespace
{
	FString FormatIntentTile(const FTileIndex& Tile)
	{
		return Tile == FTileIndex::Invalid
			? TEXT("-")
			: FString::Printf(TEXT("(%d,%d)"), Tile.mX, Tile.mY);
	}

	FString FormatIntentPath(const TArray<FTileIndex>& Path)
	{
		if (Path.IsEmpty())
		{
			return TEXT("-");
		}

		TArray<FString> Parts;
		const int32 VisibleCount = FMath::Min(Path.Num(), 5);
		Parts.Reserve(VisibleCount + 1);
		for (int32 Index = 0; Index < VisibleCount; ++Index)
		{
			Parts.Add(FormatIntentTile(Path[Index]));
		}
		if (Path.Num() > VisibleCount)
		{
			Parts.Add(FString::Printf(TEXT("… %s"), *FormatIntentTile(Path.Last())));
		}
		return FString::Join(Parts, TEXT(" → "));
	}

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
		default:                                 return NSLOCTEXT("CombatTileMapHUDWidget", "IntentStatePlanned", "고정됨");
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

	UTextBlock* Header = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (Header != nullptr)
	{
		Header->SetText(NSLOCTEXT(
			"CombatTileMapHUDWidget",
			"EnemyIntentHeader",
			"적 행동 예고  ·  계획 고정\n★ 연습 대상  ·  밝은 타일 = 예정 이동 경로"));
		Header->SetColorAndOpacity(FSlateColor(FLinearColor(0.42f, 1.0f, 0.83f, 1.0f)));
		Header->SetLineHeightPercentage(1.02f);
		FSlateFontInfo Font = Header->GetFont();
		Font.Size = 20;
		Header->SetFont(Font);
		if (UVerticalBoxSlot* HeaderSlot = mEnemyIntentList->AddChildToVerticalBox(Header))
		{
			HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
	}

	for (int32 RowIndex = 0; RowIndex < Intents->Num(); ++RowIndex)
	{
		const FEnemyIntentUI& Intent = (*Intents)[RowIndex];
		const int32 DisplayOrder = Intent.mExecutionOrder > 0 ? Intent.mExecutionOrder : RowIndex + 1;
		const FString RecommendedMark = Intent.mIsRecommendedInterventionTarget ? TEXT("★ ") : FString();
		const FString InterventionMark = Intent.mWasDisplaced ? TEXT("  [개입됨]") : FString();
		const FText StateLabel = GetIntentStateLabel(Intent.mResult);
		const bool bHasMovement = Intent.mPlannedOrigin != FTileIndex::Invalid
			&& Intent.mPlannedDestination != FTileIndex::Invalid
			&& Intent.mPlannedDestination != Intent.mPlannedOrigin;
		const int32 MoveTileCount = FMath::Max(Intent.mPathTileIndexes.Num() - 1, 0);
		const FString Movement = bHasMovement
			? FString::Printf(
				TEXT("예정 이동  %d칸  ·  경로 %s"),
				MoveTileCount,
				*FormatIntentPath(Intent.mPathTileIndexes))
			: TEXT("예정 이동 없음");
		const bool bHasAttack = Intent.mTargetTile != FTileIndex::Invalid || Intent.mEffectTileIndexes.IsEmpty() == false;
		const FString Attack = bHasAttack
			? FString::Printf(TEXT("예정 공격  %s → %s"), *Intent.mActionName.ToString(), *FormatIntentTile(Intent.mTargetTile))
			: TEXT("예정 공격 없음");
		const FString Outcome = Intent.mResult == EEnemyIntentResultUI::Planned
			? FString::Printf(TEXT("상태  %s%s"), *StateLabel.ToString(), Intent.mWasDisplaced ? TEXT("  ·  출발점 변경") : TEXT(""))
			: FString::Printf(TEXT("결과  %s  ·  %s"), *StateLabel.ToString(), *GetLatestIntentMessage(Intent));
		const FString Hint = Intent.mIsRecommendedInterventionTarget && Intent.mWasDisplaced == false
			? TEXT("\n개입 힌트  이 적을 밀면 고정 이동이 출발점에서 취소됩니다.")
			: FString();
		const FString RowString = FString::Printf(
			TEXT("%s%d. %s  ·  %s%s\n%s\n%s  ·  %s%s"),
			*RecommendedMark,
			DisplayOrder,
			*Intent.mEnemyName.ToString(),
			*Intent.mActionName.ToString(),
			*InterventionMark,
			*Movement,
			*Attack,
			*Outcome,
			*Hint);

		UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (Row == nullptr)
		{
			continue;
		}
		Row->SetText(FText::FromString(RowString));
		Row->SetAutoWrapText(true);
		Row->SetLineHeightPercentage(1.04f);
		Row->SetColorAndOpacity(FSlateColor(
			Intent.mIsRecommendedInterventionTarget && Intent.mWasDisplaced == false
				? FLinearColor(1.0f, 0.84f, 0.34f, 1.0f)
				: GetIntentStateColor(Intent.mResult, Intent.mWasDisplaced)));
		FSlateFontInfo Font = Row->GetFont();
		Font.Size = 16;
		Row->SetFont(Font);
		if (UVerticalBoxSlot* RowSlot = mEnemyIntentList->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, RowIndex + 1 < Intents->Num() ? 8.0f : 0.0f));
		}
	}

	mEnemyIntentPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCombatTileMapHUDWidget::UpdateEnemyIntentTutorial()
{
	if (mEnemyIntentTutorialPanel == nullptr
		|| mEnemyIntentTutorialText == nullptr
		|| mEnemyIntentTutorialContinueButton == nullptr
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
		mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const TArray<FEnemyIntentUI>& Intents = mCombatUIModel->GetEnemyIntentUIs();
	const EEnemyIntentTutorialStage PreviousStage = mEnemyIntentTutorialStage;
	if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::WaitingForIntent)
	{
		// Stage 1 시작 방의 첫 라운드에서만 자동 시작한다. 이후 전투에는 예고 패널만 남긴다.
		const ARDGameModeBase* GameMode = GetWorld() != nullptr
			? Cast<ARDGameModeBase>(GetWorld()->GetAuthGameMode())
			: nullptr;
		const URunPersistData* RunData = GameMode != nullptr ? GameMode->GetRunPersistData() : nullptr;
		const bool bIsFirstCombat = RunData != nullptr
			&& RunData->GetStage().mStageLevel == EStageLevelType::Stage1
			&& RunData->GetStage().mCurRow == 0;
		if (bIsFirstCombat == false
			|| FindRecommendedIntent(Intents) == nullptr
			|| mCombatUIModel->GetTurnUI().mRound > 1)
		{
			mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}
		mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::ReviewIntent;
	}

	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	const int32 SmashSkillIndex = Skills.IndexOfByPredicate([](const FSkillUI& Skill)
	{
		return Skill.mIsDisplacementSkill;
	});
	const FSkillUI* SmashSkill = Skills.IsValidIndex(SmashSkillIndex) ? &Skills[SmashSkillIndex] : nullptr;
	const int32 RequiredDiceCount = SmashSkill != nullptr ? FMath::Max(SmashSkill->mDiceCost, 0) : 0;
	const int32 SelectedSkillIndex = mCombatUIModel->GetSelectedSkillIndex();
	const bool bSmashSelected = Skills.IsValidIndex(SelectedSkillIndex)
		&& Skills[SelectedSkillIndex].mIsDisplacementSkill;
	const int32 SelectedDiceCount = mCombatUIModel->GetSelectedDiceIndices().Num();
	const ECombatBuildPhaseUI BuildPhase = mCombatUIModel->GetTurnUI().mPhase;
	bool bCommittedSelectedDice = RequiredDiceCount > 0 && SelectedDiceCount == RequiredDiceCount;
	for (int32 DiceIndex : mCombatUIModel->GetSelectedDiceIndices())
	{
		const TArray<FDiceSlotUI>& DiceUIs = mCombatUIModel->GetDiceUIs();
		if (DiceUIs.IsValidIndex(DiceIndex) == false || DiceUIs[DiceIndex].mIsUsed == false)
		{
			bCommittedSelectedDice = false;
			break;
		}
	}
	if (mEnemyIntentTutorialInterventionSubmitted == false
		&& PreviousStage == EEnemyIntentTutorialStage::ConfirmTarget
		&& BuildPhase == ECombatBuildPhaseUI::None
		&& bSmashSelected
		&& bCommittedSelectedDice)
	{
		// 같은 타일 두 번째 클릭으로 BuildSkill이 주사위를 사용 처리한 순간이다. 실제 밀치기 보고는
		// 스킬/이동 연출 뒤에 오므로 그 사이 선택 초기화를 보고 2단계로 역행하지 않게 별도 보존한다.
		mEnemyIntentTutorialInterventionSubmitted = true;
	}

	if (mEnemyIntentTutorialIntervenedEnemyUnitId == INDEX_NONE)
	{
		const FEnemyIntentUI* DisplacedIntent = Intents.FindByPredicate([](const FEnemyIntentUI& Intent)
		{
			return Intent.mWasDisplaced && Intent.mIsRecommendedInterventionTarget;
		});
		if (DisplacedIntent == nullptr)
		{
			DisplacedIntent = Intents.FindByPredicate([](const FEnemyIntentUI& Intent)
			{
				return Intent.mWasDisplaced;
			});
		}
		if (DisplacedIntent != nullptr)
		{
			mEnemyIntentTutorialIntervenedEnemyUnitId = DisplacedIntent->mEnemyUnitId;
		}
	}

	const FEnemyIntentUI* IntervenedIntent = FindIntentByEnemyId(Intents, mEnemyIntentTutorialIntervenedEnemyUnitId);
	if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::Complete)
	{
		// 완료 안내는 다음 라운드의 새 Planned 스냅샷이 같은 적 ID로 들어와도 닫기 전까지 유지한다.
	}
	else if (IntervenedIntent != nullptr && HasResolvedIntentOutcome(*IntervenedIntent))
	{
		mEnemyIntentTutorialCompletedEnemyName = IntervenedIntent->mEnemyName.ToString();
		mEnemyIntentTutorialCompletedResult = GetLatestIntentMessage(*IntervenedIntent);
		mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::Complete;
	}
	else if (IntervenedIntent != nullptr && IntervenedIntent->mWasDisplaced)
	{
		mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::EndTurnAndObserve;
	}
	else if (mEnemyIntentTutorialInterventionSubmitted)
	{
		mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::ApplyingIntervention;
	}
	else if (mEnemyIntentTutorialReviewAcknowledged == false)
	{
		mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::ReviewIntent;
	}
	else if (bSmashSelected == false)
	{
		mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::SelectSmash;
	}
	else if (RequiredDiceCount <= 0 || SelectedDiceCount != RequiredDiceCount)
	{
		mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::SelectDice;
	}
	else if (BuildPhase == ECombatBuildPhaseUI::Preview)
	{
		mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::ConfirmTarget;
	}
	else
	{
		mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::SelectTarget;
	}

	const FEnemyIntentUI* RecommendedIntent = FindRecommendedIntent(Intents);
	const int32 RecommendedOrder = RecommendedIntent != nullptr && RecommendedIntent->mExecutionOrder > 0
		? RecommendedIntent->mExecutionOrder
		: 1;
	const FString RecommendedName = RecommendedIntent != nullptr
		? RecommendedIntent->mEnemyName.ToString()
		: TEXT("★ 연습 대상");
	const FString SmashName = SmashSkill != nullptr && SmashSkill->mName.IsEmpty() == false
		? SmashSkill->mName.ToString()
		: TEXT("강타");
	int32 SmashRailSlot = INDEX_NONE;
	for (int32 RailSlotIndex = 0; RailSlotIndex < mSkillRailPanels.Num(); ++RailSlotIndex)
	{
		if (GetSkillDataIndexForRailSlot(RailSlotIndex) == SmashSkillIndex)
		{
			SmashRailSlot = RailSlotIndex + 1;
			break;
		}
	}
	const FString RailPosition = SmashRailSlot != INDEX_NONE
		? FString::Printf(TEXT("위에서 %d번째"), SmashRailSlot)
		: TEXT("금색으로 강조된");
	const FString MoveNotice = TEXT("\n\n※ MOVE 0은 정상입니다. 이 연습에서는 MOVE를 사용하지 않습니다.");

	FString Instruction;
	switch (mEnemyIntentTutorialStage)
	{
	case EEnemyIntentTutorialStage::ReviewIntent:
		Instruction = TEXT("[1 / 6]  적 계획 읽기\n우측 예고의 ★ 연습 대상과 밝은 이동 경로를 확인하세요. 적은 상황이 바뀌어도 이 계획을 다시 계산하지 않습니다.");
		break;
	case EEnemyIntentTutorialStage::SelectSmash:
		Instruction = FString::Printf(
			TEXT("[2 / 6]  %s 선택\n왼쪽 큰 스킬 아이콘 중 %s ‘%s’를 짧게 클릭하세요. 금색 강조가 대상입니다.\n큰 아이콘과 오른쪽 흰 숫자는 행별로 짝지어진 구조가 아닙니다.%s"),
			*SmashName,
			*RailPosition,
			*SmashName,
			*MoveNotice);
		break;
	case EEnemyIntentTutorialStage::SelectDice:
		Instruction = FString::Printf(
			TEXT("[3 / 6]  주사위 %d개 선택\n왼쪽의 독립된 흰 숫자 칸 중 사용 가능한 주사위를 정확히 %d개 클릭하세요. 선택한 눈의 합만큼 적이 밀립니다.\n현재 선택  %d / %d%s"),
			RequiredDiceCount,
			RequiredDiceCount,
			SelectedDiceCount,
			RequiredDiceCount,
			*MoveNotice);
		break;
	case EEnemyIntentTutorialStage::SelectTarget:
		Instruction = FString::Printf(
			TEXT("[4 / 6]  ★ %d번 %s을 한 번 클릭\n우측 예고에 ★로 표시된 적입니다. 전장의 해당 적 모델(강조 타일)을 한 번 클릭하세요. 첫 클릭은 실행이 아니라 미리보기입니다.%s"),
			RecommendedOrder,
			*RecommendedName,
			*MoveNotice);
		break;
	case EEnemyIntentTutorialStage::ConfirmTarget:
		Instruction = FString::Printf(
			TEXT("[5 / 6]  같은 적을 한 번 더 클릭\n지금은 미리보기 상태입니다. 방금 누른 같은 적(같은 타일)을 두 번째로 클릭해 강타를 확정하세요.%s"),
			*MoveNotice);
		break;
	case EEnemyIntentTutorialStage::ApplyingIntervention:
		Instruction = FString::Printf(
			TEXT("강타 개입 처리 중\n적을 밀어내는 연출이 끝나면 다음 안내로 자동 전환됩니다. 지금은 다른 스킬이나 주사위를 다시 누르지 마세요.%s"),
			*MoveNotice);
		break;
	case EEnemyIntentTutorialStage::EndTurnAndObserve:
	{
		const FString IntervenedName = IntervenedIntent != nullptr
			? IntervenedIntent->mEnemyName.ToString()
			: RecommendedName;
		Instruction = FString::Printf(
			TEXT("[6 / 6]  END TURN\n%s의 출발점을 바꿔 놓았습니다. 오른쪽 아래 END TURN을 누르고, 이 적이 원래 고정 경로를 재계산하지 못해 행동을 취소하는 결과를 확인하세요.%s"),
			*IntervenedName,
			*MoveNotice);
		break;
	}
	case EEnemyIntentTutorialStage::Complete:
	{
		const FString CompletedName = mEnemyIntentTutorialCompletedEnemyName.IsEmpty() == false
			? mEnemyIntentTutorialCompletedEnemyName
			: (IntervenedIntent != nullptr
			? IntervenedIntent->mEnemyName.ToString()
			: RecommendedName);
		const FString Result = mEnemyIntentTutorialCompletedResult.IsEmpty() == false
			? mEnemyIntentTutorialCompletedResult
			: (IntervenedIntent != nullptr
			? GetLatestIntentMessage(*IntervenedIntent)
			: TEXT("고정 행동이 망가졌습니다."));
		Instruction = FString::Printf(
			TEXT("튜토리얼 완료\n%s 결과: %s\n적은 밀린 뒤에도 예정된 행동을 다시 계산하지 않았습니다."),
			*CompletedName,
			*Result);
		break;
	}
	case EEnemyIntentTutorialStage::WaitingForIntent:
	default:
		SetEndTurnTutorialHighlight(false);
		mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	mEnemyIntentTutorialText->SetText(FText::FromString(Instruction));
	const bool bShowTutorialButton = mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::ReviewIntent
		|| mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::Complete;
	mEnemyIntentTutorialContinueButton->SetVisibility(
		bShowTutorialButton
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	if (mEnemyIntentTutorialContinueText != nullptr)
	{
		mEnemyIntentTutorialContinueText->SetText(
			mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::Complete
				? NSLOCTEXT("CombatTileMapHUDWidget", "IntentTutorialClose", "닫기")
				: NSLOCTEXT("CombatTileMapHUDWidget", "IntentTutorialStart", "튜토리얼 시작"));
	}
	mEnemyIntentTutorialPanel->SetBrushColor(
		mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::Complete
			? FLinearColor(0.05f, 0.25f, 0.18f, 0.94f)
			: (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::ConfirmTarget
				|| mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::ApplyingIntervention
				? FLinearColor(0.30f, 0.20f, 0.035f, 0.95f)
				: FLinearColor(0.030f, 0.060f, 0.065f, 0.94f)));
	SetEndTurnTutorialHighlight(mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::EndTurnAndObserve);
	mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// 도메인 알림보다 튜토리얼 단계가 한 박자 늦게 정해지므로, 단계가 바뀐 그 프레임에
	// 스킬/주사위 강조도 다시 그려 사용자가 다음 입력 위치를 즉시 볼 수 있게 한다.
	if (PreviousStage != mEnemyIntentTutorialStage)
	{
		RefreshSkillRailWidgets();
		RefreshOwnedDiceCards();
	}
}

void UCombatTileMapHUDWidget::HandleEnemyIntentTutorialContinue()
{
	if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::Complete)
	{
		mEnemyIntentTutorialDismissed = true;
		mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::Collapsed);
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

	if (mEnemyIntentTutorialStage != EEnemyIntentTutorialStage::ReviewIntent)
	{
		return;
	}

	mEnemyIntentTutorialReviewAcknowledged = true;
	UpdateEnemyIntentTutorial();
}
