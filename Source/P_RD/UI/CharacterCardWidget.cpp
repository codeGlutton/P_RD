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
		// 잠긴 캐릭터도 눌러 상세/잠금 사유를 보여준다. 실제 확정 가능 여부는 선택 화면의 mSelectable 검증이 막는다.
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
	// 배열 위치가 아니라 GameMode가 내려준 안정적인 option index를 올려 보낸다.
	OnCharacterCardClicked.Broadcast(mCharacterOption.mIndex);
}
