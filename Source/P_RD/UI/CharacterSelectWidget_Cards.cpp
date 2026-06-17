#include "UI/CharacterSelectWidget.h"

#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "UI/CharacterCardWidget.h"

/** @brief WBP가 배치한 카드 인스턴스에 GameMode 후보 데이터를 순서대로 주입한다. */
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

	/*
	 * 카드는 C++이 생성하지 않는다.
	 * WBP가 카드의 개수/위치/반응형 배치를 소유하고, C++은 GameMode가 준 후보 데이터를 순서대로 주입한다.
	 */
	// DisplayCount는 "보여줄 수 있는 카드 수"다. 후보가 더 많으면 로그로 알리고, WBP가 없는 카드는 C++이 만들지 않는다.
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

/** @brief 컨테이너 아래 WBP_CharacterCard를 디자이너가 배치한 순서대로 재귀 수집한다. */
void UCharacterSelectWidget::CollectDesignerCharacterCards(UPanelWidget* RootPanel, TArray<UCharacterCardWidget*>& OutCards) const
{
	if (RootPanel == nullptr)
	{
		return;
	}

	// 카드가 SizeBox/Overlay 등으로 감싸져 있어도 WBP hierarchy 그대로 찾기 위해 재귀 탐색한다.
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

/** @brief 현재 선택 index와 카드별 View index를 비교해 선택 연출만 다시 보낸다. */
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

/** @brief 카드 클릭 이벤트를 선택 상태 갱신으로 위임한다. */
void UCharacterSelectWidget::HandleCharacterCardClicked(int32 CharacterIndex)
{
	SelectCharacter(CharacterIndex);
}
