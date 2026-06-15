#include "UI/DicePanelWidget.h"

#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "InputCoreTypes.h"

/**
 * @brief PC/에디터에서 주사위 선택 후보를 기록한다.
 *
 * @details
 * 마우스를 누른 위치의 주사위를 바로 확정하지 않고 후보로만 저장한다.
 * 손을 뗀 위치가 같은 카드일 때만 상세 캐러셀 선택으로 확정해, 버튼이나 패널 배경 입력과 섞이지 않게 한다.
 */
FReply UDicePanelWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
	if (BeginDicePanelPress(ScreenPosition))
	{
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

/**
 * @brief PC/에디터에서 같은 주사위를 눌렀는지 확정한다.
 *
 * @details
 * 선택 후보가 없으면 부모 캐러셀 입력을 그대로 사용한다.
 * 후보가 있으면 마우스 캡처를 해제해 다른 UI가 다시 정상적으로 입력을 받을 수 있게 한다.
 */
FReply UDicePanelWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}

	if (mPressedDicePanelIndex != INDEX_NONE)
	{
		FinishDicePanelPress(InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

/**
 * @brief 마우스가 위젯 밖으로 나가면 선택 후보를 정리한다.
 */
void UDicePanelWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	mPressedDicePanelIndex = INDEX_NONE;
	mDicePanelPressPosition = FVector2D::ZeroVector;
	Super::NativeOnMouseLeave(InMouseEvent);
}

/**
 * @brief 모바일 터치로 주사위 선택 후보를 기록한다.
 */
FReply UDicePanelWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	const FVector2D ScreenPosition = InGestureEvent.GetScreenSpacePosition();
	if (BeginDicePanelPress(ScreenPosition))
	{
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
}

/**
 * @brief 모바일 터치 종료 시 같은 주사위를 눌렀는지 확정한다.
 */
FReply UDicePanelWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (mPressedDicePanelIndex != INDEX_NONE)
	{
		FinishDicePanelPress(InGestureEvent.GetScreenSpacePosition());
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnTouchEnded(InGeometry, InGestureEvent);
}

int32 UDicePanelWidget::FindDicePanelIndexAtPosition(const FVector2D& ScreenPosition) const
{
	int32 BestIndex = INDEX_NONE;
	int32 BestZOrder = TNumericLimits<int32>::Lowest();
	const int32 DiceCount = FMath::Min(mDicePanelViews.Num(), DicePanelSlotCount);

	for (int32 DiceIndex = 0; DiceIndex < DiceCount; ++DiceIndex)
	{
		const UImage* DiceImage = mDicePanelImages.IsValidIndex(DiceIndex) ? mDicePanelImages[DiceIndex].Get() : nullptr;
		if (DiceImage == nullptr || DiceImage->GetVisibility() == ESlateVisibility::Collapsed)
		{
			continue;
		}

		const bool bIsUnderDiceImage = DiceImage->GetCachedGeometry().IsUnderLocation(ScreenPosition);
		const bool bIsUnderCard = mDicePanelCardBorders.IsValidIndex(DiceIndex)
			&& mDicePanelCardBorders[DiceIndex] != nullptr
			&& mDicePanelCardBorders[DiceIndex]->GetCachedGeometry().IsUnderLocation(ScreenPosition);
		if (bIsUnderDiceImage == false && bIsUnderCard == false)
		{
			continue;
		}

		int32 ZOrder = 0;
		if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(DiceImage->Slot))
		{
			ZOrder = CanvasSlot->GetZOrder();
		}
		if (ZOrder >= BestZOrder)
		{
			BestZOrder = ZOrder;
			BestIndex = DiceIndex;
		}
	}

	return BestIndex;
}

bool UDicePanelWidget::BeginDicePanelPress(const FVector2D& ScreenPosition)
{
	const int32 DiceIndex = FindDicePanelIndexAtPosition(ScreenPosition);
	if (DiceIndex == INDEX_NONE)
	{
		return false;
	}

	mPressedDicePanelIndex = DiceIndex;
	mDicePanelPressPosition = ScreenPosition;
	return true;
}

bool UDicePanelWidget::FinishDicePanelPress(const FVector2D& ScreenPosition)
{
	const int32 ReleasedIndex = FindDicePanelIndexAtPosition(ScreenPosition);
	const bool bIsTap = mPressedDicePanelIndex != INDEX_NONE
		&& ReleasedIndex == mPressedDicePanelIndex
		&& FVector2D::Distance(mDicePanelPressPosition, ScreenPosition) <= DicePanelTapDistanceThreshold;

	if (bIsTap)
	{
		SelectDicePanelIndex(mPressedDicePanelIndex);
	}

	mPressedDicePanelIndex = INDEX_NONE;
	mDicePanelPressPosition = FVector2D::ZeroVector;
	return bIsTap;
}

void UDicePanelWidget::SelectDicePanelIndex(int32 DiceIndex)
{
	if (mDicePanelViews.IsValidIndex(DiceIndex) == false)
	{
		return;
	}

	mDicePanelCarouselActivated = true;
	mSelectedDicePanelIndex = DiceIndex;
	ApplyDicePanelLayout();
	RefreshDicePanelPreviewActors();
	RefreshDiceCarouselControlWidgets();
}
