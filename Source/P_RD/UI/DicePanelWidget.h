/** @brief 인게임 주사위 버튼으로 여는 임시 주사위 패널. */
// @file DicePanelWidget.h

#pragma once

#include "RDMinimal.h"
#include "UI/CarouselPanelWidget.h"
#include "UI/DiceViewData.h"

#include "DicePanelWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UImage;
class UIndexedButtonWidget;
class UTextBlock;
class UWidget;
class ACombatDiceCaptureActor;

/** @brief DICE 버튼으로 여는 보유 주사위 확인 패널 */
// 현재 단계에서는 실제 주사위 데이터 사용/소모 로직을 붙이지 않고, WBP로 만든 주사위 패널을 인게임에서 열고 조작할 수 있게 한다.
// 처음에는 보유 주사위를 한 줄로 보여주고, 주사위를 누르면 상세 캐러셀 화면으로 전환한다.
// 상세 화면에서는 선택한 주사위와 면 구성을 크게 보여준다.
// 왜 표시만 처리하는가:
// 주사위 사용 규칙은 전투 시스템/스킬 시스템과 연결되어야 하므로 UI 패널이 먼저 결정하면 안 된다.
// 또한 모바일에서 주사위를 직접 드래그해 돌리면 터치 입력과 캡처 갱신이 겹쳐 화면이 떨릴 수 있다.
// 그래서 지금은 "어떤 주사위를 가지고 있는지 확인한다"는 표시 역할만 남긴다.
// 실제 주사위 사용이 붙으면 이 패널은 "어떤 주사위를 눌렀다"는 입력만 올리고,
// 주사위 소모나 효과 적용은 별도 게임 로직으로 넘기는 형태가 맞다.
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UDicePanelWidget : public UCarouselPanelWidget
{
	GENERATED_BODY()

public:
	/** @brief 주사위 패널 기본 객체를 생성한다. */
	// 카드 슬롯과 닫기 버튼은 부모 UCarouselPanelWidget이 WBP 이름 규칙으로 찾는다.
	UDicePanelWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	/** @brief WBP 기본 Construct 흐름을 유지한다. */
	void NativeConstruct() override;

	/** @brief 런타임 캐러셀 버튼 이벤트를 해제한다. */
	void NativeDestruct() override;

	/** @brief 예약된 선택 주사위 캡처만 처리한다. */
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** @brief 패널이 열릴 때 현재 Run의 보유 주사위와 전투 HUD의 굴림 결과를 다시 읽는다. */
	void ApplyOpenUI() override;

	/** @brief PC/에디터 마우스 누름으로 주사위 선택 후보를 기록한다. */
	FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** @brief PC/에디터 마우스 뗌으로 같은 주사위를 눌렀는지 확정한다. */
	FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** @brief 포인터가 패널 밖으로 나가면 주사위 선택 후보를 정리한다. */
	void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	/** @brief 모바일 터치 시작으로 주사위 선택 후보를 기록한다. */
	FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	/** @brief 모바일 터치 종료로 같은 주사위를 눌렀는지 확정한다. */
	FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	/** @brief 카드 자체는 기울이지 않고, 카드 안의 3D 주사위 액터만 회전시킨다. */
	float GetCarouselItemAngle(int32 ItemIndex) const override;

	/** @brief WBP 목업 6개가 아니라 현재 보유 주사위 개수만 캐러셀 카드로 사용한다. */
	int32 GetCarouselItemDisplayCount() const override;

	/** @brief 선택 카드가 바뀌면 상세 카드 배치를 다시 적용한다. */
	void HandleCarouselSelectionChanged(int32 PreviousIndex, int32 NewIndex) override;

private:
	// WBP 목업 슬롯은 최대 6개지만 실제 표시 개수는 mDicePanelViews.Num()으로 제한한다.
	static constexpr int32 DicePanelSlotCount = 6;

	// d20까지 면 버튼을 만들 수 있게 잡은 상한. 실제 표시 여부는 선택 주사위 FaceCount가 결정한다.
	static constexpr int32 DicePanelMaxFaceButtonCount = 20;

	// 터치 시작/종료가 이 거리 안이면 탭으로 본다. 캐러셀 드래그와 카드 선택을 분리하는 임계값이다.
	static constexpr float DicePanelTapDistanceThreshold = 28.0f;

	// 선택 면으로 자동 회전하는 보간 속도. 너무 높으면 캡처가 튀고, 너무 낮으면 버튼 피드백이 늦다.
	static constexpr float DicePanelFaceRotationSpeed = 8.0f;

	// 목표 회전과 거의 일치했는지 판정하는 dot 임계값. 도달 후 불필요한 RenderTarget 캡처를 멈춘다.
	static constexpr float DicePanelFaceRotationSnapDot = 0.9995f;

	/** @brief RunPersistData의 보유 주사위와 전투 HUD의 굴림 결과를 패널 표시 데이터로 변환한다. */
	void RefreshDicePanelViews();

	/** @brief WBP에 배치된 DiceFace_N 슬롯을 현재 표시 데이터에 맞게 갱신한다. */
	void RefreshDicePanelWidgets();

	/** @brief 지정 주사위 카드의 배경/숫자/점 표시를 갱신한다. */
	void ApplyDiceFaceVisuals(int32 DiceIndex, const FDiceViewData& DiceView);

	/** @brief 주사위 숫자에 맞게 WBP 점 위젯을 표시한다. */
	void ApplyDicePipVisibility(int32 DiceIndex, int32 ResultValue, bool IsRolled) const;

	/** @brief WBP에 없는 6면용 중간 점을 런타임으로 만들거나 찾아 반환한다. */
	UBorder* EnsureRuntimeDicePip(int32 DiceIndex, TCHAR PipName, const FVector2D& Position) const;

	/** @brief WidgetTree에서 이름으로 위젯을 찾아 반환한다. */
	UWidget* FindDiceWidget(const FString& WidgetName) const;

	/** @brief WidgetTree에서 이름으로 Border를 찾아 반환한다. */
	UBorder* FindDiceBorder(const FString& WidgetName) const;

	/** @brief WidgetTree에서 이름으로 TextBlock을 찾아 반환한다. */
	UTextBlock* FindDiceText(const FString& WidgetName) const;

	/** @brief 현재 굴림 상태에 맞는 숫자 텍스트를 반환한다. */
	FText BuildDiceFaceText(const FDiceViewData& DiceView) const;

	/** @brief 선택 카드의 3D 주사위 액터에 회전값을 적용한다. */
	void ApplyDiceRotation();

	/** @brief 선택 주사위를 지정한 눈이 보이는 방향으로 부드럽게 회전시킨다. */
	void StartDicePanelFaceRotation(int32 FaceValue);

	/** @brief 회전 중인 3D 주사위 캡처를 갱신한다. */
	void TickDicePanelFaceRotation(float InDeltaTime);

	/** @brief 지정 주사위만 다음 Tick에서 다시 캡처하도록 예약한다. */
	void RequestDicePanelPreviewCapture(int32 DiceIndex);

	/** @brief 지정 주사위의 현재 회전값을 RenderTarget에 반영한다. */
	void CaptureDicePanelPreviewActor(int32 DiceIndex);

	/** @brief WBP 카드 슬롯 위에 투명 RenderTarget Image를 만든다. */
	void EnsureDicePanelPreviewWidgets();

	/** @brief 3D 주사위 전용 카드 배치를 적용한다. */
	void ApplyDicePanelLayout();

	/** @brief 지정 Image에 주사위 RenderTarget을 연결한다. */
	void ApplyDicePanelCaptureBrush(UImage* DiceImage, ACombatDiceCaptureActor* DiceActor, FVector2D BrushSize) const;

	/** @brief 주사위 패널 전용 캡처 액터를 생성한다. */
	ACombatDiceCaptureActor* SpawnDicePanelCaptureActor(int32 DiceIndex, int32 RenderTargetSize);

	/** @brief 패널에서 만든 캡처 액터들을 제거한다. */
	void DestroyDicePanelCaptureActors();

	/** @brief 카드 슬롯 Image 뒤의 3D 주사위 캡처 액터를 준비하고 표시 상태를 갱신한다. */
	void RefreshDicePanelPreviewActors();

	/** @brief 지정 주사위 카드의 기본 3D 회전값을 반환한다. */
	FRotator GetDicePanelIdleRotation(int32 DiceIndex) const;

	/** @brief 화면 좌표 아래의 3D 주사위 카드 index를 찾는다. */
	int32 FindDicePanelIndexAtPosition(const FVector2D& ScreenPosition) const;

	/** @brief 주사위 카드 터치 후보를 기록한다. */
	bool BeginDicePanelPress(const FVector2D& ScreenPosition);

	/** @brief 터치 시작/종료 위치가 같은 카드면 캐러셀 선택으로 확정한다. */
	bool FinishDicePanelPress(const FVector2D& ScreenPosition);

	/** @brief 지정 index를 선택하고 캐러셀 배치로 전환한다. */
	void SelectDicePanelIndex(int32 DiceIndex);

	/** @brief 캐러셀 상태에서 쓰는 좌우 버튼과 면 숫자 안내를 만든다. */
	void EnsureDiceCarouselControlWidgets();

	/** @brief 캐러셀 보조 UI 위치를 맞춘다. */
	void ApplyDiceCarouselControlLayout() const;

	/** @brief 캐러셀 보조 UI 표시 상태와 텍스트를 갱신한다. */
	void RefreshDiceCarouselControlWidgets();

	/** @brief 캐러셀 면 숫자 버튼 색상을 현재 선택 상태에 맞춘다. */
	void RefreshDiceCarouselFaceButtonStyles();

	/** @brief 주사위 면 버튼을 해당 면 값 핸들러에 연결한다. */
	void BindDiceCarouselFaceButton(UIndexedButtonWidget* FaceButton, int32 FaceValue);

	/** @brief 주사위 면 버튼의 해당 면 값 핸들러 연결을 해제한다. */
	void UnbindDiceCarouselFaceButton(UIndexedButtonWidget* FaceButton, int32 FaceValue);

	/** @brief 생성된 모든 주사위 면 버튼 이벤트를 해제한다. */
	void UnbindDiceCarouselFaceButtons();

	/** @brief 주사위 면 버튼 공통 처리. */
	void HandleDiceCarouselFaceButtonClicked(int32 FaceValue);

	/** @brief IndexedButtonWidget이 넘긴 1-base 면 값을 공통 면 버튼 처리로 보낸다. */
	UFUNCTION()
	void HandleDiceCarouselFaceIndexedButtonClicked(int32 FaceValue);

	/** @brief Carousel에서 이전 주사위를 선택한다. */
	UFUNCTION()
	void HandleDiceCarouselPreviousButtonClicked();

	/** @brief Carousel에서 다음 주사위를 선택한다. */
	UFUNCTION()
	void HandleDiceCarouselNextButtonClicked();

	/** @brief Carousel 상세에서 overview 목록 배치로 돌아간다. */
	UFUNCTION()
	void HandleDiceCarouselBackButtonClicked();

	UFUNCTION()
	void HandleDiceCarouselFaceOneButtonClicked();

	UFUNCTION()
	void HandleDiceCarouselFaceTwoButtonClicked();

	UFUNCTION()
	void HandleDiceCarouselFaceThreeButtonClicked();

	UFUNCTION()
	void HandleDiceCarouselFaceFourButtonClicked();

	UFUNCTION()
	void HandleDiceCarouselFaceFiveButtonClicked();

	UFUNCTION()
	void HandleDiceCarouselFaceSixButtonClicked();

private:
	/** @brief 현재 패널에 표시할 보유 주사위 목록 */
	TArray<FDiceViewData> mDicePanelViews;

	/** @brief 각 카드 슬롯 안에 표시되는 주사위 RenderTarget Image */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mDicePanelImages;

	/** @brief 각 3D 주사위 Image 뒤에 깔리는 카드 배경 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mDicePanelCardBorders;

	/** @brief 각 카드 슬롯 RenderTarget을 만드는 3D 주사위 액터 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ACombatDiceCaptureActor>> mDicePanelPreviewActors;

	/** @brief 각 카드 슬롯 3D 주사위의 현재 회전값 */
	TArray<FQuat> mDicePanelPreviewRotations;

	/** @brief 각 카드 슬롯 3D 주사위가 도달해야 하는 회전값 */
	TArray<FQuat> mDicePanelTargetPreviewRotations;

	/** @brief 각 카드 슬롯 3D 주사위가 목표 회전으로 보간 중인지 여부 */
	TArray<bool> mDicePanelPreviewRotationAnimations;

	/** @brief 각 카드 슬롯에서 마지막으로 직접 선택한 면 숫자 */
	TArray<int32> mDicePanelSelectedFaceValues;

	/** @brief 하나의 주사위를 선택해 캐러셀 배치로 전환했는지 여부 */
	bool mDicePanelCarouselActivated = false;

	/** @brief 현재 선택된 3D 주사위 카드 index */
	int32 mSelectedDicePanelIndex = 0;

	/** @brief 터치 시작 시점에 눌린 3D 주사위 카드 index */
	int32 mPressedDicePanelIndex = INDEX_NONE;

	/** @brief 탭과 드래그를 구분하기 위한 터치 시작 좌표 */
	FVector2D mDicePanelPressPosition = FVector2D::ZeroVector;

	/** @brief 다음 Tick에서 다시 캡처할 주사위 index */
	int32 mPendingDicePanelCaptureIndex = INDEX_NONE;

	/** @brief 런타임 위젯을 붙일 RootCanvas */
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> mRootCanvas;

	/** @brief 캐러셀에서 현재 선택된 주사위 순번을 표시하는 제목 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mDiceCarouselTitleText;

	/** @brief 선택된 주사위의 면 구성과 현재 결과를 안내하는 텍스트 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mDiceCarouselFaceText;

	/** @brief 선택된 주사위를 특정 면으로 돌리는 숫자 버튼 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UIndexedButtonWidget>> mDiceCarouselFaceButtons;

	/** @brief 숫자 버튼 안에 표시되는 TextBlock */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mDiceCarouselFaceButtonTexts;

	/** @brief 캐러셀에서 이전 주사위를 선택하는 버튼 */
	UPROPERTY(Transient)
	TObjectPtr<UButton> mDiceCarouselPreviousButton;

	/** @brief 캐러셀에서 다음 주사위를 선택하는 버튼 */
	UPROPERTY(Transient)
	TObjectPtr<UButton> mDiceCarouselNextButton;

	/** @brief 캐러셀에서 전체 주사위 목록으로 돌아가는 버튼 */
	UPROPERTY(Transient)
	TObjectPtr<UButton> mDiceCarouselBackButton;
};
