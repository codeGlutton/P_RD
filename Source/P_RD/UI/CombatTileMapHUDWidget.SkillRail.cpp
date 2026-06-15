#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"
#include "UI/IndexedButtonWidget.h"

using namespace RDCombatHUD;

void UCombatTileMapHUDWidget::RebuildSkillRailWidgets()
{
	if (RootCanvas == nullptr || WidgetTree == nullptr || mSkillRailPanels.Num() == CombatSkillSlotCount)
	{
		return;
	}

	for (UBorder* SkillRailPanel : mSkillRailPanels)
	{
		if (SkillRailPanel != nullptr)
		{
			SkillRailPanel->RemoveFromParent();
		}
	}
	mSkillRailPanels.Reset();
	mSkillRailTexts.Reset();

	for (int32 SkillIndex = 0; SkillIndex < CombatSkillSlotCount; ++SkillIndex)
	{
		UBorder* SkillRailPanel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			FName(*FString::Printf(TEXT("RuntimeSkillRailPanel_%d"), SkillIndex))
		);
		UTextBlock* SkillRailText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			FName(*FString::Printf(TEXT("RuntimeSkillRailText_%d"), SkillIndex))
		);
		if (SkillRailPanel == nullptr || SkillRailText == nullptr)
		{
			continue;
		}

		SkillRailPanel->SetBrushColor(GetCombatSkillRailBrushColor(false));
		SkillRailPanel->SetPadding(GetCombatSkillRailPadding());
		SkillRailText->SetJustification(ETextJustify::Center);
		SkillRailText->SetColorAndOpacity(FSlateColor(GetCombatSkillRailTextColor(false)));
		SkillRailText->SetText(GetCombatSkillRailLabel(SkillIndex));
		SkillRailPanel->AddChild(SkillRailText);
		RootCanvas->AddChildToCanvas(SkillRailPanel);

		mSkillRailPanels.Add(SkillRailPanel);
		mSkillRailTexts.Add(SkillRailText);
	}

	RefreshSkillRailWidgets();
}

void UCombatTileMapHUDWidget::RefreshSkillRailWidgets()
{
	for (int32 SkillIndex = 0; SkillIndex < mSkillRailPanels.Num(); ++SkillIndex)
	{
		const bool bSelected = SkillIndex == mSelectedSkillIndex;
		if (UBorder* SkillRailPanel = mSkillRailPanels[SkillIndex])
		{
			SkillRailPanel->SetBrushColor(GetCombatSkillRailBrushColor(bSelected));
			SkillRailPanel->SetRenderScale(GetCombatSkillRailScale(bSelected));
		}

		if (mSkillRailTexts.IsValidIndex(SkillIndex))
		{
			if (UTextBlock* SkillRailText = mSkillRailTexts[SkillIndex])
			{
				SkillRailText->SetColorAndOpacity(FSlateColor(GetCombatSkillRailTextColor(bSelected)));
			}
		}
	}
}

void UCombatTileMapHUDWidget::EnsureSkillInputButtons()
{
	if (RootCanvas == nullptr || WidgetTree == nullptr || mSkillInputButtons.Num() == CombatSkillSlotCount)
	{
		return;
	}

	for (UIndexedButtonWidget* SkillInputButton : mSkillInputButtons)
	{
		if (SkillInputButton != nullptr)
		{
			SkillInputButton->RemoveFromParent();
		}
	}
	mSkillInputButtons.Reset();

	for (int32 SkillIndex = 0; SkillIndex < CombatSkillSlotCount; ++SkillIndex)
	{
		UIndexedButtonWidget* SkillInputButton = WidgetTree->ConstructWidget<UIndexedButtonWidget>(
			UIndexedButtonWidget::StaticClass(),
			FName(*FString::Printf(TEXT("SkillInputButton_%d"), SkillIndex))
		);
		if (SkillInputButton == nullptr)
		{
			continue;
		}

		SkillInputButton->SetBackgroundColor(GetTransparentInputButtonColor());
		SkillInputButton->SetVisibility(ESlateVisibility::Visible);
		SkillInputButton->SetButtonIndex(SkillIndex);
		SkillInputButton->OnIndexedPressed.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleSkillButtonPressed);
		SkillInputButton->OnReleased.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleSkillButtonReleased);
		RootCanvas->AddChildToCanvas(SkillInputButton);
		mSkillInputButtons.Add(SkillInputButton);
	}
}

int32 UCombatTileMapHUDWidget::FindSkillRailIndexAtScreenPosition(const FVector2D& ScreenPosition) const
{
	const FGeometry CachedGeometry = GetCachedGeometry();
	const FVector2D LocalPosition = CachedGeometry.AbsoluteToLocal(ScreenPosition);
	const FVector2D LocalSize = CachedGeometry.GetLocalSize();
	if (LocalSize.X <= 0.0f || LocalSize.Y <= 0.0f)
	{
		return INDEX_NONE;
	}

	const float NormalizedX = LocalPosition.X / LocalSize.X;
	const float NormalizedY = LocalPosition.Y / LocalSize.Y;
	if (NormalizedX < CombatSkillRailLeft || NormalizedX > CombatSkillRailRight)
	{
		return INDEX_NONE;
	}

	for (int32 SkillIndex = 0; SkillIndex < CombatSkillSlotCount; ++SkillIndex)
	{
		const float SlotTop = CombatSkillRailTop + StaticCast<float>(SkillIndex) * (CombatSkillRailHeight + CombatSkillRailGap);
		const float SlotBottom = SlotTop + CombatSkillRailHeight;
		if (NormalizedY >= SlotTop && NormalizedY <= SlotBottom)
		{
			return SkillIndex;
		}
	}

	return INDEX_NONE;
}
