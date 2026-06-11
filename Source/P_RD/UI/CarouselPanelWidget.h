/**
 * @file CarouselPanelWidget.h
 * @brief 탑바 플로팅 패널에서 공통으로 쓰는 캐러셀형 선택 위젯 베이스.
 */

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"

#include "CarouselPanelWidget.generated.h"

class UButton;
class UTextBlock;
class UWidget;

/**
 * @brief 여러 카드형 항목을 가로 목록 또는 원형 캐러셀로 보여주는 공통 팝업 베이스
 *
 * @details
 * DicePanel과 SkillPanel은 실제 데이터 연결 전에도 같은 배치/닫기/선택 동작을 공유한다.
 * WBP에는 CarouselItem_0, CarouselItem_1 같은 이름으로 항목을 둔다.
 * 항목 안에 CarouselButton_0 같은 버튼을 따로 넣어두면 클릭 이벤트도 연결하지만, 버튼이 없어도 패널 자체가 터치 위치로 항목을 선택한다.
 *
 * 왜 공통 베이스로 두는가:
 * 주사위와 스킬은 내용은 다르지만 “탑바 버튼으로 열림, 닫기 버튼으로 닫힘, 여러 카드 중 하나를 선택함”이라는 화면 동작이 같다.
 * 이 동작을 각 패널에 따로 넣으면 이후 카드 배치나 터치 반응을 고칠 때 두 파일을 같이 수정해야 한다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class P_RD_API UCarouselPanelWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	UCarouselPanelWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	void NativeConstruct() override;
	void NativeDestruct() override;
	void ApplyOpenUI() override;
	FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	bool IsCarouselActivated() const;
	int32 GetSelectedCarouselIndex() const;
	UWidget* GetSelectedCarouselItem() const;
	bool IsPointerOverSelectedCarouselItem(const FVector2D& ScreenPosition) const;
	virtual float GetCarouselItemAngle(int32 ItemIndex) const;
	virtual void HandleCarouselSelectionChanged(int32 PreviousIndex, int32 NewIndex);

private:
	UFUNCTION()
	void HandleCloseButtonClicked();

	UFUNCTION()
	void HandleCarouselButton0Clicked();

	UFUNCTION()
	void HandleCarouselButton1Clicked();

	UFUNCTION()
	void HandleCarouselButton2Clicked();

	UFUNCTION()
	void HandleCarouselButton3Clicked();

	UFUNCTION()
	void HandleCarouselButton4Clicked();

	UFUNCTION()
	void HandleCarouselButton5Clicked();

	UFUNCTION()
	void HandleCarouselButton6Clicked();

	UFUNCTION()
	void HandleCarouselButton7Clicked();

	void CacheCarouselWidgets();
	void BindCarouselEvents();
	void UnbindCarouselEvents();
	void SelectCarouselItem(int32 ItemIndex);
	void ApplyCarouselLayout();
	void ApplyLinearListLayout();
	void ResetCarouselState();
	bool BeginCarouselPress(const FVector2D& ScreenPosition);
	bool FinishCarouselPress(const FVector2D& ScreenPosition);
	int32 FindCarouselItemIndexAtPosition(const FVector2D& ScreenPosition) const;
	int32 GetCarouselItemZOrder(int32 ItemIndex) const;
	void BindCarouselButton(UButton* Button, int32 ItemIndex);
	void UnbindCarouselButton(UButton* Button, int32 ItemIndex);

private:
	/** @brief 패널을 닫는 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	/** @brief 닫기 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CloseButtonText;

	/** @brief 이름 규칙으로 찾은 캐러셀 항목들 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> mCarouselItems;

	/** @brief 항목 안에 별도 버튼을 둔 WBP를 위한 선택 입력 버튼들 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> mCarouselButtons;

	int32 mSelectedCarouselIndex = 0;
	int32 mPressedCarouselIndex = INDEX_NONE;
	bool mCarouselActivated = false;
	FVector2D mCarouselPressPosition = FVector2D::ZeroVector;
};
