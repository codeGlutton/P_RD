#include "UI/CharacterSelectWidget.h"

#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "UI/CharacterCardWidget.h"

void UCharacterSelectWidget::RebuildCharacterCards()
{
	if (mCharacterCardContainer == nullptr)
	{
		return;
	}

	for (UCharacterCardWidget* CardWidget : mCharacterCardWidgets)
	{
		if (CardWidget != nullptr)
		{
			CardWidget->OnCharacterCardClicked.RemoveDynamic(this, &UCharacterSelectWidget::HandleCharacterCardClicked);
		}
	}
	mCharacterCardWidgets.Reset();

	TArray<UCharacterCardWidget*> DesignerCards;
	CollectDesignerCharacterCards(mCharacterCardContainer, OUT DesignerCards);
	if (DesignerCards.IsEmpty())
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: place WBP_CharacterCard children inside mCharacterCardContainer. C++ no longer creates character cards."));
		return;
	}

	const int32 DisplayCount = FMath::Min(mCharacterOptions.Num(), DesignerCards.Num());
	for (int32 CardIndex = 0; CardIndex < DisplayCount; ++CardIndex)
	{
		UCharacterCardWidget* CardWidget = DesignerCards[CardIndex];
		if (CardWidget == nullptr)
		{
			continue;
		}

		const FFrontendCharacterOption& Option = mCharacterOptions[CardIndex];
		CardWidget->SetVisibility(ESlateVisibility::Visible);
		CardWidget->OnCharacterCardClicked.AddUniqueDynamic(this, &UCharacterSelectWidget::HandleCharacterCardClicked);
		CardWidget->SetCharacterOption(Option, Option.mIndex == mSelectedCharacterIndex);
		mCharacterCardWidgets.Add(CardWidget);
	}

	for (int32 CardIndex = DisplayCount; CardIndex < DesignerCards.Num(); ++CardIndex)
	{
		if (UCharacterCardWidget* CardWidget = DesignerCards[CardIndex])
		{
			CardWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (DisplayCount < mCharacterOptions.Num())
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: WBP has %d character cards, but GameMode returned %d options."), DesignerCards.Num(), mCharacterOptions.Num());
	}
}

void UCharacterSelectWidget::CollectDesignerCharacterCards(UPanelWidget* RootPanel, TArray<UCharacterCardWidget*>& OutCards) const
{
	if (RootPanel == nullptr)
	{
		return;
	}

	for (int32 ChildIndex = 0; ChildIndex < RootPanel->GetChildrenCount(); ++ChildIndex)
	{
		UWidget* ChildWidget = RootPanel->GetChildAt(ChildIndex);
		if (UCharacterCardWidget* CardWidget = Cast<UCharacterCardWidget>(ChildWidget))
		{
			OutCards.Add(CardWidget);
			continue;
		}

		if (UPanelWidget* ChildPanel = Cast<UPanelWidget>(ChildWidget))
		{
			CollectDesignerCharacterCards(ChildPanel, OUT OutCards);
		}
	}
}

void UCharacterSelectWidget::SyncCharacterCards() const
{
	for (int32 CardIndex = 0; CardIndex < mCharacterCardWidgets.Num(); ++CardIndex)
	{
		UCharacterCardWidget* CardWidget = mCharacterCardWidgets[CardIndex];
		if (CardWidget != nullptr)
		{
			const bool bSelected = mCharacterOptions.IsValidIndex(CardIndex)
				&& mCharacterOptions[CardIndex].mIndex == mSelectedCharacterIndex;
			CardWidget->SetSelected(bSelected);
		}
	}
}

void UCharacterSelectWidget::HandleCharacterCardClicked(int32 CharacterIndex)
{
	SelectCharacter(CharacterIndex);
}
