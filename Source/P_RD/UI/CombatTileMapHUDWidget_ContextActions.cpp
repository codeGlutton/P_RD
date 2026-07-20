#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Actor/TileMap/TileMap.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CapsuleComponent.h"
#include "Components/Image.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextBlock.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Pawn/Unit.h"
#include "Pawn/Enemy/EnemyUnitModel.h"
#include "RDCollision.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
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
	// 적을 눌렀을 때 주변에 별도의 "사용 가능 스킬" 팔레트를 띄우지 않는다.
	// 적 대상 행동은 좌측 스킬 레일 선택 -> 적 드래그 한 가지 입력 흐름으로만 제공한다.
	if (bIsPlayer == false)
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
	const int32 TileDistance = GetTileDistance(PlayerUnit->mTile, TargetUnit->mTile);
	auto CanReachTarget = [TileDistance](const FSkillUI& Skill)
	{
		const int32 ActionPower = Skill.mIsDisplacementSkill ? 6 : 3;
		const float MaximumRange = Skill.mTargeting.mSelectRange
			+ Skill.mTargeting.mSelectRangeRatio * StaticCast<float>(ActionPower);
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
				Role = TEXT("기사 주변 칸으로 드래그");
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
		const int32 TargetUnitId = mContextTargetUnitId;
		const FVector2D TargetScreen = mContextTargetScreenPosition;
		if (PrepareDirectSkill(SkillIndex, TargetScreen, 6) == false)
		{
			return;
		}
		mDirectArmedSkillIndex = SkillIndex;
		mDirectArmedTargetUnitId = TargetUnitId;
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

bool UCombatTileMapHUDWidget::SelectSkillWithActionPower(int32 SkillIndex, int32 DesiredPower)
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
	if (mCombatUIModel->GetSelectedSkillIndex() != SkillIndex)
	{
		if (mCombatUIModel->GetSelectedSkillIndex() != INDEX_NONE)
		{
			mCombatUIModel->RequestCancel();
		}
		mCombatUIModel->RequestSelectSkillWithPower(SkillIndex, DesiredPower);
	}
	return mCombatUIModel->GetSelectedSkillIndex() == SkillIndex;
}

bool UCombatTileMapHUDWidget::PrepareDirectSkill(
	int32 SkillIndex,
	const FVector2D& TargetScreenPosition,
	int32 DesiredPower)
{
	if (SelectSkillWithActionPower(SkillIndex, DesiredPower) == false)
	{
		return false;
	}
	mCombatUIModel->RequestWorldTouch(TargetScreenPosition, false);
	const ECombatBuildPhaseUI Phase = mCombatUIModel->GetTurnUI().mPhase;
	return Phase == ECombatBuildPhaseUI::Preview
		|| Phase == ECombatBuildPhaseUI::ThrowDestinationSelection;
}

bool UCombatTileMapHUDWidget::SelectDirectThrowDestination(
	const FVector2D& TargetScreenPosition,
	const FVector2D& DestinationScreenPosition)
{
	if (mCombatUIModel == nullptr || RootCanvas == nullptr)
	{
		return false;
	}
	const FVector2D RequestedDirection = (DestinationScreenPosition - TargetScreenPosition).GetSafeNormal();
	if (RequestedDirection.IsNearlyZero())
	{
		return false;
	}
	APlayerController* PlayerController = GetOwningPlayer();
	float BestCandidateScore = -BIG_NUMBER;
	FVector2D BestCandidateScreen = FVector2D::ZeroVector;
	FVector BestCandidateWorld = FVector::ZeroVector;
	bool bFoundCandidate = false;
	if (PlayerController != nullptr)
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
			const float TouchDistance = FVector2D::Distance(CandidateScreen, DestinationScreenPosition);
			const float ProximityScore = 1.0f - FMath::Clamp(TouchDistance / 520.0f, 0.0f, 1.0f);
			const float CandidateScore = DirectionScore * 0.45f + ProximityScore * 0.55f;
			if (CandidateScore > BestCandidateScore)
			{
				BestCandidateScore = CandidateScore;
				BestCandidateScreen = CandidateScreen;
				BestCandidateWorld = CandidateWorld;
				bFoundCandidate = true;
			}
		}
	}
	if (bFoundCandidate == false)
	{
		return false;
	}
	if (mHasDirectThrowCandidate
		&& mDirectThrowCandidateWorld.Equals(BestCandidateWorld, 1.0f)
		&& mCombatUIModel->GetTurnUI().mPhase == ECombatBuildPhaseUI::Preview)
	{
		return true;
	}
	mCombatUIModel->RequestWorldTouch(BestCandidateScreen, false);
	if (mCombatUIModel->GetTurnUI().mPhase != ECombatBuildPhaseUI::Preview)
	{
		return false;
	}
	mDirectThrowCandidateWorld = BestCandidateWorld;
	mHasDirectThrowCandidate = true;
	return true;
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
	// 드래그 중에는 적과 착지 칸이 이미 게임플레이 Preview에 확정되어 있다. 손을 놓을 때
	// 같은 좌표를 다시 WorldTouch로 보내면 Preview 입력으로 재해석되어 실행이 유실될 수 있으므로,
	// 현재 보이는 밝은 착지 칸을 그대로 확정한다.
	const bool bPreparedDisplacement = DestinationScreenPosition != nullptr
		&& mCombatUIModel->GetSelectedSkillIndex() == SkillIndex
		&& mCombatUIModel->GetTurnUI().mPhase == ECombatBuildPhaseUI::Preview
		&& mHasDirectThrowCandidate;
	if (bPreparedDisplacement == false)
	{
		if (SelectSkillWithActionPower(SkillIndex, DesiredPower) == false)
		{
			return false;
		}
		mCombatUIModel->RequestWorldTouch(TargetScreenPosition, false);
		if (DestinationScreenPosition != nullptr
			&& SelectDirectThrowDestination(TargetScreenPosition, *DestinationScreenPosition) == false)
		{
			mCombatUIModel->RequestCancel();
			return false;
		}
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

bool UCombatTileMapHUDWidget::TryExecuteTapSkillAtScreenPosition(const FVector2D& ScreenPosition)
{
	if (mCombatUIModel == nullptr)
	{
		return false;
	}
	const int32 SelectedSkillIndex = mCombatUIModel->GetSelectedSkillIndex();
	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	if (Skills.IsValidIndex(SelectedSkillIndex) == false)
	{
		return false;
	}
	const FSkillUI& Skill = Skills[SelectedSkillIndex];
	const bool bExplicitTapAction = mSelectedSubactionMode == ECombatSubactionMode::BasicAttack
		|| mSelectedSubactionMode == ECombatSubactionMode::Stagger
		|| mSelectedSubactionMode == ECombatSubactionMode::Swap;
	const bool bLegacyTapAction = mSelectedSubactionMode == ECombatSubactionMode::None
		&& (SelectedSkillIndex == 0 || Skill.mIsStaggerSkill);
	// 공격/제압/자리교환은 적 한 번 탭, 손아귀와 밀치기는 방향 드래그, 기동은 빈 타일 탭이다.
	if (bExplicitTapAction == false && bLegacyTapAction == false)
	{
		return false;
	}

	int32 UnitId = INDEX_NONE;
	bool bIsPlayer = false;
	FVector2D UnitScreenPosition = FVector2D::ZeroVector;
	if (FindUnitAtScreenPosition(ScreenPosition, UnitId, bIsPlayer, UnitScreenPosition) == false
		|| bIsPlayer)
	{
		return false;
	}
	const FUnitUI* TargetUnit = mCombatUIModel->GetUnitUIs().FindByPredicate([UnitId](const FUnitUI& Unit)
	{
		return Unit.mUnitId == UnitId;
	});
	const FUnitUI* PlayerUnit = mCombatUIModel->GetUnitUIs().FindByPredicate([](const FUnitUI& Unit)
	{
		return Unit.mIsPlayer && Unit.mHP > 0.0f;
	});
	const int32 SelectRange = FMath::Max(FMath::CeilToInt(Skill.mTargeting.mSelectRange), 1);
	if (TargetUnit == nullptr || PlayerUnit == nullptr
		|| GetTileDistance(TargetUnit->mTile, PlayerUnit->mTile) > SelectRange)
	{
		// 범위 밖 적 탭은 선택을 취소하지 않고 현재 사거리 표시를 그대로 유지한다.
		return true;
	}
	// 유효한 적을 눌렀다면 이 입력은 여기서 소유한다. 실패 좌표를 월드 입력으로 한 번 더 보내
	// 프리뷰/취소 상태가 이중 전환되는 일을 막는다.
	ExecuteDirectSkill(
		SelectedSkillIndex,
		UnitScreenPosition,
		nullptr,
		FMath::Max(mSelectedSubactionDesiredPower, 1));
	return true;
}

bool UCombatTileMapHUDWidget::TryExecuteTapTileActionAtScreenPosition(const FVector2D& ScreenPosition)
{
	const bool bMovementAction = mSelectedSubactionMode == ECombatSubactionMode::Move
		|| mSelectedSubactionMode == ECombatSubactionMode::Charge;
	if (mCombatUIModel == nullptr || bMovementAction == false
		|| mCombatUIModel->GetSelectedSkillIndex() == INDEX_NONE)
	{
		return false;
	}
	int32 UnitId = INDEX_NONE;
	bool bIsPlayer = false;
	FVector2D UnitScreenPosition = FVector2D::ZeroVector;
	if (FindUnitAtScreenPosition(ScreenPosition, UnitId, bIsPlayer, UnitScreenPosition) && bIsPlayer)
	{
		// 기사 몸에서 시작한 포인터만 아래 직접 드래그 경로가 캡처한다.
		return false;
	}
	// 빈 타일 탭은 기존 스킬 조준으로 새지 않게 소비하고, 기사를 직접 잡으라는 사거리 상태를 유지한다.
	return true;
}

void UCombatTileMapHUDWidget::RefreshDirectMoveRangeHighlight()
{
	if (mCombatUIModel == nullptr
		|| (mSelectedSubactionMode != ECombatSubactionMode::Move
			&& mSelectedSubactionMode != ECombatSubactionMode::Charge
			&& mSelectedSubactionMode != ECombatSubactionMode::Leap))
	{
		return;
	}
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	UUnitModel* PlayerUnit = CombatModel != nullptr ? CombatModel->GetPlayerUnit() : nullptr;
	if (TileMap == nullptr || PlayerUnit == nullptr)
	{
		return;
	}
	const int32 Range = mSelectedSubactionMode == ECombatSubactionMode::Leap ? 3 : 1;
	const FTileIndex Origin = PlayerUnit->GetTileTransform().mIndex;
	TArray<FTileIndex> RangeTiles;
	if (mSelectedSubactionMode == ECombatSubactionMode::Leap)
	{
		for (int32 Y = Origin.mY - Range; Y <= Origin.mY + Range; ++Y)
		{
			for (int32 X = Origin.mX - Range; X <= Origin.mX + Range; ++X)
			{
				const FTileIndex Candidate(X, Y);
				if (Candidate != Origin && TileMap->IsValidIndex(Candidate)
					&& TileMap->CanPlace(Candidate, PlayerUnit))
				{
					RangeTiles.Add(Candidate);
				}
			}
		}
	}
	else
	{
		RangeTiles = mSelectedSubactionMode == ECombatSubactionMode::Charge
			? TileMap->GetAimableTiles(Origin, Range, EAimPattern::Star, true, true, PlayerUnit)
			: TileMap->GetReachableTiles(Origin, Range);
	}
	TileMap->ClearTileHighlight(ETileHighlightFlag::Aim | ETileHighlightFlag::Select | ETileHighlightFlag::Effect);
	TileMap->ClearMovePath();
	TileMap->SetTileHighlight(RangeTiles, ETileHighlightFlag::Aim);
}

void UCombatTileMapHUDWidget::ClearDirectMovePreview(bool bClearAim)
{
	if (USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this))
	{
		if (UTileMapModel* TileMap = CombatModel->GetTileMap())
		{
			TileMap->ClearMovePath();
			TileMap->ClearTileHighlight(ETileHighlightFlag::Select | ETileHighlightFlag::Effect);
			if (bClearAim)
			{
				TileMap->ClearTileHighlight(ETileHighlightFlag::Aim);
			}
		}
	}
	mDirectMovePath.Reset();
	mDirectMoveLandingTile = FTileIndex::Invalid;
	mDirectMoveImpactTile = FTileIndex::Invalid;
	mDirectMoveImpactLabel = FText::GetEmpty();
}

bool UCombatTileMapHUDWidget::UpdateDirectMovePath(const FVector2D& ScreenPosition)
{
	if (mCombatUIModel == nullptr || mDirectUnitGestureTargetIsPlayer == false)
	{
		return false;
	}
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	UUnitModel* PlayerUnit = CombatModel != nullptr ? CombatModel->GetPlayerUnit() : nullptr;
	FVector TouchWorld = FVector::ZeroVector;
	FTileIndex TouchTile = FTileIndex::Invalid;
	if (TileMap == nullptr || PlayerUnit == nullptr
		|| GetDirectGestureFloorWorldLocation(ScreenPosition, TouchWorld, &TouchTile) == false)
	{
		return false;
	}
	const FTileIndex Origin = PlayerUnit->GetTileTransform().mIndex;
	const int32 Range = mDirectMoveIsLeap ? 3 : 1;
	TArray<FTileIndex> NewPath;
	mDirectMoveImpactTile = FTileIndex::Invalid;
	mDirectMoveImpactLabel = FText::GetEmpty();
	ESRPGDisplacementWeight ImpactWeight = ESRPGDisplacementWeight::Invalid;
	bool bImpactStopsBeforeTile = false;

	if (mDirectMoveIsLeap)
	{
		const int32 DeltaX = FMath::Abs(TouchTile.mX - Origin.mX);
		const int32 DeltaY = FMath::Abs(TouchTile.mY - Origin.mY);
		if ((DeltaX > 0 || DeltaY > 0) && DeltaX <= Range && DeltaY <= Range
			&& TileMap->CanPlace(TouchTile, PlayerUnit))
		{
			NewPath = { Origin, TouchTile };
			mDirectMoveImpactLabel = NSLOCTEXT("WarriorMove", "LeapLandingPreview", "도약 착지 · 놓아서 실행");
		}
	}
	else if (mDirectMoveIsCharge == false)
	{
		NewPath = TileMap->FindPath(Origin, TouchTile);
		if (NewPath.Num() > Range + 1)
		{
			NewPath.SetNum(Range + 1);
		}
	}
	else
	{
		NewPath.Add(Origin);
		const int32 RawDeltaX = TouchTile.mX - Origin.mX;
		const int32 RawDeltaY = TouchTile.mY - Origin.mY;
		const int32 AbsX = FMath::Abs(RawDeltaX);
		const int32 AbsY = FMath::Abs(RawDeltaY);
		int32 StepX = FMath::Sign(RawDeltaX);
		int32 StepY = FMath::Sign(RawDeltaY);
		if (AbsX > AbsY * 2) { StepY = 0; }
		else if (AbsY > AbsX * 2) { StepX = 0; }
		const int32 DesiredSteps = FMath::Min(FMath::Max(AbsX, AbsY), Range);
		for (int32 StepIndex = 1; StepIndex <= DesiredSteps; ++StepIndex)
		{
			const FTileIndex Candidate(Origin.mX + StepX * StepIndex, Origin.mY + StepY * StepIndex);
			if (TileMap->IsValidIndex(Candidate) == false)
			{
				break;
			}
			NewPath.Add(Candidate);
			UBoardActorModel* Blocker = nullptr;
			for (UBoardActorModel* Actor : TileMap->GetActorsOnTile(Candidate, ETileLayerFlag::All))
			{
				if (Actor != nullptr && Actor != PlayerUnit
					&& EnumHasAnyFlags(Actor->GetBlockLayerFlags(), PlayerUnit->GetTileLayerFlags()))
				{
					Blocker = Actor;
					break;
				}
			}
			if (UEnemyUnitModel* Enemy = Cast<UEnemyUnitModel>(Blocker))
			{
				mDirectMoveImpactTile = Candidate;
				ImpactWeight = Enemy->GetDisplacementWeight();
				switch (ImpactWeight)
				{
				case ESRPGDisplacementWeight::Light:
					mDirectMoveImpactLabel = NSLOCTEXT("WarriorMove", "LightChargePreview", "경량 관통 · 밀면서 계속 전진");
					break;
				case ESRPGDisplacementWeight::Medium:
					mDirectMoveImpactLabel = Range >= 4
						? NSLOCTEXT("WarriorMove", "MediumChargePreview", "중량 충돌 · 1칸 밀고 여기서 정지")
						: NSLOCTEXT("WarriorMove", "MediumChargeWeakPreview", "힘 부족 · 정면 충돌 후 정지");
					bImpactStopsBeforeTile = Range < 4;
					break;
				case ESRPGDisplacementWeight::Heavy:
				default:
					mDirectMoveImpactLabel = NSLOCTEXT("WarriorMove", "HeavyChargePreview", "초중량 충돌 · 반동 피해 후 정지");
					bImpactStopsBeforeTile = true;
					break;
				}

				const int32 PreviewPushDistance = ImpactWeight == ESRPGDisplacementWeight::Light
					? FMath::Clamp(1 + Range / 4, 1, 2)
					: (ImpactWeight == ESRPGDisplacementWeight::Medium && Range >= 4 ? 1 : 0);
				FTileIndex PushCursor = Candidate;
				int32 PreviewMovedDistance = 0;
				FTileIndex PreviewCollisionTile = FTileIndex::Invalid;
				UBoardActorModel* PreviewCollisionActor = nullptr;
				for (int32 PushIndex = 0; PushIndex < PreviewPushDistance; ++PushIndex)
				{
					const FTileIndex PushCandidate(PushCursor.mX + StepX, PushCursor.mY + StepY);
					if (TileMap->IsValidIndex(PushCandidate) == false
						|| TileMap->CanPlace(PushCandidate, Enemy) == false)
					{
						PreviewCollisionTile = PushCandidate;
						for (UBoardActorModel* PushActor : TileMap->GetActorsOnTile(PushCandidate, ETileLayerFlag::All))
						{
							if (PushActor != nullptr && PushActor != Enemy
								&& EnumHasAnyFlags(PushActor->GetBlockLayerFlags(), Enemy->GetTileLayerFlags()))
							{
								PreviewCollisionActor = PushActor;
								break;
							}
						}
						break;
					}
					++PreviewMovedDistance;
					PushCursor = PushCandidate;
				}
				if (PreviewPushDistance > 0 && PreviewMovedDistance == 0)
				{
					mDirectMoveImpactTile = TileMap->IsValidIndex(PreviewCollisionTile)
						? PreviewCollisionTile : Candidate;
					mDirectMoveImpactLabel = Cast<UEnemyUnitModel>(PreviewCollisionActor) != nullptr
						? NSLOCTEXT("WarriorMove", "ChargeChainCollisionPreview", "연쇄 충돌 · 두 적 피해 · 직전 칸 정지")
						: NSLOCTEXT("WarriorMove", "ChargePinnedPreview", "뒤가 막힘 · 충돌 피해 · 직전 칸 정지");
					bImpactStopsBeforeTile = true;
					break;
				}
				if (ImpactWeight != ESRPGDisplacementWeight::Light)
				{
					break;
				}
			}
			else if (Blocker != nullptr || TileMap->CanPlace(Candidate, PlayerUnit) == false)
			{
				mDirectMoveImpactTile = Candidate;
				mDirectMoveImpactLabel = NSLOCTEXT("WarriorMove", "ObstacleChargePreview", "장애물 충돌 · 피해를 받고 직전 칸 정지");
				bImpactStopsBeforeTile = true;
				break;
			}
		}
	}

	if (NewPath.IsEmpty())
	{
		return false;
	}
	mDirectMovePath = MoveTemp(NewPath);
	mDirectMoveLandingTile = mDirectMovePath.Last();
	if (bImpactStopsBeforeTile && mDirectMovePath.Num() >= 2)
	{
		mDirectMoveLandingTile = mDirectMovePath[mDirectMovePath.Num() - 2];
	}

	TileMap->ClearMovePath();
	TileMap->ClearTileHighlight(ETileHighlightFlag::Select | ETileHighlightFlag::Effect);
	if (mDirectMovePath.Num() > 1)
	{
		TArray<FTileIndex> SelectedPath = mDirectMovePath;
		SelectedPath.RemoveAt(0);
		TileMap->SetMovePath(mDirectMovePath);
		TileMap->SetTileHighlight(SelectedPath, ETileHighlightFlag::Select);
	}
	if (mDirectMoveImpactTile != FTileIndex::Invalid)
	{
		TileMap->SetTileHighlight({ mDirectMoveImpactTile }, ETileHighlightFlag::Effect);
	}
	return mDirectMovePath.Num() > 1;
}

bool UCombatTileMapHUDWidget::BeginDirectUnitGesture(const FVector2D& ScreenPosition)
{
	if (mCombatUIModel == nullptr)
	{
		return false;
	}
	const int32 SelectedSkillIndex = mCombatUIModel->GetSelectedSkillIndex();
	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	const bool bExplicitDragAction = mSelectedSubactionMode == ECombatSubactionMode::Pull
		|| mSelectedSubactionMode == ECombatSubactionMode::Throw
		|| mSelectedSubactionMode == ECombatSubactionMode::ShortThrow;
	const bool bMovementAction = mSelectedSubactionMode == ECombatSubactionMode::Move
		|| mSelectedSubactionMode == ECombatSubactionMode::Charge
		|| mSelectedSubactionMode == ECombatSubactionMode::Leap;
	if (bMovementAction && SelectedSkillIndex == INDEX_NONE)
	{
		return false;
	}
	const bool bSelectedDragAction = Skills.IsValidIndex(SelectedSkillIndex)
		&& (bExplicitDragAction
			|| (mSelectedSubactionMode == ECombatSubactionMode::None
				&& (Skills[SelectedSkillIndex].mIsPullSkill || Skills[SelectedSkillIndex].mIsThrowSkill)));
	if (SelectedSkillIndex != INDEX_NONE && bSelectedDragAction == false && bMovementAction == false)
	{
		// 기본 공격/방해는 한 번 탭, 손아귀와 기동은 아래 직접 드래그 입력이 담당한다.
		return false;
	}
	int32 UnitId = INDEX_NONE;
	bool bIsPlayer = false;
	FVector2D UnitScreenPosition;
	if (FindUnitAtScreenPosition(ScreenPosition, UnitId, bIsPlayer, UnitScreenPosition) == false)
	{
		return false;
	}
	mDirectGripGesture = false;
	mDirectGripCanSwap = false;
	mDirectGripSwapPreview = false;
	if (bMovementAction)
	{
		if (bIsPlayer == false)
		{
			return false;
		}
		mDirectArmedSkillIndex = ContextMoveAction;
		mDirectArmedTargetUnitId = UnitId;
		mDirectMoveIsCharge = mSelectedSubactionMode == ECombatSubactionMode::Charge;
		mDirectMoveIsLeap = mSelectedSubactionMode == ECombatSubactionMode::Leap;
		const FUnitUI* DraggedPlayer = mCombatUIModel->GetUnitUIs().FindByPredicate([UnitId](const FUnitUI& Unit)
		{
			return Unit.mUnitId == UnitId;
		});
		if (DraggedPlayer == nullptr || DraggedPlayer->mTile == FTileIndex::Invalid)
		{
			mDirectArmedSkillIndex = INDEX_NONE;
			mDirectArmedTargetUnitId = INDEX_NONE;
			return false;
		}
		mDirectMovePath = { DraggedPlayer->mTile };
		mDirectMoveLandingTile = mDirectMovePath[0];
		mDirectMoveImpactTile = FTileIndex::Invalid;
		mDirectMoveImpactLabel = FText::GetEmpty();
	}
	if (bSelectedDragAction)
	{
		if (bIsPlayer)
		{
			return false;
		}
		int32 ArmedSkillIndex = SelectedSkillIndex;
		// 구형 통합 손아귀 선택에만 거리 기반 자동 전환을 남긴다. 행동군 플라이아웃에서
		// 끌기/던지기를 명시적으로 고른 경우에는 선택한 세부 행동을 절대 바꾸지 않는다.
		const bool bStartedWithGrip = Skills[SelectedSkillIndex].mIsPullSkill
			&& mSelectedSubactionMode == ECombatSubactionMode::None;
		if (bStartedWithGrip)
		{
			const FUnitUI* TargetUnit = mCombatUIModel->GetUnitUIs().FindByPredicate([UnitId](const FUnitUI& Unit)
			{
				return Unit.mUnitId == UnitId;
			});
			const FUnitUI* PlayerUnit = mCombatUIModel->GetUnitUIs().FindByPredicate([](const FUnitUI& Unit)
			{
				return Unit.mIsPlayer && Unit.mHP > 0.0f;
			});
			const bool bAdjacent = TargetUnit != nullptr && PlayerUnit != nullptr
				&& TargetUnit->mTile != FTileIndex::Invalid
				&& PlayerUnit->mTile != FTileIndex::Invalid
				&& GetTileDistance(TargetUnit->mTile, PlayerUnit->mTile) == 1;
			if (bAdjacent)
			{
				// 같은 손아귀 카드라도 이미 가까운 적을 잡으면 투척 조준으로 자연스럽게 전환한다.
				const int32 ThrowSkillIndex = FindDirectSkillIndex(&FSkillUI::mIsThrowSkill);
				if (Skills.IsValidIndex(ThrowSkillIndex))
				{
					if (SelectSkillWithActionPower(ThrowSkillIndex, 6))
					{
						ArmedSkillIndex = ThrowSkillIndex;
					}
					else
					{
						SelectSkillWithActionPower(SelectedSkillIndex, 6);
					}
				}

				const int32 SwapSkillIndex = FindDirectSkillIndex(&FSkillUI::mIsSwapSkill);
				mDirectGripCanSwap = Skills.IsValidIndex(SwapSkillIndex);
			}
			mDirectGripGesture = true;
		}
		mDirectArmedSkillIndex = ArmedSkillIndex;
		mDirectArmedTargetUnitId = UnitId;
		mCombatUIModel->RequestWorldTouch(UnitScreenPosition, false);
		const ECombatBuildPhaseUI Phase = mCombatUIModel->GetTurnUI().mPhase;
		if (Phase != ECombatBuildPhaseUI::Preview
			&& Phase != ECombatBuildPhaseUI::ThrowDestinationSelection)
		{
			mDirectArmedSkillIndex = INDEX_NONE;
			mDirectArmedTargetUnitId = INDEX_NONE;
			return false;
		}
	}
	else if (bIsPlayer == false)
	{
		// 행동을 고르지 않은 적 탭/드래그는 아무 팔레트도 열지 않는다. 좌측 레일에서
		// 스킬을 먼저 고른 경우에만 아래의 직접 조작 상태로 진입한다.
		CloseContextActions();
		return false;
	}
	mDirectUnitGestureActive = true;
	mDirectUnitGestureDragged = false;
	mDirectUnitGestureTargetId = UnitId;
	mDirectUnitGestureTargetIsPlayer = bIsPlayer;
	mDirectUnitGestureStart = ScreenPosition;
	mDirectUnitGestureCurrent = ScreenPosition;
	mDirectUnitGestureTargetScreen = UnitScreenPosition;
	mHasDirectThrowCandidate = false;
	mDirectThrowCandidateWorld = FVector::ZeroVector;
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
	FVector SwapFloorWorld = FVector::ZeroVector;
	mDirectGripSwapPreview = mDirectUnitGestureDragged
		&& IsDirectGripSwapDestination(ScreenPosition, SwapFloorWorld);
	if (mDirectUnitGestureDragged && mDirectUnitGestureTargetIsPlayer
		&& mDirectArmedSkillIndex == ContextMoveAction)
	{
		UpdateDirectMovePath(ScreenPosition);
	}
	if (mDirectUnitGestureDragged
		&& mDirectGripSwapPreview == false
		&& mCombatUIModel != nullptr
		&& mCombatUIModel->GetSkillUIs().IsValidIndex(mDirectArmedSkillIndex)
		&& (mCombatUIModel->GetSkillUIs()[mDirectArmedSkillIndex].mIsPullSkill
			|| mCombatUIModel->GetSkillUIs()[mDirectArmedSkillIndex].mIsThrowSkill))
	{
		SelectDirectThrowDestination(mDirectUnitGestureTargetScreen, ScreenPosition);
	}
	SetDirectUnitGestureVisual(true, ScreenPosition);
}

bool UCombatTileMapHUDWidget::GetDirectGestureFloorWorldLocation(
	const FVector2D& ScreenPosition,
	FVector& OutWorldLocation,
	FTileIndex* OutTileIndex) const
{
	OutWorldLocation = FVector::ZeroVector;
	if (OutTileIndex != nullptr)
	{
		*OutTileIndex = FTileIndex::Invalid;
	}
	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController == nullptr)
	{
		return false;
	}
	FVector2D ViewportPixel = FVector2D::ZeroVector;
	FVector2D ViewportPosition = FVector2D::ZeroVector;
	USlateBlueprintLibrary::AbsoluteToViewport(
		this, ScreenPosition, ViewportPixel, ViewportPosition);
	FHitResult HitResult;
	if (PlayerController->GetHitResultAtScreenPosition(
		ViewportPixel, RDTraceChannels::TileOnlyTrace, false, HitResult) == false)
	{
		return false;
	}
	ATileMap* TileMap = Cast<ATileMap>(HitResult.GetActor());
	if (TileMap == nullptr)
	{
		return false;
	}
	const FTileIndex TileIndex = TileMap->WorldToTileIndex(HitResult.ImpactPoint);
	if (TileMap->IsValidIndex(TileIndex) == false)
	{
		return false;
	}
	// 스냅된 착지칸은 게임플레이 프리뷰가 따로 보여준다. 이 고스트는 손가락을 그대로 따라간다.
	OutWorldLocation = HitResult.ImpactPoint;
	if (OutTileIndex != nullptr)
	{
		*OutTileIndex = TileIndex;
	}
	return true;
}

bool UCombatTileMapHUDWidget::IsDirectGripSwapDestination(
	const FVector2D& ScreenPosition,
	FVector& OutPlayerFloorWorld) const
{
	OutPlayerFloorWorld = FVector::ZeroVector;
	if (mDirectGripGesture == false || mDirectGripCanSwap == false || mCombatUIModel == nullptr)
	{
		return false;
	}
	const FUnitUI* PlayerUnit = mCombatUIModel->GetUnitUIs().FindByPredicate([](const FUnitUI& Unit)
	{
		return Unit.mIsPlayer && Unit.mHP > 0.0f;
	});
	if (PlayerUnit == nullptr || PlayerUnit->mTile == FTileIndex::Invalid)
	{
		return false;
	}
	FVector TouchWorld = FVector::ZeroVector;
	FTileIndex TouchTile = FTileIndex::Invalid;
	AUnit* PlayerView = PlayerUnit->mViewActor.IsValid()
		? Cast<AUnit>(PlayerUnit->mViewActor.Get())
		: nullptr;
	const bool bOnPlayerTile = GetDirectGestureFloorWorldLocation(ScreenPosition, TouchWorld, &TouchTile)
		&& TouchTile == PlayerUnit->mTile;
	bool bOnPlayerBody = false;
	if (PlayerView != nullptr && RootCanvas != nullptr)
	{
		FVector2D PlayerWidgetPosition = FVector2D::ZeroVector;
		if (APlayerController* PlayerController = GetOwningPlayer();
			PlayerController != nullptr
			&& UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
				PlayerController, PlayerView->GetActorLocation(), PlayerWidgetPosition, false))
		{
			const FVector2D PlayerScreenPosition = RootCanvas->GetCachedGeometry().LocalToAbsolute(PlayerWidgetPosition);
			const FVector2D Delta = ScreenPosition - PlayerScreenPosition;
			// 손가락이 기사 모델을 가려도 몸체 전체를 자리 교환 드롭 존으로 인정하고 발밑 칸에 스냅한다.
			bOnPlayerBody = FMath::Abs(Delta.X) <= 92.0f && Delta.Y >= -165.0f && Delta.Y <= 58.0f;
		}
	}
	if (bOnPlayerTile == false && bOnPlayerBody == false)
	{
		return false;
	}
	const float RootHeight = PlayerView != nullptr && PlayerView->GetCapsuleComponent() != nullptr
		? PlayerView->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
		: 0.0f;
	OutPlayerFloorWorld = PlayerView != nullptr
		? PlayerView->GetActorLocation() - FVector(0.0f, 0.0f, RootHeight)
		: TouchWorld;
	return true;
}

void UCombatTileMapHUDWidget::EnsureDirectUnitGestureGhost(int32 TargetUnitId)
{
	if (IsValid(mDirectUnitGestureGhostActor)
		&& mDirectGestureGhostTargetId == TargetUnitId)
	{
		return;
	}
	DestroyDirectUnitGestureGhost();
	if (mCombatUIModel == nullptr || GetWorld() == nullptr)
	{
		return;
	}
	const FUnitUI* TargetUnit = mCombatUIModel->GetUnitUIs().FindByPredicate([TargetUnitId](const FUnitUI& Unit)
	{
		return Unit.mUnitId == TargetUnitId && Unit.mViewActor.IsValid();
	});
	AUnit* SourceUnit = TargetUnit != nullptr ? Cast<AUnit>(TargetUnit->mViewActor.Get()) : nullptr;
	USkeletalMeshComponent* SourceMesh = SourceUnit != nullptr ? SourceUnit->GetMesh() : nullptr;
	if (SourceUnit == nullptr || SourceMesh == nullptr || SourceMesh->GetSkeletalMeshAsset() == nullptr)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	mDirectUnitGestureGhostActor = GetWorld()->SpawnActor<AActor>(
		AActor::StaticClass(), SourceUnit->GetActorTransform(), SpawnParameters);
	if (mDirectUnitGestureGhostActor == nullptr)
	{
		return;
	}
	USceneComponent* GhostRoot = NewObject<USceneComponent>(
		mDirectUnitGestureGhostActor, TEXT("DirectDragGhostRoot"), RF_Transient);
	mDirectUnitGestureGhostMesh = NewObject<USkeletalMeshComponent>(
		mDirectUnitGestureGhostActor, TEXT("DirectDragGhostMesh"), RF_Transient);
	if (GhostRoot == nullptr || mDirectUnitGestureGhostMesh == nullptr)
	{
		DestroyDirectUnitGestureGhost();
		return;
	}
	mDirectUnitGestureGhostActor->SetRootComponent(GhostRoot);
	mDirectUnitGestureGhostActor->AddInstanceComponent(GhostRoot);
	GhostRoot->RegisterComponent();
	mDirectUnitGestureGhostMesh->SetupAttachment(GhostRoot);
	mDirectUnitGestureGhostActor->AddInstanceComponent(mDirectUnitGestureGhostMesh);
	mDirectUnitGestureGhostMesh->SetSkeletalMeshAsset(SourceMesh->GetSkeletalMeshAsset());
	mDirectUnitGestureGhostMesh->SetRelativeTransform(SourceMesh->GetRelativeTransform());
	mDirectUnitGestureGhostMesh->SetLeaderPoseComponent(SourceMesh);
	mDirectUnitGestureGhostMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	mDirectUnitGestureGhostMesh->SetGenerateOverlapEvents(false);
	mDirectUnitGestureGhostMesh->SetCastShadow(false);
	mDirectUnitGestureGhostMesh->SetReceivesDecals(false);
	mDirectUnitGestureGhostMesh->SetTranslucentSortPriority(40);
	if (mDirectUnitGestureGhostMaterial != nullptr)
	{
		const int32 MaterialCount = FMath::Max(SourceMesh->GetNumMaterials(), 1);
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			mDirectUnitGestureGhostMesh->SetMaterial(MaterialIndex, mDirectUnitGestureGhostMaterial);
		}
	}
	mDirectUnitGestureGhostMesh->RegisterComponent();
	mDirectUnitGestureGhostActor->SetActorEnableCollision(false);
	mDirectUnitGestureGhostActor->SetActorTickEnabled(false);
	mDirectGestureGhostTargetId = TargetUnitId;
}

void UCombatTileMapHUDWidget::DestroyDirectUnitGestureGhost()
{
	if (IsValid(mDirectUnitGestureGhostActor))
	{
		mDirectUnitGestureGhostActor->Destroy();
	}
	mDirectUnitGestureGhostActor = nullptr;
	mDirectUnitGestureGhostMesh = nullptr;
	mDirectGestureGhostTargetId = INDEX_NONE;
}

void UCombatTileMapHUDWidget::SetDirectUnitGestureVisual(bool bVisible, const FVector2D& ScreenPosition)
{
	// 기존 선/손잡이는 더 이상 사용하지 않는다. 대상의 실제 메시 고스트가 착지 타일을 직접 보여준다.
	if (mDirectUnitGestureLine != nullptr) { mDirectUnitGestureLine->SetVisibility(ESlateVisibility::Collapsed); }
	if (mDirectUnitGestureHandle != nullptr) { mDirectUnitGestureHandle->SetVisibility(ESlateVisibility::Collapsed); }
	if (bVisible == false)
	{
		if (mDirectUnitGestureLabel != nullptr) { mDirectUnitGestureLabel->SetVisibility(ESlateVisibility::Collapsed); }
		DestroyDirectUnitGestureGhost();
		return;
	}
	if (mDirectUnitGestureLabel == nullptr || RootCanvas == nullptr || mCombatUIModel == nullptr)
	{
		return;
	}
	const bool bHasArmedAction = mDirectArmedTargetUnitId == mDirectUnitGestureTargetId
		&& (mDirectArmedSkillIndex == ContextMoveAction
			|| mCombatUIModel->GetSkillUIs().IsValidIndex(mDirectArmedSkillIndex));
	if (bHasArmedAction == false)
	{
		mDirectUnitGestureLabel->SetVisibility(ESlateVisibility::Collapsed);
		DestroyDirectUnitGestureGhost();
		return;
	}

	FVector SnappedLandingWorld = FVector::ZeroVector;
	bool bHasSnappedLanding = false;
	if (mDirectArmedSkillIndex == ContextMoveAction
		&& mDirectMoveLandingTile != FTileIndex::Invalid)
	{
		if (USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this))
		{
			if (UTileMapModel* TileMapModel = CombatModel->GetTileMap())
			{
				SnappedLandingWorld = TileMapModel->TileToWorldLocation(mDirectMoveLandingTile);
				bHasSnappedLanding = true;
			}
		}
	}
	const FDisplacementPreviewUI& Preview = mCombatUIModel->GetDisplacementPreview();
	if (bHasSnappedLanding == false && Preview.mIsActive
		&& Preview.mTargetUnitId == mDirectUnitGestureTargetId
		&& Preview.mLandingTile != FTileIndex::Invalid)
	{
		SnappedLandingWorld = Preview.mLandingWorldLocation;
		bHasSnappedLanding = true;
	}
	FVector PlayerSwapWorld = FVector::ZeroVector;
	if (mDirectGripSwapPreview && IsDirectGripSwapDestination(ScreenPosition, PlayerSwapWorld))
	{
		SnappedLandingWorld = PlayerSwapWorld;
		bHasSnappedLanding = true;
	}

	FVector TouchFloorWorld = FVector::ZeroVector;
	const bool bHasTouchFloor = GetDirectGestureFloorWorldLocation(ScreenPosition, TouchFloorWorld);
	// 유효한 착지 후보가 정해진 순간부터 3D 적을 실제 판정 타일 중심에 붙인다.
	// 아직 후보를 못 고른 짧은 구간에만 손가락을 따라가므로, 놓기 전에 최종 배치를 정확히 볼 수 있다.
	FVector GhostFloorWorld = FVector::ZeroVector;
	bool bHasGhostFloor = false;
	if (bHasSnappedLanding)
	{
		GhostFloorWorld = SnappedLandingWorld;
		bHasGhostFloor = true;
	}
	else if (mDirectUnitGestureDragged && bHasTouchFloor)
	{
		GhostFloorWorld = TouchFloorWorld;
		bHasGhostFloor = true;
	}
	else if (bHasTouchFloor)
	{
		GhostFloorWorld = TouchFloorWorld;
		bHasGhostFloor = true;
	}
	if (bHasGhostFloor == false)
	{
		mDirectUnitGestureLabel->SetVisibility(ESlateVisibility::Collapsed);
		DestroyDirectUnitGestureGhost();
		return;
	}

	EnsureDirectUnitGestureGhost(mDirectUnitGestureTargetId);
	const FUnitUI* TargetUnit = mCombatUIModel->GetUnitUIs().FindByPredicate([this](const FUnitUI& Unit)
	{
		return Unit.mUnitId == mDirectUnitGestureTargetId && Unit.mViewActor.IsValid();
	});
	AUnit* SourceUnit = TargetUnit != nullptr ? Cast<AUnit>(TargetUnit->mViewActor.Get()) : nullptr;
	if (mDirectUnitGestureGhostActor == nullptr || SourceUnit == nullptr)
	{
		mDirectUnitGestureLabel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const float RootHeight = SourceUnit->GetCapsuleComponent() != nullptr
		? SourceUnit->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
		: 0.0f;
	const FVector GhostActorLocation = GhostFloorWorld + FVector(0.0f, 0.0f, RootHeight);
	mDirectUnitGestureGhostActor->SetActorLocationAndRotation(
		GhostActorLocation, SourceUnit->GetActorRotation());

	FString Label = bHasSnappedLanding
		? TEXT("이 타일에 배치 · 놓아서 실행")
		: TEXT("원하는 칸 쪽으로 드래그");
	if (mDirectArmedSkillIndex == ContextMoveAction)
	{
		if (mDirectMoveIsLeap)
		{
			Label = mDirectMovePath.Num() > 1
				? TEXT("도약 착지 · 놓아서 실행")
				: TEXT("밝은 타일로 드래그 · 사이 장애물은 무시");
		}
		else if (mDirectMoveIsCharge)
		{
			Label = mDirectMoveImpactLabel.IsEmpty()
				? TEXT("어깨 돌진 · 직선으로 끌고 놓기")
				: mDirectMoveImpactLabel.ToString();
		}
		else
		{
			Label = mDirectMovePath.Num() > 1
				? FString::Printf(TEXT("전투 스텝 %d칸 · 놓아서 실행"), mDirectMovePath.Num() - 1)
				: TEXT("기사를 잡고 밝은 범위 안으로 드래그");
		}
	}
	else if (mCombatUIModel->GetSkillUIs().IsValidIndex(mDirectArmedSkillIndex))
	{
		const FSkillUI& ArmedSkill = mCombatUIModel->GetSkillUIs()[mDirectArmedSkillIndex];
		if (mDirectGripSwapPreview)
		{
			Label = TEXT("자리 바꾸기 · 기사 칸에 놓기");
		}
		else if (ArmedSkill.mIsPullSkill)
		{
			const FString ActionName = mSelectedSubactionName.IsEmpty()
				? TEXT("당기기") : mSelectedSubactionName.ToString();
			Label = bHasSnappedLanding
				? FString::Printf(TEXT("%s %d칸 · 놓아서 실행"), *ActionName, FMath::Max(Preview.mMoveDistance, 1))
				: TEXT("기사 주변의 원하는 칸으로 드래그");
		}
		else if (ArmedSkill.mIsThrowSkill)
		{
			if (Preview.mCollisionTile != FTileIndex::Invalid)
			{
				const bool bHitsUnit = mCombatUIModel->GetUnitUIs().ContainsByPredicate([this, &Preview](const FUnitUI& Unit)
				{
					return Unit.mUnitId != mDirectUnitGestureTargetId
						&& Unit.mHP > 0.0f
						&& Unit.mTile == Preview.mCollisionTile;
				});
				Label = bHitsUnit
					? FString::Printf(TEXT("%s와 충돌 · 둘 다 피해"), *Preview.mCollisionName.ToString())
					: TEXT("장애물 충돌 · 벽 찍기");
			}
			else
			{
				const FString ActionName = mSelectedSubactionName.IsEmpty()
					? TEXT("던지기") : mSelectedSubactionName.ToString();
				Label = bHasSnappedLanding
					? FString::Printf(TEXT("%s %d칸 · 놓아서 실행"), *ActionName, FMath::Max(Preview.mMoveDistance, 1))
					: TEXT("던질 방향으로 드래그");
			}
		}
		else
		{
			Label = bHasSnappedLanding
				? FString::Printf(TEXT("%s · 밝은 타일에 놓기"), *ArmedSkill.mName.ToString())
				: FString::Printf(TEXT("%s · 드래그"), *ArmedSkill.mName.ToString());
		}
	}
	mDirectUnitGestureLabel->SetText(FText::FromString(Label));
	FVector2D GhostWidgetPosition;
	if (APlayerController* PlayerController = GetOwningPlayer();
		PlayerController != nullptr
		&& UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PlayerController, GhostActorLocation, GhostWidgetPosition, false))
	{
		if (UCanvasPanelSlot* GestureLabelSlot = Cast<UCanvasPanelSlot>(mDirectUnitGestureLabel->Slot))
		{
			GestureLabelSlot->SetPosition(GhostWidgetPosition + FVector2D(0.0f, -104.0f));
			GestureLabelSlot->SetSize(FVector2D(
				mDirectArmedSkillIndex == ContextMoveAction ? 340.0f : 240.0f,
				34.0f));
			GestureLabelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			GestureLabelSlot->SetZOrder(962);
		}
		mDirectUnitGestureLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
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
	const bool bGripSwap = mDirectGripSwapPreview;
	const bool bWasGripGesture = mDirectGripGesture;
	const bool bWasExplicitDrag = mSelectedSubactionMode == ECombatSubactionMode::Pull
		|| mSelectedSubactionMode == ECombatSubactionMode::Throw
		|| mSelectedSubactionMode == ECombatSubactionMode::ShortThrow;
	const int32 PreviousArmedSkillIndex = mDirectArmedSkillIndex;
	const bool bWasMovementGesture = bTargetIsPlayer && PreviousArmedSkillIndex == ContextMoveAction;
	const TArray<FTileIndex> WarriorMovePath = mDirectMovePath;
	const bool bWarriorCharge = mDirectMoveIsCharge;
	const bool bWarriorLeap = mDirectMoveIsLeap;
	const int32 WarriorMoveActionPower = FMath::Clamp(mSelectedSubactionDesiredPower, 1, 6);
	mDirectUnitGestureActive = false;
	mDirectUnitGestureTargetId = INDEX_NONE;
	SetDirectUnitGestureVisual(false);
	mDirectGripGesture = false;
	mDirectGripCanSwap = false;
	mDirectGripSwapPreview = false;
	if (mDirectUnitGestureDragged == false)
	{
		if (bWasMovementGesture)
		{
			ClearDirectMovePreview(false);
			RefreshDirectMoveRangeHighlight();
			mDirectArmedSkillIndex = INDEX_NONE;
			mDirectArmedTargetUnitId = INDEX_NONE;
			return true;
		}
		if (bWasExplicitDrag && mCombatUIModel != nullptr)
		{
			mCombatUIModel->RequestCancel();
			SelectSkillWithActionPower(
				PreviousArmedSkillIndex,
				FMath::Max(mSelectedSubactionDesiredPower, 1));
			mDirectArmedSkillIndex = INDEX_NONE;
			mDirectArmedTargetUnitId = INDEX_NONE;
			return true;
		}
		// 손아귀는 드래그 전용이다. 짧은 탭으로 내부 당기기/던지기 프리뷰가 열린 채 남으면
		// 다음 공격 판정까지 오염되므로 취소한 뒤 대표 손아귀 카드의 사거리 상태만 복구한다.
		if (bWasGripGesture && mCombatUIModel != nullptr)
		{
			mCombatUIModel->RequestCancel();
			const int32 GripSkillIndex = FindDirectSkillIndex(&FSkillUI::mIsPullSkill);
			if (GripSkillIndex != INDEX_NONE)
			{
				SelectSkillWithActionPower(GripSkillIndex, 6);
			}
			mDirectArmedSkillIndex = INDEX_NONE;
			mDirectArmedTargetUnitId = INDEX_NONE;
			return true;
		}
		if (mCombatUIModel != nullptr && mCombatUIModel->GetSelectedSkillIndex() != INDEX_NONE)
		{
			return true;
		}
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
		if (WarriorMovePath.Num() >= 2 && WarriorMoveActionPower > 0)
		{
			FWarriorMoveRequest Request;
			Request.mPathTileIndexes = WarriorMovePath;
			Request.mIsCharge = bWarriorCharge;
			Request.mIsLeap = bWarriorLeap;
			Request.mActionPower = WarriorMoveActionPower;
			ClearDirectMovePreview(false);
			mCombatUIModel->RequestWarriorMove(Request);
		}
		else
		{
			ClearDirectMovePreview(false);
			RefreshDirectMoveRangeHighlight();
		}
		mDirectArmedSkillIndex = INDEX_NONE;
		mDirectArmedTargetUnitId = INDEX_NONE;
		return true;
	}

	const int32 SkillIndex = bGripSwap
		? FindDirectSkillIndex(&FSkillUI::mIsSwapSkill)
		: mDirectArmedSkillIndex;
	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	if (Skills.IsValidIndex(SkillIndex) == false)
	{
		mDirectArmedSkillIndex = INDEX_NONE;
		mDirectArmedTargetUnitId = INDEX_NONE;
		TryOpenContextActionsAtScreenPosition(TargetScreen);
		return true;
	}
	const bool bIsPullSkill = Skills[SkillIndex].mIsPullSkill;
	const bool bIsThrowSkill = Skills[SkillIndex].mIsThrowSkill;
	const FVector2D* Destination = (bIsPullSkill || bIsThrowSkill)
		? &ScreenPosition
		: nullptr;
	const int32 DesiredPower = FMath::Clamp(mSelectedSubactionDesiredPower, 1, 6);
	const bool bExecuted = ExecuteDirectSkill(SkillIndex, TargetScreen, Destination, DesiredPower);
	if (bExecuted)
	{
		if (bIsPullSkill) { mEnemyIntentTutorialInterventionSubmitted = true; }
		if (bIsThrowSkill || bGripSwap) { mEnemyIntentTutorialThrowSubmitted = true; }
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
