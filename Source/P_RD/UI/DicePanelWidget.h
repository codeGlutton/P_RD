/**
 * @file DicePanelWidget.h
 * @brief 인게임 탑바 주사위 버튼으로 여는 임시 주사위 패널.
 */

#pragma once

#include "RDMinimal.h"
#include "UI/CarouselPanelWidget.h"

#include "DicePanelWidget.generated.h"

/**
 * @brief 캐러셀 선택 위에 선택된 주사위 카드 회전 입력을 더한 패널
 *
 * @details
 * 현재 단계에서는 실제 주사위 데이터 사용/소모 로직을 붙이지 않고, WBP로 만든 주사위 패널을 탑바에서 열고 조작할 수 있게 한다.
 * 선택된 항목 위에서 드래그하면 임시 회전만 적용해 모바일 터치 반응을 확인한다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UDicePanelWidget : public UCarouselPanelWidget
{
	GENERATED_BODY()

public:
	UDicePanelWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	FReply NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	float GetCarouselItemAngle(int32 ItemIndex) const override;
	void HandleCarouselSelectionChanged(int32 PreviousIndex, int32 NewIndex) override;

private:
	bool StartDiceDrag(const FVector2D& ScreenPosition);
	void UpdateDiceDrag(const FVector2D& ScreenPosition);
	void FinishDiceDrag();
	void ApplyDiceRotation() const;

private:
	bool mDraggingDice = false;
	float mDiceAngle = 0.0f;
	FVector2D mLastPointerPosition = FVector2D::ZeroVector;
};
