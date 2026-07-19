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
	constexpr int32 ContextActionSlotCount = 4;
	constexpr int32 ContextMoveAction = -2;
	constexpr float ContextActionWidth = 204.0f;
	constexpr float ContextActionHeight = 58.0f;
	constexpr float ContextActionGap = 7.0f;

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
		Label->SetJustification(ETextJustify::Left);
		Label->SetAutoWrapText(false);
		Label->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
		Label->SetLineHeightPercentage(0.86f);
		Label->SetVisibility(ESlateVisibility::Collapsed);
		FSlateFontInfo LabelFont = Label->GetFont();
		LabelFont.Size = 15;
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

bool UCombatTileMapHUDWidget::TryOpenContextActionsAtScreenPosition(const FVector2D& ScreenPosition)
{
	if (mCombatControlsHidden || mCombatUIModel == nullptr || RootCanvas == nullptr
		|| mCombatUIModel->GetSelectedSkillIndex() != INDEX_NONE)
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
		CloseContextActions();
		return false;
	}

	mContextTargetUnitId = BestUnit->mUnitId;
	mContextTargetIsPlayer = BestUnit->mIsPlayer;
	// 실제 조준은 모델 몸체를 누른 임의 지점이 아니라 유닛 발밑(점유 타일 중심)의 절대좌표로 보낸다.
	// 그래야 큰 모델의 머리를 눌러도 뒤 타일로 빗나가지 않는다.
	mContextTargetScreenPosition = BestUnitAbsolutePosition;
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
			if (Skill.mIsThrowSkill ? TileDistance <= 1 : CanReachTarget(Skill))
			{
				AddSkill(SkillIndex);
			}
		}
	}

	FString TargetName = TargetUnit->mIsPlayer ? TEXT("내 행동") : TEXT("이 적에게 할 행동");
	if (TargetUnit->mIsPlayer == false)
	{
		const FEnemyIntentUI* Intent = mCombatUIModel->GetEnemyIntentUIs().FindByPredicate([TargetUnit](const FEnemyIntentUI& Item)
		{
			return Item.mEnemyUnitId == TargetUnit->mUnitId;
		});
		if (Intent != nullptr && Intent->mEnemyName.IsEmpty() == false)
		{
			TargetName = FString::Printf(TEXT("%s에게 할 행동"), *Intent->mEnemyName.ToString());
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
		const bool bSelected = SkillIndex == SelectedSkillIndex;
		FString LabelText;
		if (SkillIndex == ContextMoveAction)
		{
			LabelText = TEXT("이동\n빈 칸을 골라 이동");
		}
		else if (Skill != nullptr)
		{
			FString Role = TEXT("공격");
			if (Skill->mIsPullSkill)
			{
				Role = TEXT("적을 내 앞까지 당김");
			}
			else if (Skill->mIsThrowSkill)
			{
				Role = TEXT("방향을 골라 충돌시킴");
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
			const FString DiceCost = Skill->mDiceCost > 0
				? FString::Printf(TEXT("주사위 %d · "), Skill->mDiceCost)
				: TEXT("");
			LabelText = FString::Printf(TEXT("%s\n%s%s"), *Skill->mName.ToString(), *DiceCost, *Role);
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
			const bool bPreparedPreview = mContextTargetSubmitted && SelectedSkillIndex != INDEX_NONE;
			const bool bCanExecutePreparedSkill = bSelected && Skill != nullptr
				&& Skill->mIsDisplacementSkill == false
				&& BuildPhase == ECombatBuildPhaseUI::Preview;
			mContextActionButtons[SlotIndex]->SetIsEnabled(bPreparedPreview == false || bCanExecutePreparedSkill);
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
	const float TotalHeight = StaticCast<float>(mContextActionSkillIndices.Num()) * ContextActionHeight
		+ StaticCast<float>(FMath::Max(mContextActionSkillIndices.Num() - 1, 0)) * ContextActionGap;
	const bool bPlaceRight = TargetPosition.X + ContextActionWidth + 96.0f < RootSize.X * 0.74f;
	float Left = bPlaceRight ? TargetPosition.X + 82.0f : TargetPosition.X - ContextActionWidth - 82.0f;
	float Top = TargetPosition.Y - TotalHeight * 0.62f;
	Left = FMath::Clamp(Left, 12.0f, FMath::Max(12.0f, RootSize.X - ContextActionWidth - 12.0f));
	Top = FMath::Clamp(Top, 112.0f, FMath::Max(112.0f, RootSize.Y - TotalHeight - 118.0f));

	if (mContextActionTitleText != nullptr)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(mContextActionTitleText->Slot))
		{
			CanvasSlot->SetAlignment(FVector2D::ZeroVector);
			CanvasSlot->SetPosition(FVector2D(Left, Top - 30.0f));
			CanvasSlot->SetSize(FVector2D(ContextActionWidth, 26.0f));
			CanvasSlot->SetZOrder(252);
		}
	}

	for (int32 SlotIndex = 0; SlotIndex < mContextActionSkillIndices.Num(); ++SlotIndex)
	{
		const FVector2D RowPosition(Left, Top + StaticCast<float>(SlotIndex) * (ContextActionHeight + ContextActionGap));
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
			FVector2D(7.0f, 7.0f), FVector2D(44.0f, 44.0f), 251);
		PlaceWidget(mContextActionTexts.IsValidIndex(SlotIndex) ? mContextActionTexts[SlotIndex].Get() : nullptr,
			FVector2D(58.0f, 6.0f), FVector2D(ContextActionWidth - 64.0f, ContextActionHeight - 10.0f), 251);
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
		CloseContextActions();
		mCombatUIModel->RequestMove();
		return;
	}

	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	if (Skills.IsValidIndex(SkillIndex) == false)
	{
		return;
	}
	const bool bExecutePreparedSkill = mContextSelectedSkillIndex == SkillIndex
		&& mContextTargetSubmitted
		&& mCombatUIModel->GetSelectedSkillIndex() == SkillIndex
		&& mCombatUIModel->GetTurnUI().mPhase == ECombatBuildPhaseUI::Preview
		&& Skills[SkillIndex].mIsDisplacementSkill == false;
	if (bExecutePreparedSkill)
	{
		mCombatUIModel->RequestWorldTouch(mContextTargetScreenPosition, false);
		return;
	}

	mContextSelectedSkillIndex = SkillIndex;
	mContextTargetSubmitted = false;
	SelectSkillForAssignment(SkillIndex);
	RefreshContextActions();
	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
	TrySubmitContextTargetWhenReady();
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
