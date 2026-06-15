#include "UI/IndexedButtonWidget.h"

UIndexedButtonWidget::UIndexedButtonWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 기본 클릭/누름 이벤트를 받아서, 버튼 번호가 붙은 이벤트로 다시 보낸다.
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
	// 몇 번째 버튼이 클릭됐는지 같이 알려준다.
	OnIndexedClicked.Broadcast(mButtonIndex);
}

void UIndexedButtonWidget::HandlePressed()
{
	// 누르는 순간에도 같은 번호를 알려줘서 사운드나 강조 효과에 쓸 수 있게 한다.
	OnIndexedPressed.Broadcast(mButtonIndex);
}
