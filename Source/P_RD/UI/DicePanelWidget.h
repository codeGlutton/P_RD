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
 *
 * 왜 임시 회전만 처리하는가:
 * 주사위 사용 규칙은 전투 시스템/스킬 시스템과 연결되어야 하므로 UI 패널이 먼저 결정하면 안 된다.
 * 지금은 WBP 배치, 탑바 OpenUI 연결, 모바일 터치 반응이 정상인지 확인하는 단계다.
 * 실제 주사위 사용이 붙으면 이 패널은 "어떤 주사위를 눌렀다/드래그했다"는 입력만 올리고,
 * 주사위 소모나 효과 적용은 별도 게임 로직으로 넘기는 형태가 맞다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UDicePanelWidget : public UCarouselPanelWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 주사위 패널 기본 객체를 생성한다.
	 *
	 * @details
	 * 카드 슬롯과 닫기 버튼은 부모 UCarouselPanelWidget이 WBP 이름 규칙으로 찾는다.
	 */
	UDicePanelWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	/** @brief PC/에디터 마우스 누름으로 선택 주사위 드래그를 시작한다. */
	FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** @brief PC/에디터 마우스 뗌으로 선택 주사위 드래그를 종료한다. */
	FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** @brief PC/에디터 마우스 이동량을 선택 주사위 회전으로 반영한다. */
	FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** @brief 포인터가 패널 밖으로 나가면 드래그 상태를 정리한다. */
	void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	/** @brief 모바일 터치 시작으로 선택 주사위 드래그를 시작한다. */
	FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	/** @brief 모바일 터치 이동량을 선택 주사위 회전으로 반영한다. */
	FReply NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	/** @brief 모바일 터치 종료로 선택 주사위 드래그를 끝낸다. */
	FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	/** @brief 선택된 카드에만 현재 주사위 회전 각도를 반환한다. */
	float GetCarouselItemAngle(int32 ItemIndex) const override;

	/** @brief 선택 카드가 바뀌면 이전 카드의 임시 회전 상태를 초기화한다. */
	void HandleCarouselSelectionChanged(int32 PreviousIndex, int32 NewIndex) override;

private:
	/** @brief 현재 포인터가 선택 카드 위에 있으면 드래그 상태로 진입한다. */
	bool StartDiceDrag(const FVector2D& ScreenPosition);

	/** @brief 포인터 이동량으로 임시 회전 각도를 갱신한다. */
	void UpdateDiceDrag(const FVector2D& ScreenPosition);

	/** @brief 드래그 상태를 종료한다. */
	void FinishDiceDrag();

	/** @brief 선택 카드 위젯 RenderTransform에 회전값을 적용한다. */
	void ApplyDiceRotation() const;

private:
	/** @brief 선택된 주사위 카드를 드래그 중인지 여부 */
	bool mDraggingDice = false;

	/** @brief 선택된 주사위 카드에 적용할 임시 회전 각도 */
	float mDiceAngle = 0.0f;

	/** @brief 직전 포인터 위치. 현재 위치와의 차이로 회전량을 계산한다. */
	FVector2D mLastPointerPosition = FVector2D::ZeroVector;
};
