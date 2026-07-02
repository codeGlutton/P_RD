#pragma once

#include "RDMinimal.h"
#include "UI/DiceViewData.h"
#include "UI/Combat/CombatUITypes.h"
#include "UI/RDUserWidget.h"
#include "Widgets/Layout/Anchors.h"

#include "CombatTileMapHUDWidget.generated.h"

class ACombatDiceCaptureActor;
class ACombatDiceRollCaptureActor;
class UBorder;
class UButton;
class UCanvasPanel;
class UCombatUIModel;
class UIndexedButtonWidget;
class UImage;
class UProgressBar;
class UTextBlock;
class UViewport;
class UWidget;

/** @brief 전투 타일맵 HUD 와이어프레임 위젯 */
// 현재는 실제 전투 API가 붙기 전의 UI 시안 단계다.
// 스킬 레일, 보유 주사위, 명령 버튼 배치와 함께 전투 진입 시 주사위가 굴러가는 3D 연출을 확인한다.
// 주사위 연출은 타일맵 월드에 액터를 직접 올리지 않고, UMG Viewport의 별도 미리보기 월드에서만 재생한다.
// 이렇게 해야 카메라/타일/유닛 배치와 독립적으로 "화면 정면에서 주사위가 굴러 멈추는" UI 연출을 만들 수 있다.
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UCombatTileMapHUDWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	UCombatTileMapHUDWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	/** @brief 전투 HUD가 현재 표시 중인 보유 주사위 개수를 반환한다. */
	int32 GetCombatDiceViewCount() const;

	/** @brief 지정 index의 보유 주사위 표시 상태를 반환한다. */
	// DicePanel은 실제 주사위 굴림을 다시 만들지 않고, 전투 HUD가 이미 굴린 결과를 읽어 같은 값을 보여준다.
	// 반환값이 false면 해당 index에 표시할 주사위가 없다는 뜻이다.
	bool GetCombatDiceView(int32 DiceIndex, FDiceViewData& OutDiceView) const;

	/** @brief 전투 뷰모델을 연결한다(데이터/비주얼 분리 경계). */
	// 연결되면 주사위 굴림 결과는 더 이상 HUD가 정하지 않고 뷰모델(게임플레이/Mock)에서 읽는다.
	// 미연결 상태에서는 기존 단독 동작(시안용 임시 굴림)을 그대로 유지해 회귀가 없다.
	// 게임플레이 어댑터가 준비되면 여기에 같은 뷰모델을 넘기면 된다(UI 무수정).
	void BindCombatUIModel(UCombatUIModel* InUIModel);

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

	/** @brief WBP에 디자이너 스킨(HUD_* 앵커 위젯)이 있는지 한 번 판별해 캐시한다. */
	void ResolveDesignerSkin();

	/** @brief 디자이너 스킨이 활성(WBP가 좌표/아트를 정의)인지 여부. */
	bool IsDesignerSkinActive() const { return mDesignerSkinActive; }

	/** @brief WBP의 명명 앵커 위젯(HUD_*)에서 정규화 영역을 읽는다. 없으면 Fallback(기존 코드 좌표). */
	FAnchors GroupRect(FName AnchorWidgetName, const FAnchors& Fallback) const;

	/** @brief 스킬 레일 전체 영역(WBP HUD_SkillRail 또는 기존 상수 기반 Fallback). */
	FAnchors GetSkillRailGroupRect() const;

	/** @brief 스킬 레일 Count개 중 Index 항목의 정규화 영역(렌더/히트테스트 공용). */
	FAnchors GetSkillRailItemRect(int32 Index, int32 Count) const;

	/** @brief 입장 굴림용 실제 물리 테이블 캡처 액터를 준비한다. */
	void EnsureDiceRollPhysicsActor();

	/** @brief 입장 굴림용 물리 캡처 액터를 정리한다. */
	void DestroyDiceRollPhysicsActor();

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

	/** @brief 물리 굴림에서 읽은 결과면을 시안/단독 표시 데이터에 반영한다. */
	void ApplyIntroDicePhysicsResults();

	/** @brief 굴림 연출의 현재 프레임을 계산한다. */
	void UpdateIntroDiceRoll(float InDeltaTime);

	/** @brief 굴림 대기 상태에서 기기 가속도(흔들기)를 감지해 자동으로 굴림을 시작한다. */
	void UpdateShakeToRoll(float InDeltaTime);

	/** @brief 주사위 팝업이 떠 있는 동안 전투 HUD 조작층을 숨기거나 되돌린다. */
	void SetDiceRollCombatLayerSuppressed(bool bSuppressed);

	/** @brief 모든 주사위 결과를 보유 주사위 카드에 한 번에 반영한다. */
	void MarkAllDiceRolled();

	/** @brief 주사위 연출 UI를 보이거나 숨긴다. */
	// @param NewVisibility 적용할 표시 상태
	void SetDiceRollVisibility(ESlateVisibility NewVisibility);

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

	/** @brief 뷰모델 도메인 갱신 알림. 메타/유닛/턴이 바뀌면 상단 상태바를 다시 그린다. */
	UFUNCTION()
	void HandleCombatUIChanged(ECombatUIDomain Domain);

	/** @brief 뷰모델의 플레이어 메타(Lv/HP/Gold)를 상단 상태바 텍스트로 반영한다. */
	void RefreshCombatStatusBar() const;

	/** @brief 디자이너 스킨 시, concept value 칸(HUD_M_lv/hp/gold_value 앵커)에 Lv/HP/Gold 텍스트를 칸 크기에 맞춰 그린다. */
	void RefreshSkinValueLabels() const;

	/** @brief 장비 슬롯 칩(탑바 좌측 하단)을 뷰모델 장비 뷰로 다시 만든다. */
	void RebuildEquipmentBar();

	/** @brief 턴 순서 칩(탑바 가운데 하단)을 다시 만든다(무조건 플레이어부터, 그 뒤 적). */
	void RebuildTurnOrderBar();

	/** @brief 스킬/액션이 확정·취소되면 스킬·주사위 선택 강조를 푼다. */
	UFUNCTION()
	void HandleCombatActionResolved();

	/** @brief 우측 MOVE 버튼 클릭 → 이동 모드 진입 의도(RequestMove). */
	UFUNCTION()
	void HandleMoveButtonClicked();

	/** @brief 우측 MOVE 버튼의 이동 가능 수치(현재/최대)를 갱신한다. */
	void RefreshMoveButton() const;

	/** @brief 탑바 내비 버튼 클릭 → 숨겨진 TopMenuBar의 해당 패널 토글로 위임한다(MAP/DICE/SKILL/SET). */
	UFUNCTION()
	void HandleNavMapButtonClicked();
	UFUNCTION()
	void HandleNavDiceButtonClicked();
	UFUNCTION()
	void HandleNavSkillButtonClicked();
	UFUNCTION()
	void HandleNavSettingsButtonClicked();

	/** @brief 월드위젯 서브시스템에서 공용 탑바 위젯을 가져온다(스킨 모드에서 숨겨져 있어도 인스턴스는 살아있음). */
	class UTopMenuBarWidget* GetTopMenuBar() const;

	/** @brief 스킨 모드에서 레거시 탑바(WBP_TopMenuBar)를 접어 둔다. 패널 토글의 ApplyInputPassThrough가
	    매번 탑바를 다시 보이게 하므로, concept HUD 위에 레거시 상태바/INVENTORY/ROOM 배너가 겹쳐 뜨는 걸 막는다. */
	void HideLegacyTopBarWhenSkinned() const;

	/** @brief 뷰모델의 유닛 수에 맞춰 머리 위 HP바 위젯을 다시 만든다. */
	void RebuildUnitHpBars();

	/** @brief 각 유닛의 월드 위치를 화면에 투영해 HP바 위치/비율/색을 매 프레임 갱신한다. */
	void UpdateUnitHpBars();

	/** @brief 스킬 레일 입력 해제 시 탭/롱프레스 결과를 확정한다. */
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

	/** @brief 보유 주사위 카드의 선택 강조를 모두 끈다(선택의 진실원본은 DicePoolModel의 mIsSelected). */
	void ClearOwnedDiceSelectionHighlight();

private:
	/** @brief 런타임으로 주사위/턴 종료 UI를 붙일 WBP 루트 Canvas */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> RootCanvas;

	/** @brief 디자이너 스킨 아트와 런타임 위젯이 함께 쓰는 1920x1080 기준 Canvas */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> DesignCanvas;

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

	/** @brief 전투 진입 주사위 팝업 뒤에서 전투 HUD를 가리는 반투명 배경 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> mDiceRollBackdropPanel;

	/** @brief 실제 물리 테이블을 한 번에 캡처한 RenderTarget Image */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> mDiceRollPhysicsImage;

	/** @brief 전투 진입 주사위가 놓이는 팝업 보드 배경 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> mDiceRollBoardImage;

	/** @brief 팝업 보드 Texture2D */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mDiceRollBoardTexture;

	/** @brief 여러 주사위를 하나의 숨겨진 물리 테이블에서 굴리는 캡처 액터 */
	UPROPERTY(Transient)
	TObjectPtr<ACombatDiceRollCaptureActor> mDiceRollPhysicsActor;

	/** @brief 현재 런에서 읽은 보유 주사위 표시 데이터 */
	TArray<FDiceViewData> mDiceUIs;

	/** @brief 연결되면 주사위 굴림 결과의 출처가 되는 전투 뷰모델(미연결 시 기존 단독 동작) */
	UPROPERTY(Transient)
	TObjectPtr<UCombatUIModel> mCombatUIModel;

	/** @brief 해소된 행동 큐 노드(데미지/힐/상태)를 잠깐 보여주는 런타임 전투 피드 텍스트 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mCombatFeedText;

	/** @brief 상단 상태바(플레이어 Lv/HP/Gold) 런타임 텍스트 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mCombatStatusBarText;

	/** @brief 장비 칩(탑바 좌측 하단) 배경/문구 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mEquipmentChips;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mEquipmentChipTexts;

	/** @brief 턴 순서 칩(탑바 가운데 하단, 플레이어부터) 배경/문구 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mTurnOrderChips;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mTurnOrderChipTexts;

	/** @brief 우측 MOVE 명령 버튼(이동 모드 진입) */
	UPROPERTY(Transient)
	TObjectPtr<UButton> mMoveButton;

	/** @brief 탑바 내비 투명 버튼(concept 아트 위 클릭영역). 런타임 생성, TopMenuBar 패널 토글로 위임. */
	UPROPERTY(Transient)
	TObjectPtr<UButton> mNavMapButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> mNavDiceButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> mNavSkillButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> mNavSettingsButton;

	/** @brief 유닛 머리 위에 월드→스크린 투영으로 띄우는 HP바(유닛 뷰 순서와 1:1) */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UProgressBar>> mUnitHpBars;

	/** @brief 보유 주사위를 그리는 투명 RenderTarget Image 위젯 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mOwnedDiceImages;

	/** @brief 보유 주사위 RenderTarget을 만드는 3D 주사위 액터 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ACombatDiceCaptureActor>> mOwnedDicePreviewActors;

	/** @brief 보유 주사위 선택 입력을 받는 투명 버튼 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UIndexedButtonWidget>> mOwnedDiceCardWidgets;

	/** @brief 주사위 종류 라벨(d2/d4/d6/d8/d10/d12/d20) */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mOwnedDiceTypeTexts;

	/** @brief 중립 상태로 표시하는 런타임 스킬 레일 배경 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mSkillRailPanels;

	/** @brief 중립 상태로 표시하는 런타임 스킬 레일 문구 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mSkillRailTexts;

	/** @brief 스킬 레일 입력을 받기 위해 WBP 위에 얹는 투명 버튼들 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UIndexedButtonWidget>> mSkillInputButtons;

	/** @brief 현재 선택된 스킬 index */
	int32 mSelectedSkillIndex = INDEX_NONE;

	/** @brief WBP에 디자이너 스킨(HUD_* 앵커 위젯)이 있어 좌표/아트를 WBP에서 읽는 모드인지. */
	bool mDesignerSkinActive = false;

	/** @brief 지금 누르고 있는 스킬 index */
	int32 mPressedSkillIndex = INDEX_NONE;

	/** @brief 현재 프레임에서 스킬 레일 입력을 추적 중인지 여부 */
	bool mSkillPressing = false;

	/** @brief 같은 누름에서 탭 선택과 롱프레스 상세가 동시에 발생하지 않게 막는 latch */
	bool mSkillDetailOpenedFromPress = false;

	/** @brief 현재 스킬 누름의 누적 시간. NativeTick에서만 증가해 입력 이벤트 순서에 덜 민감하게 둔다. */
	float mSkillPressElapsed = 0.0f;

	/** @brief 스킬 상세를 열기 위해 필요한 누름 시간. [합의필요] 모바일 손맛 기준으로 확정되면 설정화 대상. */
	float mSkillLongPressThreshold = 0.45f;

	/** @brief 입장 주사위가 실제 회전/정착 애니메이션을 재생 중인지 여부 */
	bool mIntroDiceRollActive = false;

	/** @brief 입장 주사위가 화면에 준비되어 첫 터치를 기다리는 상태 */
	bool mIntroDiceRollReady = false;

	/** @brief 결과 표시가 끝나 닫기 터치만 기다리는 상태 */
	bool mIntroDiceResultWaitingForDismiss = false;

	/** @brief 보유 주사위 카드에 결과를 이미 반영했는지 여부. 반복 Tick으로 중복 갱신하지 않기 위한 latch */
	bool mIntroDiceResultsApplied = false;

	/** @brief 입장 굴림의 실제 결과를 전투/시안 데이터에서 이미 받아왔는지 여부 */
	bool mIntroDiceRollResultsResolved = false;

	/** @brief 현재 주사위 연출 누적 시간 */
	float mIntroDiceRollElapsed = 0.0f;

	/** @brief 굴림 완료 후 정렬을 시작하기까지의 대기 간격(결과를 잠깐 보여주는 시간). */
	float mIntroDiceAlignDelay = 0.55f;

	/** @brief 정렬 시작 전 대기 누적 시간. */
	float mIntroDiceAlignTimer = 0.0f;

	/** @brief 주사위가 한 줄로 정렬되는 애니메이션 진행 중인지. */
	bool mIntroDiceAligning = false;

	/** @brief 정렬이 끝나 탭으로 닫기를 기다리는 상태인지. */
	bool mIntroDiceAligned = false;

	/** @brief 직전 프레임의 기기 가속도(흔들기 감지용 기준값). */
	FVector mLastShakeAcceleration = FVector::ZeroVector;

	/** @brief 가속도 기준값이 한 번이라도 채워졌는지(첫 프레임 오탐 방지). */
	bool mHasShakeBaseline = false;

	/** @brief 흔들기 한 번에 한 번만 굴리도록 남은 쿨다운(초). */
	float mShakeRollCooldown = 0.0f;

	/** @brief 흔들기로 인정할 가속도 변화량 임계값(기기 단위). 작을수록 민감. */
	float mShakeTriggerThreshold = 2.6f;

	/** @brief 모션 입력 활성화를 한 번만 요청하기 위한 플래그. */
	bool mMotionControlsRequested = false;

	/** @brief 선택된 주사위/스킬 배치 상태를 보여주는 안내 문구 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mDiceAssignmentText;

	/** @brief 롱프레스 때만 보이는 스킬 상세 카드 배경 */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> mSkillDetailPanel;

	/** @brief 롱프레스 때만 보이는 스킬 상세 문구 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mSkillDetailText;

	/** @brief 스킬칸을 제외한 왼쪽 빈 영역을 어둡게 덮는 상세 오버레이 조각 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mSkillDetailBackdropPanels;

	/** @brief 주사위 연출을 시작/닫기 위한 투명 입력 버튼 */
	UPROPERTY(Transient)
	TObjectPtr<UButton> mDiceRollInputButton;

	/** @brief 스킬 상세가 열린 동안 바깥 터치를 받는 투명 버튼 */
	UPROPERTY(Transient)
	TObjectPtr<UButton> mSkillDetailDismissButton;
};
