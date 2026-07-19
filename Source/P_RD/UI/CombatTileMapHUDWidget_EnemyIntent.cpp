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
			Parts.Add(FString::Printf(TEXT("…%s"), *FormatIntentTile(Path.Last())));
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

	bool HasResolvedIntentOutcome(const TArray<FEnemyIntentUI>& Intents)
	{
		for (const FEnemyIntentUI& Intent : Intents)
		{
			if (Intent.mResult != EEnemyIntentResultUI::Planned
				&& Intent.mResult != EEnemyIntentResultUI::Executing)
			{
				return true;
			}
		}
		return false;
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
		Header->SetText(NSLOCTEXT("CombatTileMapHUDWidget", "EnemyIntentHeader", "적 행동 예고  ·  계획 고정"));
		Header->SetColorAndOpacity(FSlateColor(FLinearColor(0.42f, 1.0f, 0.83f, 1.0f)));
		FSlateFontInfo Font = Header->GetFont();
		Font.Size = 22;
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
		const FString InterventionMark = Intent.mWasDisplaced ? TEXT("  [개입됨]") : FString();
		const FText StateLabel = GetIntentStateLabel(Intent.mResult);
		const FString Result = Intent.mResultText.IsEmpty() ? StateLabel.ToString() : Intent.mResultText.ToString();
		const FString RowString = FString::Printf(
			TEXT("%d. %s  ·  %s%s\n표적 %s   경로 %s\n결과  %s"),
			DisplayOrder,
			*Intent.mEnemyName.ToString(),
			*Intent.mActionName.ToString(),
			*InterventionMark,
			*FormatIntentTile(Intent.mTargetTile),
			*FormatIntentPath(Intent.mPathTileIndexes),
			*Result);

		UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (Row == nullptr)
		{
			continue;
		}
		Row->SetText(FText::FromString(RowString));
		Row->SetAutoWrapText(true);
		Row->SetLineHeightPercentage(1.04f);
		Row->SetColorAndOpacity(FSlateColor(GetIntentStateColor(Intent.mResult, Intent.mWasDisplaced)));
		FSlateFontInfo Font = Row->GetFont();
		Font.Size = 17;
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
	if (mCombatControlsHidden)
	{
		mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const TArray<FEnemyIntentUI>& Intents = mCombatUIModel->GetEnemyIntentUIs();
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
		if (bIsFirstCombat == false || Intents.IsEmpty() || mCombatUIModel->GetTurnUI().mRound > 1)
		{
			mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}
		mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::ReviewIntent;
	}

	if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::SelectSmash)
	{
		const int32 SelectedSkill = mCombatUIModel->GetSelectedSkillIndex();
		const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
		if (Skills.IsValidIndex(SelectedSkill) && Skills[SelectedSkill].mIsDisplacementSkill)
		{
			mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::SelectDice;
		}
	}

	if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::SelectDice
		&& mCombatUIModel->GetSelectedDiceIndices().IsEmpty() == false)
	{
		mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::TargetIntervention;
	}

	if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::TargetIntervention)
	{
		for (const FEnemyIntentUI& Intent : Intents)
		{
			if (Intent.mWasDisplaced)
			{
				mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::ObserveOutcome;
				break;
			}
		}
	}

	if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::ObserveOutcome
		&& HasResolvedIntentOutcome(Intents))
	{
		mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::Complete;
	}

	FText Instruction;
	switch (mEnemyIntentTutorialStage)
	{
	case EEnemyIntentTutorialStage::ReviewIntent:
		Instruction = NSLOCTEXT("CombatTileMapHUDWidget", "IntentTutorialReview", "1 / 5  우상단 예고에서 적의 행동, 표적, 이동 경로를 확인하세요. 이 계획은 이미 고정되어 바뀌지 않습니다.");
		break;
	case EEnemyIntentTutorialStage::SelectSmash:
		Instruction = NSLOCTEXT("CombatTileMapHUDWidget", "IntentTutorialSmash", "2 / 5  왼쪽 스킬 레일에서 ‘강타’를 선택하세요. 강타는 피해 숫자뿐 아니라 적의 위치를 바꿉니다.");
		break;
	case EEnemyIntentTutorialStage::SelectDice:
		Instruction = NSLOCTEXT("CombatTileMapHUDWidget", "IntentTutorialDice", "3 / 5  굴린 주사위 숫자를 선택하세요. 선택한 숫자만큼 대상을 밀 수 있습니다.");
		break;
	case EEnemyIntentTutorialStage::TargetIntervention:
		Instruction = NSLOCTEXT("CombatTileMapHUDWidget", "IntentTutorialTarget", "4 / 5  전장의 적을 선택해 밀어내세요. 다른 적의 경로나 공격선에 넣으면 계획이 스스로 망가집니다.");
		break;
	case EEnemyIntentTutorialStage::ObserveOutcome:
		Instruction = NSLOCTEXT("CombatTileMapHUDWidget", "IntentTutorialObserve", "5 / 5  개입 완료! 턴을 마치고, 적이 재계산 없이 예고한 행동을 실행해 빗나가거나 충돌하는 결과를 확인하세요.");
		break;
	case EEnemyIntentTutorialStage::Complete:
		Instruction = NSLOCTEXT("CombatTileMapHUDWidget", "IntentTutorialComplete", "튜토리얼 완료  ·  적의 계획을 읽고, 주사위로 위치를 바꿔, 고정된 행동을 역이용했습니다.");
		break;
	case EEnemyIntentTutorialStage::WaitingForIntent:
	default:
		mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	mEnemyIntentTutorialText->SetText(Instruction);
	mEnemyIntentTutorialContinueButton->SetVisibility(
		mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::ReviewIntent
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	mEnemyIntentTutorialPanel->SetBrushColor(
		mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::Complete
			? FLinearColor(0.05f, 0.25f, 0.18f, 0.94f)
			: FLinearColor(0.030f, 0.060f, 0.065f, 0.92f));
	mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UCombatTileMapHUDWidget::HandleEnemyIntentTutorialContinue()
{
	if (mEnemyIntentTutorialStage != EEnemyIntentTutorialStage::ReviewIntent)
	{
		return;
	}

	mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::SelectSmash;
	UpdateEnemyIntentTutorial();
}
