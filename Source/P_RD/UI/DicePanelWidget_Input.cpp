#include "UI/DicePanelWidget.h"

#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "InputCoreTypes.h"

/** @brief PC/에디터에서 주사위 선택 후보를 기록한다. */
// 마우스를 누른 위치의 주사위를 바로 확정하지 않고 후보로만 저장한다.
// 손을 뗀 위치가 같은 카드일 때만 상세 캐러셀 선택으로 확정해, 버튼이나 패널 배경 입력과 섞이지 않게 한다.
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

/** @brief PC/에디터에서 같은 주사위를 눌렀는지 확정한다. */
// 선택 후보가 없으면 부모 캐러셀 입력을 그대로 사용한다.
// 후보가 있으면 마우스 캡처를 해제해 다른 UI가 다시 정상적으로 입력을 받을 수 있게 한다.
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

/** @brief 마우스가 위젯 밖으로 나가면 선택 후보를 정리한다. */
void UDicePanelWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	mPressedDicePanelIndex = INDEX_NONE;
	mDicePanelPressPosition = FVector2D::ZeroVector;
	Super::NativeOnMouseLeave(InMouseEvent);
}

/** @brief 모바일 터치로 주사위 선택 후보를 기록한다. */
FReply UDicePanelWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	const FVector2D ScreenPosition = InGestureEvent.GetScreenSpacePosition();
	if (BeginDicePanelPress(ScreenPosition))
	{
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
}

/** @brief 모바일 터치 종료 시 같은 주사위를 눌렀는지 확정한다. */
FReply UDicePanelWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (mPressedDicePanelIndex != INDEX_NONE)
	{
		FinishDicePanelPress(InGestureEvent.GetScreenSpacePosition());
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnTouchEnded(InGeometry, InGestureEvent);
}

/** @brief 화면 좌표 아래에 있는 주사위 카드 중 가장 위 ZOrder의 index를 찾는다. */
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
		// Carousel에서는 카드가 겹칠 수 있으므로 같은 좌표에 여러 카드가 걸리면 가장 높은 ZOrder를 선택한다.
		if (ZOrder >= BestZOrder)
		{
			BestZOrder = ZOrder;
			BestIndex = DiceIndex;
		}
	}

	return BestIndex;
}

/** @brief 터치 시작 위치가 유효한 주사위 카드 위라면 탭 후보로 저장한다. */
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

/** @brief 터치 종료가 시작 카드와 충분히 가까우면 카드 선택으로 확정한다. */
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

/** @brief 지정 주사위를 상세 carousel 중심 카드로 선택하고 preview/layout/control을 갱신한다. */
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
