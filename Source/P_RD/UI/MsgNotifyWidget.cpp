#include "UI/MsgNotifyWidget.h"

#include "Components/TextBlock.h"

UMsgNotifyWidget::UMsgNotifyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, mDefaultMessage(NSLOCTEXT("MsgNotifyWidget", "DefaultMessage", "Message"))
	, mCurrentMessage(mDefaultMessage)
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UMsgNotifyWidget::SetMessage(const FText& Message)
{
	mCurrentMessage = Message;
	if (MessageTextBlock != nullptr)
	{
		MessageTextBlock->SetText(mCurrentMessage);
	}
}

void UMsgNotifyWidget::OpenUI_Implementation()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMsgNotifyWidget::CloseUI_Implementation()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UMsgNotifyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetMessage(mCurrentMessage);
}
