#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/Actor.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/IndexedButtonWidget.h"

namespace
{
	constexpr int32 ContextActionSlotCount = 6;
	constexpr int32 ContextMoveAction = -2;
	constexpr float ContextActionWidth = 104.0f;
	constexpr float ContextActionHeight = 70.0f;
	constexpr float ContextActionGap = 8.0f;
	constexpr float DirectDragThreshold = 42.0f;

	int32 GetTileDistance(const FTileIndex& A, const FTileIndex& B)
	{
		return FMath::Max(FMath::Abs(A.mX - B.mX), FMath::Abs(A.mY - B.mY));
	}

	int32 GetMaximumAvailableDiceSum(const TArray<FDiceSlotUI>& Dice, int32 DiceCost)
	{
		if (DiceCost <= 0)
		{
			return 0;
		}
		TArray<int32> Values;
		for (const FDiceSlotUI& Die : Dice)
		{
			if (Die.mIsRolled && Die.mIsUsed == false)
			{
				Values.Add(Die.mResultValue);
			}
		}
		Values.Sort(TGreater<int32>());
		if (Values.Num() < DiceCost)
		{
			return INDEX_NONE;
		}
		int32 Sum = 0;
		for (int32 Index = 0; Index < DiceCost; ++Index)
		{
			Sum += Values[Index];
		}
		return Sum;
	}

	TArray<int32> PickAutomaticDice(const TArray<FDiceSlotUI>& Dice, int32 DiceCost, int32 DesiredPower)
	{
		TArray<int32> Available;
		for (int32 Index = 0; Index < Dice.Num(); ++Index)
		{
			if (Dice[Index].mIsRolled && Dice[Index].mIsUsed == false && Dice[Index].mIsSelected == false)
			{
				Available.Add(Index);
			}
		}
		TArray<int32> Best;
		TArray<int32> Current;
		int32 BestScore = TNumericLimits<int32>::Max();
		int32 BestSum = TNumericLimits<int32>::Max();
		TFunction<void(int32, int32)> Visit = [&](int32 Start, int32 Sum)
		{
			if (Current.Num() == DiceCost)
			{
				const int32 Score = FMath::Abs(Sum - DesiredPower);
				if (Score < BestScore || (Score == BestScore && Sum < BestSum))
				{
					BestScore = Score;
					BestSum = Sum;
					Best = Current;
				}
				return;
			}
			for (int32 Cursor = Start; Cursor < Available.Num(); ++Cursor)
			{
				Current.Add(Available[Cursor]);
				Visit(Cursor + 1, Sum + Dice[Available[Cursor]].mResultValue);
				Current.Pop();
			}
		};
		if (DiceCost <= 0)
		{
			return Best;
		}
		Visit(0, 0);
		return Best;
	}
}

void UCombatTileMapHUDWidget::EnsureContextActionWidgets()
{
	if (RootCanvas == nullptr || WidgetTree == nullptr || mContextActionButtons.Num() == ContextActionSlotCount)
	{
		return;
	}

	mContextActionTitleText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("ContextActionTitleText"));
	if (mContextActionTitleText != nullptr)
	{
		FSlateFontInfo Font = mContextActionTitleText->GetFont();
		Font.Size = 17;
		Font.OutlineSettings.OutlineSize = 2;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.94f);
		mContextActionTitleText->SetFont(Font);
		mContextActionTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.90f, 0.38f, 1.0f)));
		mContextActionTitleText->SetJustification(ETextJustify::Center);
		mContextActionTitleText->SetVisibility(ESlateVisibility::Collapsed);
		RootCanvas->AddChildToCanvas(mContextActionTitleText);
	}

	mDirectUnitGestureLine = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DirectUnitGestureLine"));
	mDirectUnitGestureHandle = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DirectUnitGestureHandle"));
	mDirectUnitGestureLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DirectUnitGestureLabel"));
	if (mDirectUnitGestureLine != nullptr && mDirectUnitGestureHandle != nullptr && mDirectUnitGestureLabel != nullptr)
	{
		mDirectUnitGestureLine->SetBrushColor(FLinearColor(0.16f, 1.0f, 0.78f, 0.92f));
		mDirectUnitGestureLine->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		mDirectUnitGestureHandle->SetBrushColor(FLinearColor(1.0f, 0.78f, 0.16f, 0.96f));
		mDirectUnitGestureLabel->SetJustification(ETextJustify::Center);
		mDirectUnitGestureLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		FSlateFontInfo GestureFont = mDirectUnitGestureLabel->GetFont();
		GestureFont.Size = 17;
		GestureFont.OutlineSettings.OutlineSize = 2;
		GestureFont.OutlineSettings.OutlineColor = FLinearColor::Black;
		mDirectUnitGestureLabel->SetFont(GestureFont);
		mDirectUnitGestureLine->SetVisibility(ESlateVisibility::Collapsed);
		mDirectUnitGestureHandle->SetVisibility(ESlateVisibility::Collapsed);
		mDirectUnitGestureLabel->SetVisibility(ESlateVisibility::Collapsed);
		RootCanvas->AddChildToCanvas(mDirectUnitGestureLine);
		RootCanvas->AddChildToCanvas(mDirectUnitGestureHandle);
		RootCanvas->AddChildToCanvas(mDirectUnitGestureLabel);
	}

	for (int32 SlotIndex = 0; SlotIndex < ContextActionSlotCount; ++SlotIndex)
	{
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), FName(*FString::Printf(TEXT("ContextActionPanel_%d"), SlotIndex)));
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), FName(*FString::Printf(TEXT("ContextActionIcon_%d"), SlotIndex)));
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), FName(*FString::Printf(TEXT("ContextActionText_%d"), SlotIndex)));
		UIndexedButtonWidget* Button = WidgetTree->ConstructWidget<UIndexedButtonWidget>(
			UIndexedButtonWidget::StaticClass(), FName(*FString::Printf(TEXT("ContextActionButton_%d"), SlotIndex)));
		if (Panel == nullptr || Icon == nullptr || Label == nullptr || Button == nullptr)
		{
			continue;
		}

		Panel->SetPadding(FMargin(3.0f));
		Panel->SetBrushColor(FLinearColor(0.018f, 0.055f, 0.075f, 0.94f));
		Panel->SetVisibility(ESlateVisibility::Collapsed);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		Label->SetJustification(ETextJustify::Center);
		Label->SetAutoWrapText(false);
		Label->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
		Label->SetLineHeightPercentage(0.86f);
		Label->SetVisibility(ESlateVisibility::Collapsed);
		FSlateFontInfo LabelFont = Label->GetFont();
		LabelFont.Size = 13;
		LabelFont.OutlineSettings.OutlineSize = 1;
		LabelFont.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.90f);
		Label->SetFont(LabelFont);
		Button->SetButtonIndex(SlotIndex);
		Button->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.01f));
		Button->SetVisibility(ESlateVisibility::Collapsed);
		Button->OnIndexedClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleContextActionClicked);

		RootCanvas->AddChildToCanvas(Panel);
		RootCanvas->AddChildToCanvas(Icon);
		RootCanvas->AddChildToCanvas(Label);
		RootCanvas->AddChildToCanvas(Button);
		mContextActionPanels.Add(Panel);
		mContextActionIcons.Add(Icon);
		mContextActionTexts.Add(Label);
		mContextActionButtons.Add(Button);
	}
}

bool UCombatTileMapHUDWidget::FindUnitAtScreenPosition(
	const FVector2D& ScreenPosition,
	int32& OutUnitId,
	bool& OutIsPlayer,
	FVector2D& OutUnitScreenPosition) const
{
	OutUnitId = INDEX_NONE;
	OutIsPlayer = false;
	OutUnitScreenPosition = FVector2D::ZeroVector;
	if (mCombatControlsHidden || mCombatUIModel == nullptr || RootCanvas == nullptr)
	{
		return false;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController == nullptr)
	{
		return false;
	}

	const FGeometry& RootGeometry = RootCanvas->GetCachedGeometry();
	const TArray<FUnitUI>& Units = mCombatUIModel->GetUnitUIs();
	const FUnitUI* BestUnit = nullptr;
	FVector2D BestUnitAbsolutePosition = FVector2D::ZeroVector;
	float BestScore = TNumericLimits<float>::Max();
	for (const FUnitUI& Unit : Units)
	{
		if (Unit.mUnitId == INDEX_NONE || Unit.mHP <= 0.0f)
		{
			continue;
		}
		const FVector WorldLocation = Unit.mViewActor.IsValid()
			? Unit.mViewActor->GetActorLocation()
			: Unit.mWorldLocation;
		FVector2D WidgetPosition;
		if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PlayerController, WorldLocation, WidgetPosition, false) == false)
		{
			continue;
		}
		const FVector2D AbsolutePosition = RootGeometry.LocalToAbsolute(WidgetPosition);
		const FVector2D Delta = ScreenPosition - AbsolutePosition;
		// 투영점은 모델 발밑이다. 좌우는 넉넉히, 위쪽은 캐릭터 몸체 전체를 클릭 영역으로 잡는다.
		if (FMath::Abs(Delta.X) > 92.0f || Delta.Y < -165.0f || Delta.Y > 58.0f)
		{
			continue;
		}
		const float Score = FMath::Square(Delta.X) + FMath::Square(Delta.Y * 0.62f);
		if (Score < BestScore)
		{
			BestScore = Score;
			BestUnit = &Unit;
			BestUnitAbsolutePosition = AbsolutePosition;
		}
	}

	if (BestUnit == nullptr)
	{
		return false;
	}
	OutUnitId = BestUnit->mUnitId;
	OutIsPlayer = BestUnit->mIsPlayer;
	OutUnitScreenPosition = BestUnitAbsolutePosition;
	return true;
}

bool UCombatTileMapHUDWidget::TryOpenContextActionsAtScreenPosition(const FVector2D& ScreenPosition)
{
	if (mCombatControlsHidden || mCombatUIModel == nullptr || RootCanvas == nullptr
		|| mCombatUIModel->GetSelectedSkillIndex() != INDEX_NONE)
	{
		return false;
	}

	int32 UnitId = INDEX_NONE;
	bool bIsPlayer = false;
	FVector2D UnitScreenPosition;
	if (FindUnitAtScreenPosition(ScreenPosition, UnitId, bIsPlayer, UnitScreenPosition) == false)
	{
		CloseContextActions();
		return false;
	}

	mContextTargetUnitId = UnitId;
	mContextTargetIsPlayer = bIsPlayer;
	if (mDirectArmedTargetUnitId != UnitId)
	{
		mDirectArmedSkillIndex = INDEX_NONE;
		mDirectArmedTargetUnitId = INDEX_NONE;
	}
	// 실제 조준은 모델 몸체를 누른 임의 지점이 아니라 유닛 발밑(점유 타일 중심)의 절대좌표로 보낸다.
	// 그래야 큰 모델의 머리를 눌러도 뒤 타일로 빗나가지 않는다.
	mContextTargetScreenPosition = UnitScreenPosition;
	mContextSelectedSkillIndex = INDEX_NONE;
	mContextTargetSubmitted = false;
	RefreshContextActions();
	UpdateContextActionLayout();
	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
	UpdateEnemyIntentTutorial();
	return true;
}

void UCombatTileMapHUDWidget::RefreshContextActions()
{
	mContextActionSkillIndices.Reset();
	if (mContextActionTitleText != nullptr)
	{
		mContextActionTitleText->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (int32 SlotIndex = 0; SlotIndex < mContextActionButtons.Num(); ++SlotIndex)
	{
		if (mContextActionPanels.IsValidIndex(SlotIndex) && mContextActionPanels[SlotIndex] != nullptr)
		{
			mContextActionPanels[SlotIndex]->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (mContextActionIcons.IsValidIndex(SlotIndex) && mContextActionIcons[SlotIndex] != nullptr)
		{
			mContextActionIcons[SlotIndex]->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (mContextActionTexts.IsValidIndex(SlotIndex) && mContextActionTexts[SlotIndex] != nullptr)
		{
			mContextActionTexts[SlotIndex]->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (mContextActionButtons[SlotIndex] != nullptr)
		{
			mContextActionButtons[SlotIndex]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (mCombatControlsHidden || mCombatUIModel == nullptr || mContextTargetUnitId == INDEX_NONE)
	{
		return;
	}

	const TArray<FUnitUI>& Units = mCombatUIModel->GetUnitUIs();
	const FUnitUI* TargetUnit = Units.FindByPredicate([this](const FUnitUI& Unit)
	{
		return Unit.mUnitId == mContextTargetUnitId && Unit.mHP > 0.0f;
	});
	const FUnitUI* PlayerUnit = Units.FindByPredicate([](const FUnitUI& Unit)
	{
		return Unit.mIsPlayer && Unit.mHP > 0.0f;
	});
	if (TargetUnit == nullptr || PlayerUnit == nullptr)
	{
		CloseContextActions();
		return;
	}

	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	const TArray<FDiceSlotUI>& Dice = mCombatUIModel->GetDiceUIs();
	const int32 TileDistance = GetTileDistance(PlayerUnit->mTile, TargetUnit->mTile);
	auto CanReachTarget = [&Dice, TileDistance](const FSkillUI& Skill)
	{
		const int32 AvailableDiceSum = GetMaximumAvailableDiceSum(Dice, Skill.mDiceCost);
		if (AvailableDiceSum == INDEX_NONE)
		{
			return false;
		}
		const float MaximumRange = Skill.mTargeting.mSelectRange
			+ Skill.mTargeting.mSelectRangeRatio * StaticCast<float>(AvailableDiceSum);
		return TileDistance <= FMath::Max(1, FMath::CeilToInt(MaximumRange));
	};
	auto AddSkill = [this, &Skills](int32 SkillIndex)
	{
		if (mContextActionSkillIndices.Num() < ContextActionSlotCount
			&& Skills.IsValidIndex(SkillIndex)
			&& Skills[SkillIndex].mName.IsEmpty() == false)
		{
			mContextActionSkillIndices.Add(SkillIndex);
		}
	};

	if (TargetUnit->mIsPlayer)
	{
		mContextActionSkillIndices.Add(ContextMoveAction);
		for (int32 SkillIndex = 2; SkillIndex < Skills.Num(); ++SkillIndex)
		{
			const FSkillUI& Skill = Skills[SkillIndex];
			if (Skill.mIsUsable && Skill.mIsDisplacementSkill == false)
			{
				AddSkill(SkillIndex);
			}
		}
	}
	else
	{
		if (Skills.IsValidIndex(0) && Skills[0].mIsUsable && CanReachTarget(Skills[0]))
		{
			AddSkill(0);
		}
		for (int32 SkillIndex = 0; SkillIndex < Skills.Num(); ++SkillIndex)
		{
			const FSkillUI& Skill = Skills[SkillIndex];
			if (Skill.mIsUsable == false || Skill.mIsDisplacementSkill == false)
			{
				continue;
			}
			if ((Skill.mIsThrowSkill || Skill.mIsSwapSkill) ? TileDistance <= 1 : CanReachTarget(Skill))
			{
				AddSkill(SkillIndex);
			}
		}
	}

	FString TargetName = TargetUnit->mIsPlayer ? TEXT("먼저 이동을 선택하세요") : TEXT("먼저 행동을 선택하세요");
	if (TargetUnit->mIsPlayer == false)
	{
		const FEnemyIntentUI* Intent = mCombatUIModel->GetEnemyIntentUIs().FindByPredicate([TargetUnit](const FEnemyIntentUI& Item)
		{
			return Item.mEnemyUnitId == TargetUnit->mUnitId;
		});
		if (Intent != nullptr && Intent->mEnemyName.IsEmpty() == false)
		{
			TargetName = FString::Printf(TEXT("%s · 먼저 행동 선택"), *Intent->mEnemyName.ToString());
		}
	}
	if (mDirectArmedTargetUnitId == TargetUnit->mUnitId)
	{
		if (mDirectArmedSkillIndex == ContextMoveAction)
		{
			TargetName = TEXT("이동 선택됨 · 나를 빈 칸으로 드래그");
		}
		else if (Skills.IsValidIndex(mDirectArmedSkillIndex))
		{
			TargetName = FString::Printf(
				TEXT("%s 선택됨 · 이 적을 드래그"),
				*Skills[mDirectArmedSkillIndex].mName.ToString());
		}
	}
	if (mContextActionTitleText != nullptr)
	{
		mContextActionTitleText->SetText(FText::FromString(TargetName));
		mContextActionTitleText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	const int32 SelectedSkillIndex = mCombatUIModel->GetSelectedSkillIndex();
	const ECombatBuildPhaseUI BuildPhase = mCombatUIModel->GetTurnUI().mPhase;
	for (int32 SlotIndex = 0; SlotIndex < mContextActionSkillIndices.Num(); ++SlotIndex)
	{
		const int32 SkillIndex = mContextActionSkillIndices[SlotIndex];
		const FSkillUI* Skill = Skills.IsValidIndex(SkillIndex) ? &Skills[SkillIndex] : nullptr;
		const bool bSelected = (mDirectArmedTargetUnitId == TargetUnit->mUnitId
			&& mDirectArmedSkillIndex == SkillIndex)
			|| SkillIndex == SelectedSkillIndex;
		FString LabelText;
		if (SkillIndex == ContextMoveAction)
		{
			LabelText = bSelected ? TEXT("이동 ✓\n이제 드래그") : TEXT("이동\n먼저 선택");
		}
		else if (Skill != nullptr)
		{
			FString Role = TEXT("공격");
			if (Skill->mIsPullSkill)
			{
				Role = TEXT("내 쪽 드래그");
			}
			else if (Skill->mIsThrowSkill)
			{
				Role = TEXT("바깥 스와이프");
			}
			else if (Skill->mIsStaggerSkill)
			{
				Role = TEXT("옆 스와이프");
			}
			else if (Skill->mIsSwapSkill)
			{
				Role = TEXT("내 칸에 놓기");
			}
			else if (TargetUnit->mIsPlayer)
			{
				Role = TEXT("나에게 사용");
			}
			if (bSelected && mContextTargetSubmitted && BuildPhase == ECombatBuildPhaseUI::Preview
				&& Skill->mIsDisplacementSkill == false)
			{
				Role = TEXT("한 번 더 눌러 실행");
			}
			LabelText = bSelected
				? FString::Printf(TEXT("%s ✓\n이제 드래그"), *Skill->mName.ToString())
				: FString::Printf(TEXT("%s\n%s"), *Skill->mName.ToString(), *Role);
		}

		if (mContextActionPanels.IsValidIndex(SlotIndex) && mContextActionPanels[SlotIndex] != nullptr)
		{
			mContextActionPanels[SlotIndex]->SetBrushColor(bSelected
				? FLinearColor(0.42f, 0.30f, 0.035f, 0.97f)
				: FLinearColor(0.018f, 0.055f, 0.075f, 0.94f));
			mContextActionPanels[SlotIndex]->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (mContextActionIcons.IsValidIndex(SlotIndex) && mContextActionIcons[SlotIndex] != nullptr)
		{
			if (Skill != nullptr && Skill->mIcon != nullptr)
			{
				mContextActionIcons[SlotIndex]->SetBrushFromTexture(Skill->mIcon, false);
				mContextActionIcons[SlotIndex]->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				mContextActionIcons[SlotIndex]->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		if (mContextActionTexts.IsValidIndex(SlotIndex) && mContextActionTexts[SlotIndex] != nullptr)
		{
			mContextActionTexts[SlotIndex]->SetText(FText::FromString(LabelText));
			mContextActionTexts[SlotIndex]->SetColorAndOpacity(FSlateColor(bSelected
				? FLinearColor(1.0f, 0.88f, 0.30f, 1.0f)
				: FLinearColor(0.88f, 0.97f, 1.0f, 1.0f)));
			mContextActionTexts[SlotIndex]->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (mContextActionButtons.IsValidIndex(SlotIndex) && mContextActionButtons[SlotIndex] != nullptr)
		{
			mContextActionButtons[SlotIndex]->SetIsEnabled(true);
			mContextActionButtons[SlotIndex]->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void UCombatTileMapHUDWidget::UpdateContextActionLayout()
{
	if (mCombatControlsHidden || mCombatUIModel == nullptr || RootCanvas == nullptr
		|| mContextTargetUnitId == INDEX_NONE || mContextActionSkillIndices.Num() == 0)
	{
		return;
	}
	const FUnitUI* TargetUnit = mCombatUIModel->GetUnitUIs().FindByPredicate([this](const FUnitUI& Unit)
	{
		return Unit.mUnitId == mContextTargetUnitId && Unit.mHP > 0.0f;
	});
	APlayerController* PlayerController = GetOwningPlayer();
	if (TargetUnit == nullptr || PlayerController == nullptr)
	{
		CloseContextActions();
		return;
	}

	const FVector WorldLocation = TargetUnit->mViewActor.IsValid()
		? TargetUnit->mViewActor->GetActorLocation()
		: TargetUnit->mWorldLocation;
	FVector2D TargetPosition;
	if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		PlayerController, WorldLocation, TargetPosition, false) == false)
	{
		CloseContextActions();
		return;
	}

	const FVector2D RootSize = RootCanvas->GetCachedGeometry().GetLocalSize();
	const int32 ColumnCount = FMath::Min(mContextActionSkillIndices.Num(), 3);
	const int32 RowCount = FMath::DivideAndRoundUp(mContextActionSkillIndices.Num(), 3);
	const float TotalWidth = StaticCast<float>(ColumnCount) * ContextActionWidth
		+ StaticCast<float>(FMath::Max(ColumnCount - 1, 0)) * ContextActionGap;
	const float TotalHeight = StaticCast<float>(RowCount) * ContextActionHeight
		+ StaticCast<float>(FMath::Max(RowCount - 1, 0)) * ContextActionGap;
	float Left = TargetPosition.X - TotalWidth * 0.5f;
	float Top = TargetPosition.Y - TotalHeight - 92.0f;
	if (Top < 112.0f) { Top = TargetPosition.Y + 68.0f; }
	Left = FMath::Clamp(Left, 12.0f, FMath::Max(12.0f, RootSize.X - TotalWidth - 12.0f));
	Top = FMath::Clamp(Top, 112.0f, FMath::Max(112.0f, RootSize.Y - TotalHeight - 118.0f));

	if (mContextActionTitleText != nullptr)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(mContextActionTitleText->Slot))
		{
			CanvasSlot->SetAlignment(FVector2D::ZeroVector);
			CanvasSlot->SetPosition(FVector2D(Left, Top - 30.0f));
			CanvasSlot->SetSize(FVector2D(TotalWidth, 26.0f));
			CanvasSlot->SetZOrder(252);
		}
	}

	for (int32 SlotIndex = 0; SlotIndex < mContextActionSkillIndices.Num(); ++SlotIndex)
	{
		const int32 Column = SlotIndex % 3;
		const int32 Row = SlotIndex / 3;
		const FVector2D RowPosition(
			Left + StaticCast<float>(Column) * (ContextActionWidth + ContextActionGap),
			Top + StaticCast<float>(Row) * (ContextActionHeight + ContextActionGap));
		auto PlaceWidget = [RowPosition](UWidget* Widget, const FVector2D& Offset, const FVector2D& Size, int32 ZOrder)
		{
			if (Widget == nullptr) { return; }
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
			{
				CanvasSlot->SetAlignment(FVector2D::ZeroVector);
				CanvasSlot->SetPosition(RowPosition + Offset);
				CanvasSlot->SetSize(Size);
				CanvasSlot->SetZOrder(ZOrder);
			}
		};
		PlaceWidget(mContextActionPanels.IsValidIndex(SlotIndex) ? mContextActionPanels[SlotIndex].Get() : nullptr,
			FVector2D::ZeroVector, FVector2D(ContextActionWidth, ContextActionHeight), 250);
		PlaceWidget(mContextActionIcons.IsValidIndex(SlotIndex) ? mContextActionIcons[SlotIndex].Get() : nullptr,
			FVector2D(35.0f, 5.0f), FVector2D(34.0f, 34.0f), 251);
		PlaceWidget(mContextActionTexts.IsValidIndex(SlotIndex) ? mContextActionTexts[SlotIndex].Get() : nullptr,
			FVector2D(4.0f, 39.0f), FVector2D(ContextActionWidth - 8.0f, 28.0f), 251);
		PlaceWidget(mContextActionButtons.IsValidIndex(SlotIndex) ? mContextActionButtons[SlotIndex].Get() : nullptr,
			FVector2D::ZeroVector, FVector2D(ContextActionWidth, ContextActionHeight), 253);
	}
}

void UCombatTileMapHUDWidget::CloseContextActions()
{
	mContextTargetUnitId = INDEX_NONE;
	mContextTargetIsPlayer = false;
	mContextTargetScreenPosition = FVector2D::ZeroVector;
	mContextSelectedSkillIndex = INDEX_NONE;
	mContextTargetSubmitted = false;
	mContextActionSkillIndices.Reset();
	if (mContextActionTitleText != nullptr)
	{
		mContextActionTitleText->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (UBorder* Widget : mContextActionPanels) { if (Widget != nullptr) { Widget->SetVisibility(ESlateVisibility::Collapsed); } }
	for (UImage* Widget : mContextActionIcons) { if (Widget != nullptr) { Widget->SetVisibility(ESlateVisibility::Collapsed); } }
	for (UTextBlock* Widget : mContextActionTexts) { if (Widget != nullptr) { Widget->SetVisibility(ESlateVisibility::Collapsed); } }
	for (UIndexedButtonWidget* Widget : mContextActionButtons) { if (Widget != nullptr) { Widget->SetVisibility(ESlateVisibility::Collapsed); } }
}

void UCombatTileMapHUDWidget::HandleContextActionClicked(int32 ActionSlotIndex)
{
	if (mCombatUIModel == nullptr || mContextActionSkillIndices.IsValidIndex(ActionSlotIndex) == false)
	{
		return;
	}
	const int32 SkillIndex = mContextActionSkillIndices[ActionSlotIndex];
	if (SkillIndex == ContextMoveAction)
	{
		mDirectArmedSkillIndex = ContextMoveAction;
		mDirectArmedTargetUnitId = mContextTargetUnitId;
		RefreshContextActions();
		UpdateEnemyIntentTutorial();
		return;
	}

	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	if (Skills.IsValidIndex(SkillIndex) == false)
	{
		return;
	}
	if (Skills[SkillIndex].mIsDisplacementSkill)
	{
		mDirectArmedSkillIndex = SkillIndex;
		mDirectArmedTargetUnitId = mContextTargetUnitId;
		RefreshContextActions();
		UpdateEnemyIntentTutorial();
		return;
	}
	ExecuteDirectSkill(SkillIndex, mContextTargetScreenPosition, nullptr, 3);
	UpdateEnemyIntentTutorial();
}

void UCombatTileMapHUDWidget::TrySubmitContextTargetWhenReady()
{
	if (mCombatUIModel == nullptr || mContextTargetUnitId == INDEX_NONE || mContextTargetSubmitted)
	{
		return;
	}
	const int32 SelectedSkillIndex = mCombatUIModel->GetSelectedSkillIndex();
	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	if (SelectedSkillIndex == INDEX_NONE || SelectedSkillIndex != mContextSelectedSkillIndex
		|| Skills.IsValidIndex(SelectedSkillIndex) == false)
	{
		return;
	}
	const int32 RequiredDiceCount = FMath::Max(Skills[SelectedSkillIndex].mDiceCost, 0);
	if (mCombatUIModel->GetSelectedDiceIndices().Num() < RequiredDiceCount)
	{
		return;
	}

	mContextTargetSubmitted = true;
	mCombatUIModel->RequestWorldTouch(mContextTargetScreenPosition, false);
	RefreshContextActions();
}

int32 UCombatTileMapHUDWidget::FindDirectSkillIndex(bool FSkillUI::* SkillFlag) const
{
	if (mCombatUIModel == nullptr)
	{
		return INDEX_NONE;
	}
	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	for (int32 SkillIndex = 0; SkillIndex < Skills.Num(); ++SkillIndex)
	{
		if (Skills[SkillIndex].mIsUsable && Skills[SkillIndex].*SkillFlag)
		{
			return SkillIndex;
		}
	}
	return INDEX_NONE;
}

bool UCombatTileMapHUDWidget::ExecuteDirectSkill(
	int32 SkillIndex,
	const FVector2D& TargetScreenPosition,
	const FVector2D* DestinationScreenPosition,
	int32 DesiredPower)
{
	if (mCombatUIModel == nullptr || mCombatUIModel->GetSkillUIs().IsValidIndex(SkillIndex) == false)
	{
		return false;
	}
	const FSkillUI Skill = mCombatUIModel->GetSkillUIs()[SkillIndex];
	if (Skill.mIsUsable == false)
	{
		return false;
	}
	if (mCombatUIModel->GetSelectedSkillIndex() != INDEX_NONE)
	{
		mCombatUIModel->RequestCancel();
	}
	mCombatUIModel->RequestSelectSkill(SkillIndex);
	const TArray<int32> DiceIndices = PickAutomaticDice(
		mCombatUIModel->GetDiceUIs(),
		FMath::Max(Skill.mDiceCost, 0),
		FMath::Max(DesiredPower, 1) * FMath::Max(Skill.mDiceCost, 1));
	if (DiceIndices.Num() < Skill.mDiceCost)
	{
		mCombatUIModel->RequestCancel();
		return false;
	}
	for (int32 DiceIndex : DiceIndices)
	{
		mCombatUIModel->RequestToggleDice(DiceIndex);
	}
	mCombatUIModel->RequestWorldTouch(TargetScreenPosition, false);
	if (DestinationScreenPosition != nullptr)
	{
		// 손을 뗀 픽셀을 다시 레이캐스트하면 모델/장애물에 걸리거나 같은 타일로 판정될 수 있다.
		// 게임플레이가 이미 계산한 유효 8방향 후보를 화면에 투영해 드래그와 가장 가까운 방향으로 스냅한다.
		const FVector2D RequestedDirection = (*DestinationScreenPosition - TargetScreenPosition).GetSafeNormal();
		APlayerController* PlayerController = GetOwningPlayer();
		float BestDirectionScore = -2.0f;
		FVector2D BestCandidateScreen = FVector2D::ZeroVector;
		bool bFoundCandidate = false;
		if (PlayerController != nullptr && RootCanvas != nullptr)
		{
			for (const FVector& CandidateWorld : mCombatUIModel->GetDisplacementPreview().mDirectionCandidateWorldLocations)
			{
				FVector2D CandidateWidget;
				if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
					PlayerController, CandidateWorld, CandidateWidget, false) == false)
				{
					continue;
				}
				const FVector2D CandidateScreen = RootCanvas->GetCachedGeometry().LocalToAbsolute(CandidateWidget);
				const float DirectionScore = FVector2D::DotProduct(
					(CandidateScreen - TargetScreenPosition).GetSafeNormal(),
					RequestedDirection);
				if (DirectionScore > BestDirectionScore)
				{
					BestDirectionScore = DirectionScore;
					BestCandidateScreen = CandidateScreen;
					bFoundCandidate = true;
				}
			}
		}
		if (bFoundCandidate == false)
		{
			mCombatUIModel->RequestCancel();
			return false;
		}
		mCombatUIModel->RequestWorldTouch(BestCandidateScreen, false);
	}
	if (mCombatUIModel->GetTurnUI().mPhase != ECombatBuildPhaseUI::Preview)
	{
		mCombatUIModel->RequestCancel();
		return false;
	}
	mCombatUIModel->RequestConfirmSkill();
	CloseContextActions();
	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
	return true;
}

bool UCombatTileMapHUDWidget::BeginDirectUnitGesture(const FVector2D& ScreenPosition)
{
	if (mCombatUIModel == nullptr || mCombatUIModel->GetSelectedSkillIndex() != INDEX_NONE)
	{
		return false;
	}
	int32 UnitId = INDEX_NONE;
	bool bIsPlayer = false;
	FVector2D UnitScreenPosition;
	if (FindUnitAtScreenPosition(ScreenPosition, UnitId, bIsPlayer, UnitScreenPosition) == false)
	{
		return false;
	}
	mDirectUnitGestureActive = true;
	mDirectUnitGestureDragged = false;
	mDirectUnitGestureTargetId = UnitId;
	mDirectUnitGestureTargetIsPlayer = bIsPlayer;
	mDirectUnitGestureStart = ScreenPosition;
	mDirectUnitGestureCurrent = ScreenPosition;
	mDirectUnitGestureTargetScreen = UnitScreenPosition;
	CloseContextActions();
	SetDirectUnitGestureVisual(true, ScreenPosition);
	return true;
}

void UCombatTileMapHUDWidget::UpdateDirectUnitGesture(const FVector2D& ScreenPosition)
{
	if (mDirectUnitGestureActive == false)
	{
		return;
	}
	mDirectUnitGestureCurrent = ScreenPosition;
	mDirectUnitGestureDragged |= FVector2D::Distance(mDirectUnitGestureStart, ScreenPosition) >= DirectDragThreshold;
	SetDirectUnitGestureVisual(true, ScreenPosition);
}

void UCombatTileMapHUDWidget::SetDirectUnitGestureVisual(bool bVisible, const FVector2D& ScreenPosition)
{
	if (mDirectUnitGestureLine == nullptr || mDirectUnitGestureHandle == nullptr
		|| mDirectUnitGestureLabel == nullptr || RootCanvas == nullptr)
	{
		return;
	}
	if (bVisible == false)
	{
		mDirectUnitGestureLine->SetVisibility(ESlateVisibility::Collapsed);
		mDirectUnitGestureHandle->SetVisibility(ESlateVisibility::Collapsed);
		mDirectUnitGestureLabel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const bool bHasArmedAction = mDirectArmedTargetUnitId == mDirectUnitGestureTargetId
		&& (mDirectArmedSkillIndex == ContextMoveAction
			|| (mCombatUIModel != nullptr
				&& mCombatUIModel->GetSkillUIs().IsValidIndex(mDirectArmedSkillIndex)));
	if (bHasArmedAction == false)
	{
		// 행동을 고르기 전에는 손가락을 움직여도 드래그 선 자체를 보여주지 않는다.
		// 릴리스 시 팔레트만 열리므로, 방향 제스처가 행동을 몰래 추론한다는 오해가 생기지 않는다.
		mDirectUnitGestureLine->SetVisibility(ESlateVisibility::Collapsed);
		mDirectUnitGestureHandle->SetVisibility(ESlateVisibility::Collapsed);
		mDirectUnitGestureLabel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const FGeometry& RootGeometry = RootCanvas->GetCachedGeometry();
	const FVector2D Start = RootGeometry.AbsoluteToLocal(mDirectUnitGestureTargetScreen);
	const FVector2D End = RootGeometry.AbsoluteToLocal(ScreenPosition);
	const FVector2D Delta = End - Start;
	const float Length = Delta.Size();
	if (UCanvasPanelSlot* GestureLineSlot = Cast<UCanvasPanelSlot>(mDirectUnitGestureLine->Slot))
	{
		GestureLineSlot->SetPosition((Start + End) * 0.5f);
		GestureLineSlot->SetSize(FVector2D(FMath::Max(Length, 8.0f), 8.0f));
		GestureLineSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		GestureLineSlot->SetZOrder(960);
	}
	FWidgetTransform LineTransform;
	LineTransform.Angle = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
	mDirectUnitGestureLine->SetRenderTransform(LineTransform);
	if (UCanvasPanelSlot* GestureHandleSlot = Cast<UCanvasPanelSlot>(mDirectUnitGestureHandle->Slot))
	{
		GestureHandleSlot->SetPosition(End);
		GestureHandleSlot->SetSize(FVector2D(30.0f, 30.0f));
		GestureHandleSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		GestureHandleSlot->SetZOrder(961);
	}
	FString Label = TEXT("선택한 행동 · 놓아서 실행");
	if (mDirectArmedTargetUnitId == mDirectUnitGestureTargetId)
	{
		if (mDirectArmedSkillIndex == ContextMoveAction)
		{
			Label = TEXT("이동 · 빈 칸에서 놓아 실행");
		}
		else if (mCombatUIModel != nullptr
			&& mCombatUIModel->GetSkillUIs().IsValidIndex(mDirectArmedSkillIndex))
		{
			const FSkillUI& ArmedSkill = mCombatUIModel->GetSkillUIs()[mDirectArmedSkillIndex];
			if (ArmedSkill.mIsPullSkill) { Label = TEXT("끌어당기기 · 기사 쪽으로 드래그"); }
			else if (ArmedSkill.mIsThrowSkill) { Label = TEXT("밀기 · 던지기 · 원하는 방향으로 드래그"); }
			else if (ArmedSkill.mIsStaggerSkill) { Label = TEXT("다리 걸기 · 옆으로 드래그"); }
			else if (ArmedSkill.mIsSwapSkill) { Label = TEXT("자리 바꾸기 · 기사 위에 놓기"); }
			else { Label = ArmedSkill.mName.ToString() + TEXT(" · 놓아서 실행"); }
		}
	}
	mDirectUnitGestureLabel->SetText(FText::FromString(Label));
	if (UCanvasPanelSlot* GestureLabelSlot = Cast<UCanvasPanelSlot>(mDirectUnitGestureLabel->Slot))
	{
		GestureLabelSlot->SetPosition(End + FVector2D(0.0f, -48.0f));
		GestureLabelSlot->SetSize(FVector2D(220.0f, 34.0f));
		GestureLabelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		GestureLabelSlot->SetZOrder(962);
	}
	mDirectUnitGestureLine->SetVisibility(ESlateVisibility::HitTestInvisible);
	mDirectUnitGestureHandle->SetVisibility(ESlateVisibility::HitTestInvisible);
	mDirectUnitGestureLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
}

bool UCombatTileMapHUDWidget::EndDirectUnitGesture(const FVector2D& ScreenPosition)
{
	if (mDirectUnitGestureActive == false)
	{
		return false;
	}
	UpdateDirectUnitGesture(ScreenPosition);
	const int32 TargetId = mDirectUnitGestureTargetId;
	const bool bTargetIsPlayer = mDirectUnitGestureTargetIsPlayer;
	const FVector2D TargetScreen = mDirectUnitGestureTargetScreen;
	mDirectUnitGestureActive = false;
	mDirectUnitGestureTargetId = INDEX_NONE;
	SetDirectUnitGestureVisual(false);
	if (mDirectUnitGestureDragged == false)
	{
		return TryOpenContextActionsAtScreenPosition(TargetScreen);
	}
	if (mCombatUIModel == nullptr)
	{
		return true;
	}
	if (mDirectArmedTargetUnitId != TargetId || mDirectArmedSkillIndex == INDEX_NONE)
	{
		TryOpenContextActionsAtScreenPosition(TargetScreen);
		return true;
	}
	if (bTargetIsPlayer)
	{
		if (mDirectArmedSkillIndex != ContextMoveAction)
		{
			TryOpenContextActionsAtScreenPosition(TargetScreen);
			return true;
		}
		mCombatUIModel->RequestMove();
		mCombatUIModel->RequestWorldTouch(ScreenPosition, false);
		mCombatUIModel->RequestWorldTouch(ScreenPosition, false);
		mDirectArmedSkillIndex = INDEX_NONE;
		mDirectArmedTargetUnitId = INDEX_NONE;
		return true;
	}

	const int32 SkillIndex = mDirectArmedSkillIndex;
	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	if (Skills.IsValidIndex(SkillIndex) == false)
	{
		mDirectArmedSkillIndex = INDEX_NONE;
		mDirectArmedTargetUnitId = INDEX_NONE;
		TryOpenContextActionsAtScreenPosition(TargetScreen);
		return true;
	}
	const FVector2D* Destination = Skills[SkillIndex].mIsThrowSkill ? &ScreenPosition : nullptr;
	const float DragLength = FVector2D::Distance(ScreenPosition, TargetScreen);
	const int32 DesiredPower = FMath::Clamp(FMath::RoundToInt(DragLength / 72.0f), 1, 6);
	const bool bExecuted = ExecuteDirectSkill(SkillIndex, TargetScreen, Destination, DesiredPower);
	if (bExecuted)
	{
		if (Skills[SkillIndex].mIsPullSkill) { mEnemyIntentTutorialInterventionSubmitted = true; }
		if (Skills[SkillIndex].mIsThrowSkill) { mEnemyIntentTutorialThrowSubmitted = true; }
		mDirectArmedSkillIndex = INDEX_NONE;
		mDirectArmedTargetUnitId = INDEX_NONE;
	}
	else
	{
		TryOpenContextActionsAtScreenPosition(TargetScreen);
		RefreshContextActions();
	}
	UpdateEnemyIntentTutorial();
	return true;
}

UWidget* UCombatTileMapHUDWidget::FindContextActionButtonForSkill(int32 SkillIndex) const
{
	for (int32 SlotIndex = 0; SlotIndex < mContextActionSkillIndices.Num(); ++SlotIndex)
	{
		if (mContextActionSkillIndices[SlotIndex] == SkillIndex
			&& mContextActionButtons.IsValidIndex(SlotIndex)
			&& mContextActionButtons[SlotIndex] != nullptr)
		{
			return mContextActionButtons[SlotIndex].Get();
		}
	}
	return nullptr;
}
