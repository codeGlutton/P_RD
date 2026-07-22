#include "UI/CarouselPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "UI/CarouselLayoutPolicy.h"

namespace
{
	UWidget* FindWidgetByName(const UWidgetTree* WidgetTree, const FString& WidgetName)
	{
		/*
		 * 카드형 패널은 WBP 디자인에서 CarouselItem_0 같은 이름으로 카드 슬롯을 만든다.
		 * C++이 특정 WBP 클래스를 직접 알지 않고도 같은 이름 규칙으로 슬롯을 찾아 공통 배치/입력을 적용하기 위한 helper다.
		 */
		return WidgetTree != nullptr ? WidgetTree->FindWidget(FName(*WidgetName)) : nullptr;
	}
}

/**
 * @brief 닫기 버튼 클릭을 공통 CloseUI() 생명주기로 연결한다.
 */
void UCarouselPanelWidget::HandleCloseButtonClicked()
{
	CloseUI();
}

void UCarouselPanelWidget::HandleCarouselButton0Clicked()
{
	SelectCarouselItem(0);
}

void UCarouselPanelWidget::HandleCarouselButton1Clicked()
{
	SelectCarouselItem(1);
}

void UCarouselPanelWidget::HandleCarouselButton2Clicked()
{
	SelectCarouselItem(2);
}

void UCarouselPanelWidget::HandleCarouselButton3Clicked()
{
	SelectCarouselItem(3);
}

void UCarouselPanelWidget::HandleCarouselButton4Clicked()
{
	SelectCarouselItem(4);
}

void UCarouselPanelWidget::HandleCarouselButton5Clicked()
{
	SelectCarouselItem(5);
}

void UCarouselPanelWidget::HandleCarouselButton6Clicked()
{
	SelectCarouselItem(6);
}

void UCarouselPanelWidget::HandleCarouselButton7Clicked()
{
	SelectCarouselItem(7);
}

void UCarouselPanelWidget::CacheCarouselWidgets()
{
	/*
	 * WBP마다 카드 개수를 코드로 직접 넘기지 않기 위해 이름 규칙으로 최대 8개까지 찾는다.
	 * CarouselItem_N이 있으면 그 위젯을 카드로 사용하고, 없으면 CarouselButton_N 자체를 카드로 사용한다.
	 * 이렇게 해두면 단순 버튼형 WBP와 카드 안에 버튼이 들어간 WBP를 같은 코드로 처리할 수 있다.
	 */
	mCarouselItems.Reset();
	mCarouselButtons.Reset();

	if (CloseButton != nullptr)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCloseButtonClicked);
	}
	if (CloseButtonText != nullptr)
	{
		CloseButtonText->SetText(NSLOCTEXT("CarouselPanelWidget", "CloseButtonText", "X"));
	}

	for (int32 Index = 0; Index < FCarouselLayoutPolicy::MaxItemCount; ++Index)
	{
		const FString ItemName = FString::Printf(TEXT("CarouselItem_%d"), Index);
		UWidget* Item = FindWidgetByName(WidgetTree, ItemName);
		const FString ButtonName = FString::Printf(TEXT("CarouselButton_%d"), Index);
		UButton* Button = Cast<UButton>(FindWidgetByName(WidgetTree, ButtonName));
		if (Item == nullptr)
		{
			Item = Button;
		}
		if (Item == nullptr)
		{
			continue;
		}

		if (Button != nullptr)
		{
			mCarouselButtons.Add(Button);
		}
		mCarouselItems.Add(Item);
	}

	mSelectedCarouselIndex = mCarouselItems.IsEmpty() ? INDEX_NONE : FMath::Clamp(mSelectedCarouselIndex, 0, mCarouselItems.Num() - 1);
}

void UCarouselPanelWidget::BindCarouselEvents()
{
	/*
	 * UButton 델리게이트는 파라미터를 직접 받을 수 없으므로,
	 * 인덱스별 작은 핸들러를 통해 SelectCarouselItem(Index)로 연결한다.
	 */
	for (int32 Index = 0; Index < mCarouselButtons.Num(); ++Index)
	{
		BindCarouselButton(mCarouselButtons[Index], Index);
	}
}

void UCarouselPanelWidget::UnbindCarouselEvents()
{
	/*
	 * 캐시된 버튼 수만큼 Construct에서 붙인 핸들러를 해제한다.
	 * WBP가 재생성되어 이전 버튼 인스턴스가 사라지는 상황에서도 UObject 델리게이트 잔류를 최소화한다.
	 */
	if (CloseButton != nullptr)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCloseButtonClicked);
	}

	for (int32 Index = 0; Index < mCarouselButtons.Num(); ++Index)
	{
		UnbindCarouselButton(mCarouselButtons[Index], Index);
	}
}

void UCarouselPanelWidget::BindCarouselButton(UButton* Button, int32 ItemIndex)
{
	/*
	 * Dynamic delegate는 일반 람다 인덱스 캡처가 아니라 UFUNCTION 바인딩을 요구한다.
	 * 현재 WBP 최대 카드 수를 8개로 제한하고, 각 인덱스 전용 핸들러를 연결한다.
	 */
	if (Button == nullptr)
	{
		return;
	}

	switch (ItemIndex)
	{
	case 0:
		Button->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCarouselButton0Clicked);
		break;
	case 1:
		Button->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCarouselButton1Clicked);
		break;
	case 2:
		Button->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCarouselButton2Clicked);
		break;
	case 3:
		Button->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCarouselButton3Clicked);
		break;
	case 4:
		Button->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCarouselButton4Clicked);
		break;
	case 5:
		Button->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCarouselButton5Clicked);
		break;
	case 6:
		Button->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCarouselButton6Clicked);
		break;
	case 7:
		Button->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCarouselButton7Clicked);
		break;
	default:
		break;
	}
}

void UCarouselPanelWidget::UnbindCarouselButton(UButton* Button, int32 ItemIndex)
{
	/*
	 * BindCarouselButton()에서 붙인 인덱스별 핸들러를 정확히 같은 함수로 해제한다.
	 * AddUniqueDynamic으로 중복을 막아도 Destruct 해제까지 해두면 위젯 재사용 시 이벤트 상태가 예측 가능하다.
	 */
	if (Button == nullptr)
	{
		return;
	}

	switch (ItemIndex)
	{
	case 0:
		Button->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCarouselButton0Clicked);
		break;
	case 1:
		Button->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCarouselButton1Clicked);
		break;
	case 2:
		Button->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCarouselButton2Clicked);
		break;
	case 3:
		Button->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCarouselButton3Clicked);
		break;
	case 4:
		Button->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCarouselButton4Clicked);
		break;
	case 5:
		Button->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCarouselButton5Clicked);
		break;
	case 6:
		Button->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCarouselButton6Clicked);
		break;
	case 7:
		Button->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCarouselButton7Clicked);
		break;
	default:
		break;
	}
}
