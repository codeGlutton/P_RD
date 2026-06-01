#include "UI/SaveNotifyWidget.h"

#include "Components/TextBlock.h"

USaveNotifyWidget::USaveNotifyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, mSavingText(NSLOCTEXT("SaveNotifyWidget", "SavingText", "Saving..."))
	, mSavedText(NSLOCTEXT("SaveNotifyWidget", "SavedText", "Saved"))
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void USaveNotifyWidget::SetMessage(const FText& Message)
{
	if (MessageTextBlock != nullptr)
	{
		MessageTextBlock->SetText(Message);
	}
}

void USaveNotifyWidget::OpenUI_Implementation()
{
	SetMessage(mSavingText);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void USaveNotifyWidget::CloseUI_Implementation()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void USaveNotifyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetMessage(mSavingText);
}
