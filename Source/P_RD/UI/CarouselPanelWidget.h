/**
 * @file CarouselPanelWidget.h
 * @brief 탑바 플로팅 패널에서 공통으로 쓰는 캐러셀형 선택 위젯 베이스.
 */

#pragma once

#include "RDMinimal.h"
#include "UI/CarouselLayoutPolicy.h"
#include "UI/RDUserWidget.h"

#include "CarouselPanelWidget.generated.h"

class UButton;
class UTextBlock;
class UWidget;

/**
 * @brief 여러 카드형 항목을 가로 목록 또는 원형 캐러셀로 보여주는 공통 팝업 베이스
 *
 * @details
 * 카드형 패널은 실제 데이터 연결 전에도 같은 배치/닫기/선택 동작을 공유한다.
 * WBP에는 CarouselItem_0, CarouselItem_1 같은 이름으로 항목을 둔다.
 * 항목 안에 CarouselButton_0 같은 버튼을 따로 넣어두면 클릭 이벤트도 연결하지만, 버튼이 없어도 패널 자체가 터치 위치로 항목을 선택한다.
 *
 * 왜 공통 베이스로 두는가:
 * 카드 내용과 무관하게 “탑바 버튼으로 열림, 닫기 버튼으로 닫힘, 여러 카드 중 하나를 선택함”이라는 화면 동작을 공유한다.
 * 이 동작을 각 패널에 따로 넣으면 이후 카드 배치나 터치 반응을 고칠 때 두 파일을 같이 수정해야 한다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class P_RD_API UCarouselPanelWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 공통 캐러셀 패널의 표시 계층과 기본 상태를 준비한다.
	 *
	 * @details
	 * 실제 카드 위젯은 WBP에서 만들고, 생성자는 패널이 탑바 팝업 계층에서 보이도록 기본 ZOrder만 잡는다.
	 */
	UCarouselPanelWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	/** @brief WBP 카드 슬롯을 캐시하고 버튼 입력을 연결한다. */
	void NativeConstruct() override;

	/** @brief Construct에서 연결한 버튼 입력을 해제한다. */
	void NativeDestruct() override;

	/** @brief 캐러셀 선택 이동을 부드럽게 보간한다. */
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** @brief 패널이 열릴 때 이전 선택/드래그 상태를 초기화한다. */
	void ApplyOpenUI() override;

	/** @brief PC/에디터 마우스 누름을 카드 탭 후보로 기록한다. */
	FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** @brief PC/에디터 마우스 뗌을 카드 탭 확정으로 처리한다. */
	FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** @brief 모바일 터치 시작을 카드 탭 후보로 기록한다. */
	FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	/** @brief 모바일 터치 종료를 카드 탭 확정으로 처리한다. */
	FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	/** @brief 사용자가 카드를 선택해 원형 캐러셀 배치로 전환된 상태인지 확인한다. */
	bool IsCarouselActivated() const;

	/** @brief 현재 선택된 카드 인덱스를 반환한다. */
	int32 GetSelectedCarouselIndex() const;

	/** @brief 현재 선택된 카드 위젯을 반환한다. */
	UWidget* GetSelectedCarouselItem() const;

	/** @brief 지정 화면 좌표가 현재 선택된 카드 위인지 확인한다. */
	bool IsPointerOverSelectedCarouselItem(const FVector2D& ScreenPosition) const;

	/**
	 * @brief 현재 패널에서 실제로 표시할 카드 개수를 반환한다.
	 *
	 * @details
	 * SkillPanel처럼 WBP에 있는 모든 카드를 그대로 쓰는 패널은 기본값을 사용한다.
	 * 런 데이터에 따라 카드 수가 달라지는 패널은 이 값을 override해
	 * 남는 WBP 목업 카드를 숨긴다.
	 */
	virtual int32 GetCarouselItemDisplayCount() const;

	/**
	 * @brief 카드별 추가 회전 각도를 반환한다.
	 *
	 * @details
	 * 기본값은 0도다. 선택된 카드에 임시 회전을 주는 파생 위젯만 override한다.
	 */
	virtual float GetCarouselItemAngle(int32 ItemIndex) const;

	/**
	 * @brief 선택 카드가 바뀌었거나 첫 선택으로 활성화되었을 때 호출되는 확장 지점이다.
	 *
	 * @details
	 * 파생 패널은 여기서 이전 카드의 임시 표시 상태를 초기화할 수 있다.
	 */
	virtual void HandleCarouselSelectionChanged(int32 PreviousIndex, int32 NewIndex);

	/** @brief 지정 인덱스의 카드를 선택하고 캐러셀 배치를 다시 계산한다. */
	void SelectCarouselItem(int32 ItemIndex);

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

	/** @brief WBP 이름 규칙으로 CarouselItem_N과 CarouselButton_N을 찾아 배열에 저장한다. */
	void CacheCarouselWidgets();

	/** @brief 캐시된 CarouselButton_N 버튼을 인덱스별 선택 핸들러에 연결한다. */
	void BindCarouselEvents();

	/** @brief 캐시된 CarouselButton_N 버튼에서 선택 핸들러를 해제한다. */
	void UnbindCarouselEvents();

	/** @brief WBP 카드 수와 파생 위젯의 표시 카드 수 제한을 함께 반영한 실제 배치 대상 개수 */
	int32 GetClampedCarouselItemDisplayCount() const;

	/** @brief 활성화된 캐러셀 상태에서 선택 카드를 기준으로 원형 배치를 적용한다. */
	void ApplyCarouselLayout(bool bAnimate = false);

	/** @brief 현재 선택/활성 상태 기준으로 각 카드의 목표 배치를 계산한다. */
	void BuildCarouselLayouts(TArray<FCarouselItemLayout>& OutLayouts) const;

	/** @brief 현재 화면에 적용된 카드 배치를 읽어 애니메이션 시작점으로 사용한다. */
	void CaptureCurrentCarouselLayouts(TArray<FCarouselItemLayout>& OutLayouts) const;

	/** @brief 계산된 배치를 실제 카드 위젯에 적용한다. */
	void ApplyCarouselLayouts(const TArray<FCarouselItemLayout>& Layouts);

	/** @brief 현재 배치에서 목표 배치까지 캐러셀 전환 애니메이션을 시작한다. */
	void StartCarouselLayoutTransition(const TArray<FCarouselItemLayout>& TargetLayouts);

	/** @brief 패널 열림 시 선택/눌림/터치 위치 상태를 초기값으로 되돌린다. */
	void ResetCarouselState();

	/** @brief 화면 좌표 기준으로 눌린 카드 인덱스를 기록한다. */
	bool BeginCarouselPress(const FVector2D& ScreenPosition);

	/** @brief 눌렀던 카드와 뗀 카드가 같으면 선택을 확정한다. */
	bool FinishCarouselPress(const FVector2D& ScreenPosition);

	/** @brief 화면 좌표 아래에 있는 카드 중 가장 앞에 있는 카드 인덱스를 찾는다. */
	int32 FindCarouselItemIndexAtPosition(const FVector2D& ScreenPosition) const;

	/** @brief 카드의 CanvasPanel ZOrder를 가져와 겹친 카드 선택 우선순위에 사용한다. */
	int32 GetCarouselItemZOrder(int32 ItemIndex) const;

	/** @brief 인덱스별 UFUNCTION 선택 핸들러를 버튼에 연결한다. */
	void BindCarouselButton(UButton* Button, int32 ItemIndex);

	/** @brief 인덱스별 UFUNCTION 선택 핸들러를 버튼에서 해제한다. */
	void UnbindCarouselButton(UButton* Button, int32 ItemIndex);

private:
	/** @brief 패널을 닫는 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	/** @brief 닫기 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CloseButtonText;

	/**
	 * @brief 이름 규칙으로 찾은 캐러셀 항목들
	 *
	 * @details
	 * WBP에 CarouselItem_0, CarouselItem_1처럼 배치된 실제 카드 위젯이다.
	 * C++은 이 배열만 움직이고, 카드 내부 디자인은 WBP가 전부 담당한다.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> mCarouselItems;

	/**
	 * @brief 항목 안에 별도 버튼을 둔 WBP를 위한 선택 입력 버튼들
	 *
	 * @details
	 * 카드 전체 터치 판정과 별개로, WBP가 CarouselButton_N 버튼을 제공하면 버튼 클릭도 같은 SelectCarouselItem()으로 연결한다.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> mCarouselButtons;

	/** @brief 현재 선택된 카드 인덱스. 선택할 카드가 없으면 INDEX_NONE이다. */
	int32 mSelectedCarouselIndex = 0;

	/** @brief 현재 누르고 있는 카드 인덱스. 탭 확정 전까지 임시로 보관한다. */
	int32 mPressedCarouselIndex = INDEX_NONE;

	/** @brief 첫 카드 선택 이후 원형 캐러셀 배치가 활성화되었는지 여부 */
	bool mCarouselActivated = false;

	/** @brief 탭과 드래그를 구분하기 위해 처음 누른 화면 좌표를 저장한다. */
	FVector2D mCarouselPressPosition = FVector2D::ZeroVector;

	/** @brief 캐러셀 전환 애니메이션의 시작 배치 */
	TArray<FCarouselItemLayout> mCarouselStartLayouts;

	/** @brief 캐러셀 전환 애니메이션의 목표 배치 */
	TArray<FCarouselItemLayout> mCarouselTargetLayouts;

	/** @brief 선택 카드 이동 애니메이션이 진행 중인지 여부 */
	bool mCarouselTransitionActive = false;

	/** @brief 선택 카드 이동 애니메이션 누적 시간 */
	float mCarouselTransitionElapsed = 0.0f;

	/** @brief 선택 카드 이동 애니메이션 길이 */
	float mCarouselTransitionDuration = 0.26f;
};
