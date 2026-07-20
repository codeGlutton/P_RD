#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
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

	const USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	const int32 Round = CombatModel != nullptr ? CombatModel->GetRoundCount() : 1;
	const FEnemyIntentUI& FlowState = (*Intents)[0];
	int32 LivingEnemyRows = 0;
	for (const FEnemyIntentUI& Intent : *Intents)
	{
		if (Intent.mIsReinforcementWarning == false
			&& (Intent.mResult == EEnemyIntentResultUI::Planned || Intent.mResult == EEnemyIntentResultUI::Executing))
		{
			++LivingEnemyRows;
		}
	}
	const FString MomentumPips = FString::ChrN(FMath::Clamp(FlowState.mWarriorMomentum, 0, 3), TEXT('◆'))
		+ FString::ChrN(FMath::Clamp(3 - FlowState.mWarriorMomentum, 0, 3), TEXT('◇'));
	const FString FlowPips = FString::ChrN(FMath::Clamp(FlowState.mWarriorFlow, 0, 3), TEXT('◆'))
		+ FString::ChrN(FMath::Clamp(3 - FlowState.mWarriorFlow, 0, 3), TEXT('◇'));
	const FString PressurePips = FString::ChrN(FMath::Clamp(FlowState.mStandstillPressure, 0, 3), TEXT('■'))
		+ FString::ChrN(FMath::Clamp(3 - FlowState.mStandstillPressure, 0, 3), TEXT('□'));
	const FString HeaderText = FString::Printf(
		TEXT("ROUND %d/8  ·  적 %d/%d  ·  COMBO %d%s\n기세 %s  ·  FLOW %s  ·  압박 %s\n%s"),
		FMath::Clamp(Round, 1, 8),
		LivingEnemyRows,
		FMath::Max(FlowState.mEnemyCap, LivingEnemyRows),
		FlowState.mWarriorCombo,
		FlowState.mPendingReinforcementCount > 0
			? *FString::Printf(TEXT("  ·  다음 증원 %d"), FlowState.mPendingReinforcementCount)
			: TEXT(""),
		*MomentumPips,
		*FlowPips,
		*PressurePips,
		*FlowState.mMovementDangerLabel.ToString());
	if (UTextBlock* Header = MakeText(
		HeaderText,
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
		if (Intent.mIsReinforcementWarning)
		{
			UBorder* WarningBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			if (WarningBorder == nullptr) { continue; }
			WarningBorder->SetBrushColor(FLinearColor(0.24f, 0.07f, 0.01f, 0.90f));
			WarningBorder->SetPadding(FMargin(7.0f, 5.0f));
			const FString WarningText = FString::Printf(
				TEXT("⚠ 다음 증원  ·  주황 타일 (%d,%d)\n    기술 착지로 선점: 증원 넘어짐 + FLOW/COMBO 보상"),
				Intent.mCurrentTile.mX,
				Intent.mCurrentTile.mY);
			if (UTextBlock* WarningLine = MakeText(WarningText, FLinearColor(1.0f, 0.52f, 0.12f, 1.0f), 14))
			{
				WarningBorder->AddChild(WarningLine);
			}
			if (UVerticalBoxSlot* WarningSlot = mEnemyIntentList->AddChildToVerticalBox(WarningBorder))
			{
				WarningSlot->SetPadding(FMargin(0.0f, 1.0f, 0.0f, 3.0f));
			}
			continue;
		}
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
		const FString Destination = Intent.mPlannedDestination != FTileIndex::Invalid
			? FString::Printf(TEXT("(%d,%d)"), Intent.mPlannedDestination.mX, Intent.mPlannedDestination.mY)
			: TEXT("현재 칸");

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
			TEXT("%s[%d] %s  ·  %s\n    목적지 %s  ·  위험 %d칸  ·  %s"),
			Intent.mIsRecommendedInterventionTarget ? TEXT("★ ") : TEXT(""),
			DisplayOrder,
			*Intent.mEnemyName.ToString(),
			*Flow,
			*Destination,
			Intent.mEffectTileIndexes.Num(),
			*Status);
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

	if (mCombatControlsHidden || mEnemyIntentTutorialDismissed)
	{
		SetEnemyIntentTutorialOverlayVisible(false);
		mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const TArray<FEnemyIntentUI>& Intents = mCombatUIModel->GetEnemyIntentUIs();
	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	const EEnemyIntentTutorialStage PreviousStage = mEnemyIntentTutorialStage;
	if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::WaitingForIntent)
	{
		const ARDGameModeBase* GameMode = GetWorld() != nullptr
			? Cast<ARDGameModeBase>(GetWorld()->GetAuthGameMode()) : nullptr;
		const URunPersistData* RunData = GameMode != nullptr ? GameMode->GetRunPersistData() : nullptr;
		const bool bFirstCombat = RunData != nullptr
			&& RunData->GetStage().mStageLevel == EStageLevelType::Stage1
			&& RunData->GetStage().mCurRow == 0;
		if (bFirstCombat == false || FindRecommendedIntent(Intents) == nullptr
			|| mCombatUIModel->GetTurnUI().mRound > 1)
		{
			SetEnemyIntentTutorialOverlayVisible(false);
			mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}
		mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::OpenAttack;
	}

	int32 PlayerUnitId = INDEX_NONE;
	for (const FUnitUI& Unit : mCombatUIModel->GetUnitUIs())
	{
		if (Unit.mIsPlayer) { PlayerUnitId = Unit.mUnitId; break; }
	}
	const bool bPlayerTurn = PlayerUnitId != INDEX_NONE
		&& mCombatUIModel->GetTurnUI().mCurrentUnitId == PlayerUnitId;
	if (mEnemyIntentTutorialStrikeSubmitted && bPlayerTurn == false)
	{
		mEnemyIntentTutorialSawStrikeEnemyTurn = true;
	}
	if (mEnemyIntentTutorialSawStrikeEnemyTurn && bPlayerTurn)
	{
		mEnemyIntentTutorialStrikeCycleComplete = true;
	}
	if (mEnemyIntentTutorialIntervenedEnemyUnitId == INDEX_NONE)
	{
		if (const FEnemyIntentUI* Displaced = Intents.FindByPredicate([](const FEnemyIntentUI& Intent) { return Intent.mWasDisplaced; }))
		{
			mEnemyIntentTutorialIntervenedEnemyUnitId = Displaced->mEnemyUnitId;
		}
	}
	const FEnemyIntentUI* Intervened = FindIntentByEnemyId(Intents, mEnemyIntentTutorialIntervenedEnemyUnitId);
	if (mEnemyIntentTutorialInterventionSubmitted && Intervened != nullptr && Intervened->mWasDisplaced)
	{
		mEnemyIntentTutorialPullCompleted = true;
	}
	if (mEnemyIntentTutorialPullCompleted && bPlayerTurn == false)
	{
		mEnemyIntentTutorialSawEnemyTurn = true;
	}

	if (mEnemyIntentTutorialStage != EEnemyIntentTutorialStage::Complete)
	{
		if (mEnemyIntentTutorialSawEnemyTurn && bPlayerTurn)
		{
			mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::Complete;
		}
		else if (mEnemyIntentTutorialPullCompleted)
		{
			mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::ObserveResponse;
		}
		else if (mEnemyIntentTutorialInterventionSubmitted)
		{
			mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::ApplyingIntervention;
		}
		else if (mEnemyIntentTutorialStrikeCycleComplete)
		{
			const int32 SelectedSkillIndex = mCombatUIModel->GetSelectedSkillIndex();
			const bool bPullSelected = Skills.IsValidIndex(SelectedSkillIndex) && Skills[SelectedSkillIndex].mIsPullSkill;
			mEnemyIntentTutorialStage = bPullSelected
				? EEnemyIntentTutorialStage::ConfirmDestination
				: (mExpandedActionFamily == 1 ? EEnemyIntentTutorialStage::SelectPull : EEnemyIntentTutorialStage::OpenGrip);
		}
		else if (mEnemyIntentTutorialStrikeSubmitted)
		{
			mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::ObserveStrike;
		}
		else if (mSelectedSubactionMode == ECombatSubactionMode::BasicAttack)
		{
			mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::StrikeTarget;
		}
		else if (mExpandedActionFamily == 0)
		{
			mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::SelectSlash;
		}
		else
		{
			mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::OpenAttack;
		}
	}

	FString Title;
	FString Message;
	switch (mEnemyIntentTutorialStage)
	{
	case EEnemyIntentTutorialStage::OpenAttack:
		Title = TEXT("1 / 8   왼쪽 ‘공격’을 누르세요");
		Message = TEXT("네 개의 큰 카드는 행동군입니다. 먼저 공격 안의 기술을 엽니다."); break;
	case EEnemyIntentTutorialStage::SelectSlash:
		Title = TEXT("2 / 8   ‘관통 베기’를 누르세요");
		Message = TEXT("밝아진 전체 타일이 사거리입니다. 이 기술은 이동과 공격을 한 번에 합니다."); break;
	case EEnemyIntentTutorialStage::StrikeTarget:
		Title = TEXT("3 / 8   노란 테두리 적을 한 번 누르세요");
		Message = TEXT("기사가 적 뒤 빈 칸으로 순간 이동한 뒤 바로 벱니다."); break;
	case EEnemyIntentTutorialStage::ObserveStrike:
		Title = TEXT("4 / 8   기술 하나가 끝나면 적 전원이 한 번씩 행동합니다");
		Message = TEXT("오른쪽 번호가 행동 순서입니다. 다시 내 차례가 올 때까지 잠깐 보세요."); break;
	case EEnemyIntentTutorialStage::OpenGrip:
		Title = TEXT("5 / 8   이제 왼쪽 ‘손아귀’를 누르세요");
		Message = TEXT("손아귀는 적을 직접 배치해 충돌과 포위를 만드는 행동군입니다."); break;
	case EEnemyIntentTutorialStage::SelectPull:
		Title = TEXT("6 / 8   ‘후퇴 견인’을 누르세요");
		Message = TEXT("기사는 뒤로 빠지고, 고른 적은 기사 주변의 원하는 칸으로 옵니다."); break;
	case EEnemyIntentTutorialStage::ConfirmDestination:
		Title = TEXT("7 / 8   강조된 적을 누른 채 밝은 ◆ 칸에 놓으세요");
		Message = TEXT("반투명 적이 실제 착지 모습입니다. 손을 놓은 칸에 딱 맞춰 배치됩니다."); break;
	case EEnemyIntentTutorialStage::ApplyingIntervention:
		Title = TEXT("좋아요!   후퇴와 견인을 한 기술 안에서 처리 중");
		Message = TEXT("별도 후속 행동은 없습니다."); break;
	case EEnemyIntentTutorialStage::ObserveResponse:
		Title = TEXT("8 / 8   적 전원의 대응과 전장 보상을 확인하세요");
		Message = TEXT("주황=증원 요격 · 금색=충돌 보상 · 붉은 칸=다음 기술로 이탈"); break;
	case EEnemyIntentTutorialStage::Complete:
		Title = TEXT("준비 완료 · 모든 기술은 ‘이동 + 효과’ 한 번입니다");
		Message = TEXT("공격/손아귀/제압/기동을 열고, 착지 칸까지 생각해 적 무리를 헤집으세요."); break;
	default: break;
	}
	if (mEnemyIntentTutorialTitle != nullptr) { mEnemyIntentTutorialTitle->SetText(FText::FromString(Title)); }
	if (mEnemyIntentTutorialText != nullptr)
	{
		mEnemyIntentTutorialText->SetText(FText::FromString(Message));
		mEnemyIntentTutorialText->SetVisibility(Message.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (mEnemyIntentTutorialContinueButton != nullptr) { mEnemyIntentTutorialContinueButton->SetVisibility(ESlateVisibility::Collapsed); }
	RefreshEnemyIntentTutorialProgress();
	mEnemyIntentTutorialProgress->SetVisibility(ESlateVisibility::HitTestInvisible);
	mEnemyIntentTutorialPanel->SetBrushColor(mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::Complete
		? FLinearColor(0.025f, 0.18f, 0.11f, 0.92f) : FLinearColor(0.018f, 0.035f, 0.040f, 0.90f));
	mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (PreviousStage != mEnemyIntentTutorialStage)
	{
		mEnemyIntentTutorialStageElapsed = 0.0f;
		RefreshSkillRailWidgets();
	}
}

UWidget* UCombatTileMapHUDWidget::ResolveEnemyIntentTutorialFocusWidget() const
{
	if (mCombatUIModel == nullptr)
	{
		return nullptr;
	}

	switch (mEnemyIntentTutorialStage)
	{
	case EEnemyIntentTutorialStage::OpenAttack:
		return mSkillInputButtons.IsValidIndex(0) && mSkillInputButtons[0] != nullptr
			? mSkillInputButtons[0].Get() : nullptr;
	case EEnemyIntentTutorialStage::SelectSlash:
		if (mExpandedActionFamily == 0)
		{
			for (int32 SlotIndex = 0; SlotIndex < mActionSubmenuModes.Num(); ++SlotIndex)
			{
				if (mActionSubmenuModes[SlotIndex] == ECombatSubactionMode::BasicAttack
					&& mActionSubmenuButtons.IsValidIndex(SlotIndex) && mActionSubmenuButtons[SlotIndex] != nullptr)
				{
					return mActionSubmenuButtons[SlotIndex].Get();
				}
			}
		}
		return mSkillInputButtons.IsValidIndex(0) ? mSkillInputButtons[0].Get() : nullptr;
	case EEnemyIntentTutorialStage::StrikeTarget:
	{
		const FEnemyIntentUI* TargetIntent = FindRecommendedIntent(mCombatUIModel->GetEnemyIntentUIs());
		if (TargetIntent == nullptr) { return nullptr; }
		for (int32 UnitIndex = 0; UnitIndex < mCombatUIModel->GetUnitUIs().Num(); ++UnitIndex)
		{
			if (mCombatUIModel->GetUnitUIs()[UnitIndex].mUnitId == TargetIntent->mEnemyUnitId
				&& mUnitHpBars.IsValidIndex(UnitIndex) && mUnitHpBars[UnitIndex].mRoot != nullptr)
			{
				return mUnitHpBars[UnitIndex].mRoot.Get();
			}
		}
		return nullptr;
	}
	case EEnemyIntentTutorialStage::ObserveStrike:
		return mEnemyIntentPanel;
	case EEnemyIntentTutorialStage::OpenGrip:
		return mSkillInputButtons.IsValidIndex(1) && mSkillInputButtons[1] != nullptr
			? mSkillInputButtons[1].Get()
			: nullptr;

	case EEnemyIntentTutorialStage::SelectPull:
	{
		if (mExpandedActionFamily == 1)
		{
			for (int32 SlotIndex = 0; SlotIndex < mActionSubmenuModes.Num(); ++SlotIndex)
			{
				if (mActionSubmenuModes[SlotIndex] == ECombatSubactionMode::Pull
					&& mActionSubmenuButtons.IsValidIndex(SlotIndex)
					&& mActionSubmenuButtons[SlotIndex] != nullptr)
				{
					return mActionSubmenuButtons[SlotIndex].Get();
				}
			}
		}
		return mSkillInputButtons.IsValidIndex(1) && mSkillInputButtons[1] != nullptr
			? mSkillInputButtons[1].Get()
			: nullptr;
	}

	case EEnemyIntentTutorialStage::ConfirmDestination:
	{
		const TArray<FEnemyIntentUI>& Intents = mCombatUIModel->GetEnemyIntentUIs();
		const FEnemyIntentUI* TargetIntent = FindRecommendedIntent(Intents);
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

	case EEnemyIntentTutorialStage::ObserveResponse:
		return mEnemyIntentPanel;

	case EEnemyIntentTutorialStage::WaitingForIntent:
	case EEnemyIntentTutorialStage::ApplyingIntervention:
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
	case EEnemyIntentTutorialStage::OpenAttack:              ActiveStep = 0; break;
	case EEnemyIntentTutorialStage::SelectSlash:             CompletedCount = 1; ActiveStep = 1; break;
	case EEnemyIntentTutorialStage::StrikeTarget:            CompletedCount = 2; ActiveStep = 2; break;
	case EEnemyIntentTutorialStage::ObserveStrike:           CompletedCount = 3; ActiveStep = 3; break;
	case EEnemyIntentTutorialStage::OpenGrip:                CompletedCount = 4; ActiveStep = 4; break;
	case EEnemyIntentTutorialStage::SelectPull:              CompletedCount = 5; ActiveStep = 5; break;
	case EEnemyIntentTutorialStage::ConfirmDestination:
	case EEnemyIntentTutorialStage::ApplyingIntervention:    CompletedCount = 6; ActiveStep = 6; break;
	case EEnemyIntentTutorialStage::ObserveResponse:         CompletedCount = 7; ActiveStep = 7; break;
	case EEnemyIntentTutorialStage::Complete:                CompletedCount = 8; break;
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

	// 조작 단계는 실제 스킬/타깃 상태를 관찰해 진행한다. 마지막 성공 토스트만
	// 잠깐 보여준 뒤 자동으로 닫아 플레이를 다시 가리지 않는다.
	mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::Complete
		&& mEnemyIntentTutorialStageElapsed >= 2.6f)
	{
		HandleEnemyIntentTutorialContinue();
		return;
	}
	const bool bConfirmStage = mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::ConfirmDestination;
	const float PulseSpeed = bConfirmStage ? 10.0f : 5.5f;
	const float Pulse01 = 0.5f + 0.5f * FMath::Sin(mEnemyIntentTutorialPulseTime * PulseSpeed);

	int32 ActiveStep = INDEX_NONE;
	switch (mEnemyIntentTutorialStage)
	{
	case EEnemyIntentTutorialStage::OpenAttack:             ActiveStep = 0; break;
	case EEnemyIntentTutorialStage::SelectSlash:            ActiveStep = 1; break;
	case EEnemyIntentTutorialStage::StrikeTarget:           ActiveStep = 2; break;
	case EEnemyIntentTutorialStage::ObserveStrike:          ActiveStep = 3; break;
	case EEnemyIntentTutorialStage::OpenGrip:               ActiveStep = 4; break;
	case EEnemyIntentTutorialStage::SelectPull:             ActiveStep = 5; break;
	case EEnemyIntentTutorialStage::ConfirmDestination:
	case EEnemyIntentTutorialStage::ApplyingIntervention:   ActiveStep = 6; break;
	case EEnemyIntentTutorialStage::ObserveResponse:        ActiveStep = 7; break;
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
	const bool bFocusUnitModel = mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::StrikeTarget
		|| mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::ConfirmDestination;
	if (bFocusUnitModel)
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
	if (MinPoint.Y < 55.0f)
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
		case EEnemyIntentTutorialStage::OpenAttack: PointerText = TEXT("공격 열기"); break;
		case EEnemyIntentTutorialStage::SelectSlash: PointerText = TEXT("관통 베기 선택"); break;
		case EEnemyIntentTutorialStage::StrikeTarget: PointerText = TEXT("이 적 한 번 누르기"); break;
		case EEnemyIntentTutorialStage::ObserveStrike: PointerText = TEXT("번호 순서 보기"); break;
		case EEnemyIntentTutorialStage::OpenGrip: PointerText = TEXT("여기 누르기"); break;
		case EEnemyIntentTutorialStage::SelectPull:
			PointerText = TEXT("끌어오기 누르기");
			break;
		case EEnemyIntentTutorialStage::ConfirmDestination: PointerText = TEXT("누른 채 ◆ 칸으로 끌기"); break;
		case EEnemyIntentTutorialStage::ObserveResponse: PointerText = TEXT("결과와 보상 보기"); break;
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
		RefreshSkillRailWidgets();
		return;
	}

}
