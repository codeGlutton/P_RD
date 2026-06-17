#include "UI/CharacterCardWidget.h"

#include "Components/Button.h"

UCharacterCardWidget::UCharacterCardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UCharacterCardWidget::SetCharacterOption(const FFrontendCharacterOption& InOption, bool bSelected)
{
	mCharacterOption = InOption;
	mIsSelected = bSelected;
	SyncCard();
}

void UCharacterCardWidget::SetSelected(bool bSelected)
{
	mIsSelected = bSelected;
	SyncCard();
}

void UCharacterCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ValidateDesignerBindings();

	if (CardButton != nullptr)
	{
		CardButton->OnClicked.AddUniqueDynamic(this, &UCharacterCardWidget::HandleCardButtonClicked);
	}

	SyncCard();
}

void UCharacterCardWidget::NativeDestruct()
{
	if (CardButton != nullptr)
	{
		CardButton->OnClicked.RemoveDynamic(this, &UCharacterCardWidget::HandleCardButtonClicked);
	}

	Super::NativeDestruct();
}

void UCharacterCardWidget::SyncCard()
{
	if (CardButton != nullptr)
	{
		CardButton->SetIsEnabled(true);
	}

	BP_OnCharacterOptionChanged(mCharacterOption, mIsSelected);
}

void UCharacterCardWidget::ValidateDesignerBindings() const
{
	if (CardButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterCardWidget: WBP_CharacterCard requires CardButton."));
	}
}

void UCharacterCardWidget::HandleCardButtonClicked()
{
	OnCharacterCardClicked.Broadcast(mCharacterOption.mIndex);
}
