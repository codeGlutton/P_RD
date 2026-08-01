#include "UI/CarouselPanelWidget.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/Widget.h"
#include "InputCoreTypes.h"
#include "UI/CarouselLayoutPolicy.h"

/**
 * @brief 에디터/PC 환경의 마우스 누름을 캐러셀 항목 선택 시작으로 처리한다.
 *
 * @details
 * 버튼이 없는 카드 영역도 선택할 수 있게 위젯 Geometry 기준으로 눌린 항목을 찾는다.
 * 눌린 항목이 있으면 마우스를 캡처해 ButtonUp까지 같은 위젯에서 받는다.
 */
FReply UCarouselPanelWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && BeginCarouselPress(InMouseEvent.GetScreenSpacePosition()))
	{
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

/**
 * @brief 마우스 뗌 위치가 처음 누른 카드와 같은지 확인해 탭 선택을 확정한다.
 *
 * @details
 * 누른 뒤 일정 거리 이상 움직이면 드래그/스크롤 의도로 보고 선택하지 않는다.
 * 모바일 터치와 같은 규칙을 쓰기 위해 FinishCarouselPress()로 공통 처리한다.
 */
FReply UCarouselPanelWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && mPressedCarouselIndex != INDEX_NONE)
	{
		FinishCarouselPress(InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

/**
 * @brief 모바일 터치 시작을 캐러셀 항목 선택 시작으로 처리한다.
 */
FReply UCarouselPanelWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (BeginCarouselPress(InGestureEvent.GetScreenSpacePosition()))
	{
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
}

/**
 * @brief 모바일 터치 종료를 캐러셀 항목 선택 확정으로 처리한다.
 */
FReply UCarouselPanelWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (mPressedCarouselIndex != INDEX_NONE)
	{
		FinishCarouselPress(InGestureEvent.GetScreenSpacePosition());
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnTouchEnded(InGeometry, InGestureEvent);
}

bool UCarouselPanelWidget::BeginCarouselPress(const FVector2D& ScreenPosition)
{
	/*
	 * 눌림 시작 시점에는 선택을 확정하지 않는다.
	 * 사용자가 손가락을 살짝 움직인 뒤 놓는 경우를 스크롤/드래그로 볼 수 있어,
	 * FinishCarouselPress()에서 같은 카드 위에서 짧게 끝났는지 다시 확인한다.
	 */
	const int32 ItemIndex = FindCarouselItemIndexAtPosition(ScreenPosition);
	if (ItemIndex == INDEX_NONE)
	{
		return false;
	}

	mPressedCarouselIndex = ItemIndex;
	mCarouselPressPosition = ScreenPosition;
	return true;
}

bool UCarouselPanelWidget::FinishCarouselPress(const FVector2D& ScreenPosition)
{
	/*
	 * Press 시작 카드와 Release 카드가 같고 이동 거리가 작을 때만 탭으로 인정한다.
	 * 터치 스크롤이나 카드 드래그가 단순 선택으로 오인되지 않게 하기 위한 기준이다.
	 */
	const int32 ReleasedItemIndex = FindCarouselItemIndexAtPosition(ScreenPosition);
	const bool bIsTap = mPressedCarouselIndex != INDEX_NONE
		&& ReleasedItemIndex == mPressedCarouselIndex
		&& FVector2D::Distance(mCarouselPressPosition, ScreenPosition) <= FCarouselLayoutPolicy::TapDistanceThreshold;

	if (bIsTap)
	{
		SelectCarouselItem(mPressedCarouselIndex);
	}

	mPressedCarouselIndex = INDEX_NONE;
	mCarouselPressPosition = FVector2D::ZeroVector;
	return bIsTap;
}

int32 UCarouselPanelWidget::FindCarouselItemIndexAtPosition(const FVector2D& ScreenPosition) const
{
	/*
	 * 캐러셀에서는 카드들이 겹쳐 보일 수 있다.
	 * 화면 좌표 아래에 여러 카드가 겹치면 ZOrder가 가장 높은, 즉 가장 앞에 있는 카드를 선택한다.
	 */
	int32 BestItemIndex = INDEX_NONE;
	int32 BestZOrder = TNumericLimits<int32>::Lowest();

	const int32 ItemCount = GetClampedCarouselItemDisplayCount();
	for (int32 Index = 0; Index < ItemCount; ++Index)
	{
		const UWidget* Item = mCarouselItems[Index];
		if (Item == nullptr || !Item->GetCachedGeometry().IsUnderLocation(ScreenPosition))
		{
			continue;
		}

		const int32 ZOrder = GetCarouselItemZOrder(Index);
		if (ZOrder >= BestZOrder)
		{
			BestZOrder = ZOrder;
			BestItemIndex = Index;
		}
	}

	return BestItemIndex;
}

int32 UCarouselPanelWidget::GetCarouselItemZOrder(int32 ItemIndex) const
{
	/*
	 * CanvasPanelSlot이 있으면 실제 배치 ZOrder를 사용한다.
	 * WBP 구조가 바뀌어 CanvasPanelSlot이 아니더라도 선택 판정이 완전히 깨지지 않도록 0을 반환한다.
	 */
	const UWidget* Item = mCarouselItems.IsValidIndex(ItemIndex) ? mCarouselItems[ItemIndex].Get() : nullptr;
	if (Item == nullptr)
	{
		return TNumericLimits<int32>::Lowest();
	}

	if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Item->Slot))
	{
		return CanvasSlot->GetZOrder();
	}

	return 0;
}
