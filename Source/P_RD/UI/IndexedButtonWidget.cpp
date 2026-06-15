#include "UI/IndexedButtonWidget.h"

UIndexedButtonWidget::UIndexedButtonWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OnClicked.AddUniqueDynamic(this, &UIndexedButtonWidget::HandleClicked);
	OnPressed.AddUniqueDynamic(this, &UIndexedButtonWidget::HandlePressed);
}

void UIndexedButtonWidget::SetButtonIndex(int32 InButtonIndex)
{
	mButtonIndex = InButtonIndex;
}

int32 UIndexedButtonWidget::GetButtonIndex() const
{
	return mButtonIndex;
}

void UIndexedButtonWidget::HandleClicked()
{
	OnIndexedClicked.Broadcast(mButtonIndex);
}

void UIndexedButtonWidget::HandlePressed()
{
	OnIndexedPressed.Broadcast(mButtonIndex);
}
