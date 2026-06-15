#include "UI/IndexedButtonWidget.h"

UIndexedButtonWidget::UIndexedButtonWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 부모 UButton의 클릭/누름 이벤트를 내부 핸들러로 한 번만 연결한다.
	// 여기서 받아 index를 실어 다시 쏘기 때문에, 호출부는 index 없는 원본 이벤트를 직접 구독할 필요가 없다.
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
	// 원본 클릭을 index를 붙인 형태로 재전파한다. 이 한 줄이 이 래퍼의 존재 이유다.
	OnIndexedClicked.Broadcast(mButtonIndex);
}

void UIndexedButtonWidget::HandlePressed()
{
	// Press 단계에서도 같은 index payload를 유지해, 사운드/강조 같은 즉시 피드백 코드가 클릭 확정을 기다리지 않게 한다.
	OnIndexedPressed.Broadcast(mButtonIndex);
}
