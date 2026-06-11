#include "UI/DicePanelWidget.h"

#include "Components/Widget.h"
#include "InputCoreTypes.h"

UDicePanelWidget::UDicePanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

/**
 * @brief PC/에디터에서 선택된 주사위 카드 드래그를 시작한다.
 *
 * @details
 * 먼저 부모 캐러셀의 선택/탭 처리 기회를 유지한다.
 * 캐러셀이 이미 활성화되어 있고 포인터가 선택 카드 위에 있을 때만 주사위 회전 드래그를 시작한다.
 */
FReply UDicePanelWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	if (!StartDiceDrag(InMouseEvent.GetScreenSpacePosition()))
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	return FReply::Handled().CaptureMouse(TakeWidget());
}

/**
 * @brief PC/에디터에서 주사위 드래그를 종료한다.
 *
 * @details
 * 드래그 중이 아닐 때는 부모 캐러셀의 선택 처리를 그대로 사용한다.
 * 드래그 중이었다면 마우스 캡처를 해제해 다른 UI가 다시 정상적으로 입력을 받을 수 있게 한다.
 */
FReply UDicePanelWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}

	if (!mDraggingDice)
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}

	FinishDiceDrag();
	return FReply::Handled().ReleaseMouseCapture();
}

/**
 * @brief 드래그 중인 마우스 이동량을 선택 카드 회전량으로 바꾼다.
 *
 * @details
 * 현재 단계의 주사위 패널은 실제 주사위 사용/소모를 처리하지 않는다.
 * 여기서 적용하는 회전은 모바일 조작감 확인용 시각 반응이며, 게임 데이터에는 영향을 주지 않는다.
 */
FReply UDicePanelWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!mDraggingDice)
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	UpdateDiceDrag(InMouseEvent.GetScreenSpacePosition());
	return FReply::Handled();
}

/**
 * @brief 마우스가 위젯 밖으로 나가면 진행 중인 드래그를 정리한다.
 */
void UDicePanelWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	FinishDiceDrag();
	Super::NativeOnMouseLeave(InMouseEvent);
}

/**
 * @brief 모바일 터치로 선택된 주사위 카드 드래그를 시작한다.
 */
FReply UDicePanelWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (!StartDiceDrag(InGestureEvent.GetScreenSpacePosition()))
	{
		return Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
	}

	return FReply::Handled();
}

/**
 * @brief 모바일 터치 이동량을 선택 카드 회전량으로 바꾼다.
 */
FReply UDicePanelWidget::NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (!mDraggingDice)
	{
		return Super::NativeOnTouchMoved(InGeometry, InGestureEvent);
	}

	UpdateDiceDrag(InGestureEvent.GetScreenSpacePosition());
	return FReply::Handled();
}

/**
 * @brief 모바일 터치 종료 시 주사위 드래그 상태를 정리한다.
 */
FReply UDicePanelWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (!mDraggingDice)
	{
		return Super::NativeOnTouchEnded(InGeometry, InGestureEvent);
	}

	FinishDiceDrag();
	return FReply::Handled();
}

/**
 * @brief 캐러셀 배치 중 선택된 주사위 카드에만 현재 드래그 회전 각도를 적용한다.
 *
 * @details
 * 선택되지 않은 카드는 캐러셀 기본 각도 0도를 유지한다.
 * 이렇게 해야 선택 카드만 조작 중인 대상으로 보이고, 주변 카드는 배치용 카드처럼 남는다.
 */
float UDicePanelWidget::GetCarouselItemAngle(int32 ItemIndex) const
{
	return ItemIndex == GetSelectedCarouselIndex() ? mDiceAngle : 0.0f;
}

/**
 * @brief 다른 주사위 카드를 선택하면 이전 카드 회전값을 초기화한다.
 *
 * @details
 * 회전은 데이터가 아니라 임시 시각 상태다.
 * 선택이 바뀌었는데 이전 회전값이 유지되면 새 카드가 처음부터 기울어진 상태로 보여 혼란스럽다.
 */
void UDicePanelWidget::HandleCarouselSelectionChanged(int32 PreviousIndex, int32 NewIndex)
{
	mDiceAngle = 0.0f;
}

/**
 * @brief 현재 포인터 위치가 선택된 카드 위라면 드래그 상태로 진입한다.
 *
 * @details
 * 캐러셀이 아직 활성화되지 않은 초기 가로 목록 상태에서는 드래그를 시작하지 않는다.
 * 먼저 카드를 선택해 "이 주사위를 조작하겠다"는 의도가 생긴 뒤에만 회전 입력을 받는다.
 */
bool UDicePanelWidget::StartDiceDrag(const FVector2D& ScreenPosition)
{
	if (!IsCarouselActivated() || !IsPointerOverSelectedCarouselItem(ScreenPosition))
	{
		return false;
	}

	mDraggingDice = true;
	mLastPointerPosition = ScreenPosition;
	return true;
}

/**
 * @brief 포인터 이동량을 누적 회전 각도로 변환하고 선택 카드에 반영한다.
 *
 * @details
 * X/Y 이동을 모두 사용해 모바일에서 대각선으로 움직여도 반응이 보이게 한다.
 * 360도로 나머지 처리해 값이 계속 커지는 것을 막는다.
 */
void UDicePanelWidget::UpdateDiceDrag(const FVector2D& ScreenPosition)
{
	if (!mDraggingDice)
	{
		return;
	}

	const FVector2D Delta = ScreenPosition - mLastPointerPosition;
	if (!Delta.IsNearlyZero())
	{
		mDiceAngle = FMath::Fmod(mDiceAngle + Delta.X * 1.25f + Delta.Y * 0.85f, 360.0f);
		ApplyDiceRotation();
	}

	mLastPointerPosition = ScreenPosition;
}

/**
 * @brief 주사위 드래그 상태를 종료한다.
 */
void UDicePanelWidget::FinishDiceDrag()
{
	mDraggingDice = false;
}

/**
 * @brief 현재 선택 카드 위젯에 임시 회전값을 적용한다.
 *
 * @details
 * 실제 주사위 데이터나 선택 결과를 바꾸지 않고 RenderTransform만 조정한다.
 * 이후 주사위 시스템이 붙으면 이 시각 반응과 실제 사용 로직을 분리해서 연결할 수 있다.
 */
void UDicePanelWidget::ApplyDiceRotation() const
{
	if (UWidget* SelectedItem = GetSelectedCarouselItem())
	{
		SelectedItem->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		SelectedItem->SetRenderTransformAngle(mDiceAngle);
	}
}
