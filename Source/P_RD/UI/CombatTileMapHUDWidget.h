#pragma once

#include "RDMinimal.h"
#include "UI/DiceViewData.h"
#include "UI/Combat/CombatViewTypes.h"
#include "UI/RDUserWidget.h"

#include "CombatTileMapHUDWidget.generated.h"

class ACombatDiceCaptureActor;
class UBorder;
class UButton;
class UCanvasPanel;
class UCombatViewModel;
class UIndexedButtonWidget;
class UImage;
class UTextBlock;
class UViewport;
class UWidget;

/**
 * @brief 전투 타일맵 HUD 와이어프레임 위젯
 *
 * @details
 * 현재는 실제 전투 API가 붙기 전의 UI 시안 단계다.
 * 스킬 레일, 보유 주사위, 명령 버튼 배치와 함께 전투 진입 시 주사위가 굴러가는 3D 연출을 확인한다.
 *
 * 주사위 연출은 타일맵 월드에 액터를 직접 올리지 않고, UMG Viewport의 별도 미리보기 월드에서만 재생한다.
 * 이렇게 해야 카메라/타일/유닛 배치와 독립적으로 "화면 정면에서 주사위가 굴러 멈추는" UI 연출을 만들 수 있다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UCombatTileMapHUDWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	UCombatTileMapHUDWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	/** @brief 전투 HUD가 현재 표시 중인 보유 주사위 개수를 반환한다. */
	int32 GetCombatDiceViewCount() const;

	/**
	 * @brief 지정 index의 보유 주사위 표시 상태를 반환한다.
	 *
	 * @details
	 * DicePanel은 실제 주사위 굴림을 다시 만들지 않고, 전투 HUD가 이미 굴린 결과를 읽어 같은 값을 보여준다.
	 * 반환값이 false면 해당 index에 표시할 주사위가 없다는 뜻이다.
	 */
	bool GetCombatDiceView(int32 DiceIndex, FPrimaryAssetId& OutDiceId, ERarityType& OutRarityType, int32& OutResultValue, bool& OutIsRolled) const;

	/**
	 * @brief 전투 뷰모델을 연결한다(데이터/비주얼 분리 경계).
	 *
	 * @details
	 * 연결되면 주사위 굴림 결과는 더 이상 HUD가 정하지 않고 뷰모델(게임플레이/Mock)에서 읽는다.
	 * 미연결 상태에서는 기존 단독 동작(시안용 임시 굴림)을 그대로 유지해 회귀가 없다.
	 * 게임플레이 어댑터가 준비되면 여기에 같은 뷰모델을 넘기면 된다(UI 무수정).
	 */
	void BindCombatViewModel(UCombatViewModel* InViewModel);

protected:
	/** @brief WBP 바인딩과 버튼 이벤트를 연결한다. */
	void NativeConstruct() override;

	/** @brief 위젯 제거 시 버튼 이벤트를 해제한다. */
	void NativeDestruct() override;

	/** @brief 입장 주사위 연출을 시간에 따라 갱신한다. */
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** @brief PC/에디터에서 스킬 레일을 누르기 시작한 위치를 기록한다. */
	FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** @brief PC/에디터에서 스킬 레일 입력을 짧은 선택 또는 롱프레스 상세로 확정한다. */
	FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** @brief 모바일에서 스킬 레일을 누르기 시작한 위치를 기록한다. */
	FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	/** @brief 모바일에서 스킬 레일 입력을 짧은 선택 또는 롱프레스 상세로 확정한다. */
	FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	/** @brief OpenUI()로 표시될 때 주사위 입장 연출을 다시 시작한다. */
	void ApplyOpenUI() override;

	/** @brief 전투 HUD가 TopMenuBar보다 뒤에 깔리도록 낮은 ZOrder를 사용한다. */
	int32 GetViewportZOrder() const override;

private:
	/** @brief WBP에 없는 테스트용 전투 HUD 요소를 RootCanvas 위에 런타임으로 붙인다. */
	void EnsureRuntimeWidgets();

	/** @brief 런타임으로 붙인 위젯들의 화면 위치를 모바일 화면 비율 기준으로 맞춘다. */
	void ApplyRuntimeWidgetLayout() const;

	/** @brief 보유 주사위 개수에 맞춰 투명 캡처 Image와 3D 주사위 액터를 준비한다. */
	void EnsureDicePreviewActors();

	/** @brief RenderTarget 캡처 주사위 액터를 준비하고 Image brush를 갱신한다. */
	void RefreshDicePreviewActors();

	/** @brief 지정 Image에 주사위 RenderTarget을 연결한다. */
	void ApplyDiceCaptureBrush(UImage* DiceImage, ACombatDiceCaptureActor* DiceActor, FVector2D BrushSize) const;

	/** @brief 주사위 캡처 액터를 UI 전용 위치에 생성한다. */
	ACombatDiceCaptureActor* SpawnDiceCaptureActor(int32 GroupIndex, int32 DiceIndex, int32 RenderTargetSize);

	/** @brief 기존 주사위 캡처 액터들을 정리한다. */
	void DestroyDiceCaptureActors(TArray<TObjectPtr<ACombatDiceCaptureActor>>& DiceActors) const;

	/** @brief 현재 RunPersistData의 보유 주사위 목록을 전투 HUD 표시용 데이터로 변환한다. */
	void RefreshDiceViewsFromRunData();

	/** @brief 이전 WBP 시안에 남아 있는 고정 주사위 슬롯을 숨긴다. */
	void HideLegacyDiceSlots() const;

	/** @brief 이전 WBP 시안에 항상 보이던 스킬 상세 카드를 숨긴다. */
	void HideLegacySkillDetailCard() const;

	/** @brief 이전 WBP 시안의 고정 스킬 레일을 숨긴다. */
	void HideLegacySkillRail() const;

	/** @brief 선택 강조가 들어가지 않은 중립 스킬 레일을 런타임으로 다시 만든다. */
	void RebuildSkillRailWidgets();

	/** @brief 현재 선택된 스킬만 가볍게 강조한다. */
	void RefreshSkillRailWidgets();

	/** @brief 보유 주사위 3D 표시 위젯을 현재 주사위 개수에 맞춰 다시 만든다. */
	void RebuildOwnedDiceCards();

	/** @brief WBP 스킬 레일 위에 투명 입력 버튼을 얹어 짧은 탭과 롱프레스를 받는다. */
	void EnsureSkillInputButtons();

	/** @brief 보유 주사위 3D 표시의 숫자/색/선택 상태를 현재 굴림 상태에 맞게 갱신한다. */
	void RefreshOwnedDiceCards();

	/** @brief 전투 진입 주사위 굴림을 바로 시작하지 않고 터치 대기 상태로 준비한다. */
	void PrepareIntroDiceRoll();

	/** @brief 전투 진입 주사위 굴림 연출을 시작한다. */
	void StartIntroDiceRoll();

	/** @brief 굴림 연출의 현재 프레임을 계산한다. */
	void UpdateIntroDiceRoll(float InDeltaTime);

	/** @brief 모든 주사위 결과를 보유 주사위 카드에 한 번에 반영한다. */
	void MarkAllDiceRolled();

	/**
	 * @brief 주사위 연출 UI를 보이거나 숨긴다.
	 *
	 * @param NewVisibility 적용할 표시 상태
	 */
	void SetDiceRollVisibility(ESlateVisibility NewVisibility) const;

	/** @brief 주사위 연출 영역을 눌렀을 때 대기/결과 상태에 맞춰 굴림 시작 또는 닫기를 처리한다. */
	UFUNCTION()
	void HandleDiceRollInputButtonClicked();

	/** @brief 결과 확인이 끝난 주사위 연출 UI를 닫는다. */
	void DismissIntroDiceRoll();

	/** @brief 스킬 상세 카드 바깥을 눌렀을 때 상세 카드를 닫는다. */
	UFUNCTION()
	void HandleSkillDetailDismissButtonClicked();

	/** @brief 턴 종료 버튼 클릭을 받는다. 실제 전투 API가 붙기 전까지는 로그만 남긴다. */
	UFUNCTION()
	void HandleEndTurnButtonClicked();

	/** @brief 보유 주사위 카드를 누르면 배치 후보 주사위로 선택한다. */
	UFUNCTION()
	void HandleOwnedDiceCardClicked(int32 DiceIndex);

	/** @brief 행동 큐 노드 하나가 해소될 때 호출. 전투 피드에 수치/라벨을 표시한다(머리 위 위치는 게임플레이 follow-up). */
	UFUNCTION()
	void HandleCombatQueueNodeResolved(FCombatQueueNode Node);

	UFUNCTION()
	void HandleSkillButtonReleased();

	/** @brief 투명 스킬 입력 버튼이 눌렸을 때 공통 누름 상태로 진입한다. */
	UFUNCTION()
	void HandleSkillButtonPressed(int32 SkillIndex);

	/** @brief 화면 좌표가 스킬 레일 위라면 해당 스킬 index를 반환한다. */
	int32 FindSkillRailIndexAtScreenPosition(const FVector2D& ScreenPosition) const;

	/** @brief 스킬 레일 누름을 시작한다. 짧게 떼면 선택, 오래 누르면 상세로 처리한다. */
	void BeginSkillPress(int32 SkillIndex);

	/** @brief 스킬 레일을 누르고 있는 시간을 누적해 롱프레스 상세 표시를 판단한다. */
	void UpdateSkillPress(float InDeltaTime);

	/** @brief 스킬을 선택한다. 다른 스킬로 바뀌면 주사위 배치 후보를 초기화한다. */
	void SelectSkillForAssignment(int32 SkillIndex);

	/** @brief 스킬 상세 카드를 표시한다. 현재는 API 연결 전 설명용 카드만 보여준다. */
	void ShowSkillDetail(int32 SkillIndex);

	/** @brief 스킬 상세 카드를 숨긴다. */
	void HideSkillDetail() const;

	/** @brief 스킬 상세 카드가 현재 화면에 보이는지 반환한다. */
	bool IsSkillDetailVisible() const;

	/** @brief 화면 좌표가 스킬 상세 카드 안쪽인지 확인한다. */
	bool IsScreenPositionInSkillDetailPanel(const FVector2D& ScreenPosition) const;

	/** @brief 상세 카드 바깥 입력이면 상세 카드를 닫는다. */
	bool HideSkillDetailIfClickedOutside(const FVector2D& ScreenPosition);

	/** @brief 선택된 주사위와 스킬의 임시 배치 상태를 안내 문구에 반영한다. */
	void RefreshDiceAssignmentText() const;

private:
	/** @brief 런타임으로 주사위/턴 종료 UI를 붙일 WBP 루트 Canvas */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> RootCanvas;

	/** @brief 이전 WBP에 남아 있을 수 있는 3D 주사위 UMG Viewport. 새 구조에서는 숨긴다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UViewport> DiceRollViewport;

	/** @brief 주사위 연출 안내 문구 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DiceRollStatusText;

	/** @brief 턴 종료 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EndTurnButton;

	/** @brief 이전 WBP 시안에 남아 있는 고정 주사위 슬롯. 실제 보유 주사위 카드와 겹치지 않게 숨긴다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DiceFaceSize_0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DiceFaceSize_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DiceFaceSize_2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DiceFaceSize_3;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DiceFaceFill_0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DiceFaceFill_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DiceFaceFill_2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DiceFaceFill_3;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DiceLabel_0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DiceLabel_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DiceLabel_2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DiceLabel_3;

	/** @brief 전투 진입 주사위 RenderTarget을 표시하는 Image */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mDiceRollImages;

	/** @brief SceneCapture2D로 투명 RenderTarget을 만드는 주사위 액터들 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ACombatDiceCaptureActor>> mDicePreviewActors;

	/** @brief 현재 런에서 읽은 보유 주사위 표시 데이터 */
	TArray<FDiceViewData> mDiceViews;

	/** @brief 연결되면 주사위 굴림 결과의 출처가 되는 전투 뷰모델(미연결 시 기존 단독 동작) */
	UPROPERTY(Transient)
	TObjectPtr<UCombatViewModel> mCombatViewModel;

	/** @brief 해소된 행동 큐 노드(데미지/힐/상태)를 잠깐 보여주는 런타임 전투 피드 텍스트 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mCombatFeedText;

	/** @brief 보유 주사위를 그리는 투명 RenderTarget Image 위젯 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mOwnedDiceImages;

	/** @brief 보유 주사위 RenderTarget을 만드는 3D 주사위 액터 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ACombatDiceCaptureActor>> mOwnedDicePreviewActors;

	/** @brief 보유 주사위 선택 입력을 받는 투명 버튼 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UIndexedButtonWidget>> mOwnedDiceCardWidgets;

	/** @brief 중립 상태로 표시하는 런타임 스킬 레일 배경 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mSkillRailPanels;

	/** @brief 중립 상태로 표시하는 런타임 스킬 레일 문구 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mSkillRailTexts;

	/** @brief 스킬 레일 입력을 받기 위해 WBP 위에 얹는 투명 버튼들 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UIndexedButtonWidget>> mSkillInputButtons;

	/** @brief 스킬-주사위 배치 후보로 선택된 주사위 index */
	int32 mSelectedDiceIndex = INDEX_NONE;

	/** @brief 현재 선택된 스킬 index */
	int32 mSelectedSkillIndex = INDEX_NONE;

	/** @brief 지금 누르고 있는 스킬 index */
	int32 mPressedSkillIndex = INDEX_NONE;

	/** @brief 스킬 레일을 누르고 있는지 여부 */
	bool mSkillPressing = false;

	/** @brief 이번 스킬 누름에서 상세 카드를 이미 열었는지 여부 */
	bool mSkillDetailOpenedFromPress = false;

	/** @brief 스킬 롱프레스 누적 시간 */
	float mSkillPressElapsed = 0.0f;

	/** @brief 스킬 상세를 열기 위해 필요한 누름 시간 */
	float mSkillLongPressThreshold = 0.45f;

	/** @brief 주사위 연출이 진행 중인지 여부 */
	bool mIntroDiceRollActive = false;

	/** @brief 주사위 연출이 터치 시작을 기다리는지 여부 */
	bool mIntroDiceRollReady = false;

	/** @brief 주사위 결과를 보여준 뒤 닫기 터치를 기다리는지 여부 */
	bool mIntroDiceResultWaitingForDismiss = false;

	/** @brief 정지 자세로 보간하는 구간에 들어갔는지 여부 */
	bool mIntroDiceSettling = false;

	/** @brief 굴림 결과를 보유 주사위 카드에 이미 반영했는지 여부 */
	bool mIntroDiceResultsApplied = false;

	/** @brief 현재 주사위 연출 누적 시간 */
	float mIntroDiceRollElapsed = 0.0f;

	/** @brief 굴러가는 구간 길이 */
	float mIntroDiceRollDuration = 1.65f;

	/** @brief 정면 숫자에서 멈춘 뒤 유지하는 시간 */
	float mIntroDiceHoldDuration = 1.25f;

	/** @brief 정지 보간을 시작할 때의 회전값 */
	TArray<FRotator> mIntroDiceSettleStartRotations;

	/** @brief 선택된 주사위/스킬 배치 상태를 보여주는 안내 문구 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mDiceAssignmentText;

	/** @brief 롱프레스 때만 보이는 스킬 상세 카드 배경 */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> mSkillDetailPanel;

	/** @brief 롱프레스 때만 보이는 스킬 상세 문구 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mSkillDetailText;

	/** @brief 주사위 연출을 시작/닫기 위한 투명 입력 버튼 */
	UPROPERTY(Transient)
	TObjectPtr<UButton> mDiceRollInputButton;

	/** @brief 스킬 상세가 열린 동안 바깥 터치를 받는 투명 버튼 */
	UPROPERTY(Transient)
	TObjectPtr<UButton> mSkillDetailDismissButton;
};
