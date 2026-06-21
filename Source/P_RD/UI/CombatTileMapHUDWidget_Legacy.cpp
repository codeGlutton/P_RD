#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"

using namespace RDCombatHUD;

void UCombatTileMapHUDWidget::HideLegacyDiceSlots() const
{
	UWidget* LegacyDiceWidgets[] = {
		DiceFaceSize_0.Get(),
		DiceFaceSize_1.Get(),
		DiceFaceSize_2.Get(),
		DiceFaceSize_3.Get(),
		DiceFaceFill_0.Get(),
		DiceFaceFill_1.Get(),
		DiceFaceFill_2.Get(),
		DiceFaceFill_3.Get(),
		DiceLabel_0.Get(),
		DiceLabel_1.Get(),
		DiceLabel_2.Get(),
		DiceLabel_3.Get(),
	};

	for (UWidget* LegacyDiceWidget : LegacyDiceWidgets)
	{
		if (LegacyDiceWidget != nullptr)
		{
			LegacyDiceWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UCombatTileMapHUDWidget::HideLegacySkillDetailCard() const
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	const TCHAR* LegacyDetailWidgetNames[] = {
		TEXT("CarouselBackPlate"),
		TEXT("CarouselBackPlate_Fill"),
		TEXT("CarouselBackPlate_Line"),
		TEXT("CarouselTitleText"),
		TEXT("CloseButton"),
		TEXT("CloseButtonSize"),
		TEXT("CloseButtonText"),
	};

	for (const TCHAR* LegacyDetailWidgetName : LegacyDetailWidgetNames)
	{
		if (UWidget* LegacyDetailWidget = WidgetTree->FindWidget(FName(LegacyDetailWidgetName)))
		{
			LegacyDetailWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UCombatTileMapHUDWidget::HideLegacySkillRail() const
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	for (int32 SkillIndex = 0; SkillIndex < CombatSkillSlotCount; ++SkillIndex)
	{
		const FString BaseName = FString::Printf(TEXT("CarouselItem_%d"), SkillIndex);
		const FString LegacySkillWidgetNames[] = {
			BaseName,
			BaseName + TEXT("_Fill"),
			BaseName + TEXT("_Line"),
			BaseName + TEXT("_Accent"),
		};

		for (const FString& LegacySkillWidgetName : LegacySkillWidgetNames)
		{
			if (UWidget* LegacySkillWidget = WidgetTree->FindWidget(FName(*LegacySkillWidgetName)))
			{
				LegacySkillWidget->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}
