#pragma once

#include "RDMinimal.h"
#include "UI/DiceViewData.h"
#include "UI/Combat/CombatUITypes.h"
#include "UI/RDUserWidget.h"
#include "Components/CanvasPanelSlot.h"   // FAnchorData (폴드 변형 베이스 슬롯 캐시)
#include "Widgets/Layout/Anchors.h"

#include "CombatTileMapHUDWidget.generated.h"

class ACombatDiceCaptureActor;
class ACombatDiceRollCaptureActor;
struct FPresentationBarrier;
enum class EWorldWidgetType : uint8;
enum class ESRPGCombatResult : uint8;
class UTexture2D;
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

	/** @brief 전투 HUD가 공용 팝업(월드맵/설정/패널)보다 뒤에 깔리도록 낮은 ZOrder를 사용한다. */
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

	/**
	 * @brief 스킨 런타임 위젯이 붙을 캔버스.
	 * 스킨 활성 시 DesignCanvas(1920x1080 디자인 좌표계, ScaleBox 레터박스) — 디자인 정규화 앵커가 스킨 아트와 정렬된다.
	 * 아니면 풀뷰포트 RootCanvas(레거시 뷰포트 정규화 상수용). 월드투영 HP바만 예외적으로 항상 RootCanvas를 쓴다.
	 */
	UCanvasPanel* GetSkinTargetCanvas() const { return (IsDesignerSkinActive() && DesignCanvas != nullptr) ? DesignCanvas.Get() : RootCanvas.Get(); }

	/** @brief WBP의 명명 앵커 위젯(HUD_*)에서 정규화 영역을 읽는다. 없으면 Fallback(기존 코드 좌표). */
	FAnchors GroupRect(FName AnchorWidgetName, const FAnchors& Fallback) const;

	/** @brief 스킬 레일 전체 영역(WBP HUD_SkillRail 또는 기존 상수 기반 Fallback). */
	FAnchors GetSkillRailGroupRect() const;

	/**
	 * @brief 화면비 변형(폴드/좁은 가로): 배너·턴순서 클러스터를 fold 마커의 y델타로 이동/복원한다.
	 * @details 컷(1.68)은 concept_02 responsive.fold_narrow와 동기. 폴드 마커가 없는 WBP에선 아무것도 안 한다.
	 *          C++은 계산하지 않는다 — 마커(디자이너 데이터)와 베이스 슬롯 캐시 사이를 전환만 한다.
	 */
	void ApplyAspectVariantSlots(const FVector2D& ViewportSize);

	/** @brief 전투 HUD 레이아웃 진단 로그(뷰포트/비율/스킨/HUD_* 그룹 rect). resize·1920x1080 하드코딩 점검용. */
	// 뷰포트 크기가 바뀔 때만 NativeTick에서 호출한다. 타이틀 LayoutMetrics 로그와 같은 목적.
	void LogCombatLayoutMetrics(const FVector2D& ViewportSize) const;

	/** @brief 스킬 레일 Count개 중 Index 항목의 정규화 영역(렌더/히트테스트 공용). */
	FAnchors GetSkillRailItemRect(int32 Index, int32 Count) const;

	/**
	 * @brief 레일 시각 슬롯 -> 스킬 데이터 index 매핑. 미보유 슬롯이면 INDEX_NONE.
	 * @details 레일 고정 배치: 맨 위 칸=기본 공격(평타, 데이터0), 맨 아래 칸=STEP(기본 이동, 데이터1),
	 *          중간 4칸=추가 스킬(데이터 2..5). 데이터 순서는 SkillComponentModel의 "기본2+추가4" 규약을 따른다.
	 */
	int32 GetSkillDataIndexForRailSlot(int32 RailSlotIndex) const;

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

	/** @brief 선택 강조가 들어가지 않은 중립 스킬 레일을 런타임으로 다시 만든다. */
	void RebuildSkillRailWidgets();

	/** @brief 보유 스킬 스냅샷(FSkillUI) 기준으로 레일 슬롯 3상태(보유/사용불가/빈칸)를 다시 그린다. */
	void RefreshSkillRailWidgets();

	/** @brief 보유 스킬의 표시 이름을 뷰모델에서 읽는다(없으면 빈 텍스트 - 시안 라벨 폴백 없음). */
	FText GetOwnedSkillLabel(int32 SkillIndex) const;

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

	/** @brief 스킨 모드: 탑바 레벨 아래에 장착 장비 아이콘(최대 3)을 그린다. 위치·아이콘은 임시(디자이너 조정 예정). */
	void RebuildEquipmentIcons();

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

	/** @brief 내비 버튼 클릭(MAP/DICE/SKILL/SET)을 HUD 소유의 패널 토글로 연결한다. */
	UFUNCTION()
	void HandleNavMapButtonClicked();
	UFUNCTION()
	void HandleNavDiceButtonClicked();
	UFUNCTION()
	void HandleNavSkillButtonClicked();
	UFUNCTION()
	void HandleNavSettingsButtonClicked();

	/* 패널 내비/승리 흐름 (레거시 탑바에서 이관, CombatTileMapHUDWidget_Nav.cpp) */

	/** @brief 전투 모델의 전투 종료 이벤트를 구독해 승리 후 월드맵 흐름을 HUD가 소유한다. 새 전투마다 승리 잠금을 초기화한다. */
	void BindVictoryFlowEvents();

	/** @brief 월드 서브시스템에 등록된 월드 위젯을 URDUserWidget으로 가져온다. */
	URDUserWidget* GetToggleableWorldWidget(EWorldWidgetType WorldWidgetType) const;

	/**
	 * @brief 탑바 배경판(TopBar_Backdrop)을 월드맵 열림 상태와 동기한다.
	 *
	 * @details
	 * 월드맵이 열리는 경로가 여러 갈래(내비 토글/승리 강제/복원)라 개별 훅 대신 틱에서 상태를 본다.
	 * 배경판은 지도 위젯이 아니라 HUD 소유다 — 지도 팝업은 탑바보다 위층이라 탑바 뒤 배경을 가질 수 없다.
	 */
	void UpdateTopBarBackdrop() const;

	/** @brief 지정한 월드 위젯을 공통 CloseUI() 경로로 닫는다. */
	void CloseWorldWidget(EWorldWidgetType WorldWidgetType) const;

	/** @brief 플로팅 패널(MAP/SET/DICE/SKILL)을 하나만 남기고 닫는다(상호배타). */
	void CloseFloatingPanels(EWorldWidgetType ExceptWorldWidgetType) const;

	/** @brief 월드맵 토글. 조회용은 방 선택 비활성, 승리 잠금 중엔 복원 흐름으로 보낸다. */
	void ToggleWorldMap();

	/** @brief 인게임 설정 패널 토글(+InGame 모드/상태 문구 초기화). */
	void ToggleSettingsPanel();

	/** @brief DICE/SKILL처럼 단순히 열고 닫는 플로팅 패널을 토글한다. */
	void ToggleFloatingPanel(EWorldWidgetType WorldWidgetType, const TCHAR* DebugName);

	/** @brief 플레이어 승리 결과를 다음 방 선택 월드맵 표시로 연결한다. */
	void HandleEndCombatUI(TSharedPtr<FPresentationBarrier> Barrier, ESRPGCombatResult Result);

	/** @brief 승리 후 다음 방 선택이 가능한 상태로 월드맵을 연다(OpenUI 완료 시 barrier 해제). */
	void OpenWorldMapAfterPlayerWin(TSharedPtr<FPresentationBarrier> Barrier);

	/** @brief 승리 후 지도 잠금이 유지되는 동안 월드맵을 다시 연다. */
	void RestoreVictoryWorldMap();

	/** @brief 설정 패널이 열려 있으면 닫힘 완료 뒤 승리 후 월드맵을 복원한다. */
	void CloseSettingsPanelAndRestoreVictoryWorldMap();

	/** @brief 월드맵 닫기 요청을 승리 잠금 상태에 맞게 처리한다. */
	UFUNCTION()
	void HandleWorldMapCloseRequested();

	/** @brief 설정 패널 Back 요청을 승리 잠금 상태에 맞게 처리한다. */
	UFUNCTION()
	void HandleSettingsBackRequested();

	/** @brief 지도 열림 동안 탑바 뒤 월드 비침을 가리는 배경판(시안 빌더가 WBP에 생성, 초기 Collapsed) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TopBar_Backdrop;

	/** @brief 뷰모델의 유닛 수에 맞춰 머리 위 HP바 위젯을 다시 만든다. */
	void RebuildUnitHpBars();

	/** @brief 각 유닛의 월드 위치를 화면에 투영해 HP바 위치/비율/색을 매 프레임 갱신한다. */
	void UpdateUnitHpBars();

	/* ── 전투 플로팅 로그(유닛 머리 위) + 턴 라운드 배너 (CombatTileMapHUDWidget_CombatLog.cpp) ── */

	/** @brief 전투 이벤트 플로팅 로그 요청을 순차 재생 큐에 넣는다(OnCombatFloatingLog 구독). */
	UFUNCTION()
	void HandleCombatFloatingLog(FCombatFloatingLogRequest Request);

	/** @brief 모션 연출 종료 알림을 받아 해당 MotionIndex에 묶인 플로팅 로그를 제거한다. */
	UFUNCTION()
	void HandleCombatFloatingLogMotionFinished(int32 MotionIndex);

	/** @brief 현재 떠 있는(그리고 대기 중인) 플로팅 로그를 전부 즉시 제거한다(OnCombatFloatingLogsCleared 구독). */
	UFUNCTION()
	void HandleCombatFloatingLogsCleared();

	/** @brief 턴 시작 주사위 굴림 요청(OnDiceRollRequested 구독) — 굴림 오버레이를 연다. 구현: _DiceRoll.cpp */
	UFUNCTION()
	void HandleCombatDiceRollRequested();

	/** @brief 대기 중인 플로팅 로그 큐에서 다음 로그를 일정 간격으로 스폰한다. */
	void UpdateFloatingCombatLogQueue(float InDeltaTime);

	/** @brief 플로팅 로그들을 상승+페이드시키고 수명이 다하면 제거한다(매 프레임). */
	void UpdateFloatingCombatLogs(float InDeltaTime);

	/** @brief 월드 좌표 기준으로 플로팅 로그 위젯을 실제 생성한다. */
	void SpawnFloatingCombatLogAtWorld(const FCombatFloatingLogRequest& Request);

	/** @brief 대기/표시 중인 플로팅 로그에서 MotionIndex가 같은 항목을 모두 제거한다. */
	void RemoveFloatingCombatLogsByMotionIndex(int32 MotionIndex);

	/** @brief 라운드가 실제로 바뀌었을 때만 중앙 배너("N번째 턴")를 띄운다. Turn 도메인 갱신 시 호출. */
	void RefreshTurnRoundBanner();

	/** @brief 턴 라운드 배너를 잠시 보여준 뒤 페이드아웃한다(매 프레임). */
	void UpdateTurnRoundBanner(float InDeltaTime);

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

	/** @brief 주사위 연출 안내 문구 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DiceRollStatusText;

	/** @brief 턴 종료 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EndTurnButton;

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

	/** @brief 내비 투명 버튼(concept 아트 위 클릭영역). 런타임 생성, HUD 소유 패널 토글 호출. */
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

	/**
	 * @brief 아직 화면에 안 뜬 "대기 큐"의 한 칸(실행 로그 전용). 스폰 전이라 원본 요청만 들고 있다.
	 * @details 실행 로그가 몰릴 때 여기 쌓였다가 UpdateFloatingCombatLogQueue가 간격을 두고 하나씩 꺼내 스폰한다.
	 *          (미리보기 로그는 큐를 안 타고 즉시 스폰되므로 여기 들어오지 않는다.)
	 */
	struct FQueuedFloatingCombatLogEntry
	{
		FCombatFloatingLogRequest mRequest;   // 스폰 때 그대로 쓸 원본 요청
		int32 mArrivalOrder = 0;              // 같은 Sequence일 때 수신 순서를 지키기 위한 순번
	};

	/**
	 * @brief 지금 화면에 떠 있는(추적 중인) 로그 한 건. 위젯 수명은 RootCanvas가 쥐고, 여긴 위치/수명 추적용 메타만 든다.
	 * @details UpdateFloatingCombatLogs가 매 프레임 이 목록을 돌며 월드→스크린 재배치 + (실행)상승·페이드·소멸을 처리한다.
	 */
	struct FFloatingCombatLogEntry
	{
		TObjectPtr<UWidget> mRoot;                    // 캔버스에 붙은 루트(아이콘+텍스트 박스 또는 텍스트 단일)
		FVector mWorldLocation = FVector::ZeroVector; // 스폰 시점 월드 위치 스냅샷(이 위에 투영해 그린다)
		int32 mMotionIndex = INDEX_NONE;              // 속한 모션 인덱스. 그 모션 종료 시 이 값으로 묶어서 함께 제거
		float mElapsed = 0.0f;                        // 스폰 후 누적 시간(실행 로그의 상승/페이드/수명 판단용)
		bool mIsPreview = false;                      // true면 자동 소멸 안 함(MotionFinished/Clear로만 제거)
		float mStackOffsetY = 0.0f;                   // 미리보기 겹침 방지용 세로 쌓기 오프셋(px, 위로 +)
	};
	/** @brief 스폰 대기 큐(실행 로그). HandleCombatFloatingLog가 넣고, UpdateFloatingCombatLogQueue가 하나씩 꺼낸다. */
	TArray<FQueuedFloatingCombatLogEntry> mPendingFloatingCombatLogs;
	/** @brief 화면에 떠 있는 로그 목록. SpawnFloatingCombatLogAtWorld가 등록, UpdateFloatingCombatLogs가 갱신/제거. */
	TArray<FFloatingCombatLogEntry> mFloatingCombatLogs;

	/** @brief 같은 Sequence일 때 수신 순서를 유지하기 위한 내부 순번. */
	int32 mNextFloatingCombatLogArrivalOrder = 0;

	/** @brief 다음 플로팅 로그를 큐에서 꺼내기까지 남은 시간. */
	float mFloatingCombatLogQueueCooldown = 0.0f;

	/** @brief 턴 라운드 배너 텍스트(중앙 상단, 라운드가 바뀔 때 잠깐 표시) */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mTurnRoundBannerText;

	/** @brief 배너 표시 경과 시간 */
	float mTurnRoundBannerElapsed = 0.0f;

	/** @brief 마지막으로 배너를 띄운 라운드(같은 라운드 내 턴 전환 반복 방지) */
	int32 mLastShownTurnRound = 0;

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

	/** @brief 보유 주사위 카드의 굴림 값 텍스트(2D 면 표시 - 크고 진한 숫자). */
	TArray<TObjectPtr<UTextBlock>> mOwnedDiceValueTexts;

	/** @brief 보유 주사위 2D 면 판 텍스처(카드 공용, 지연 로드 캐시). */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mOwnedDiceFaceTexture;

	/** @brief 중립 상태로 표시하는 런타임 스킬 레일 배경 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mSkillRailPanels;

	/** @brief 중립 상태로 표시하는 런타임 스킬 레일 문구 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mSkillRailTexts;

	/** @brief 보유 스킬 아이콘(FSkillUI.mIcon)을 그리는 슬롯별 Image. 빈 슬롯은 숨긴다. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mSkillRailIcons;

	/** @brief 스킬 레일 입력을 받기 위해 WBP 위에 얹는 투명 버튼들 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UIndexedButtonWidget>> mSkillInputButtons;

	/** @brief 현재 선택된 스킬 index */
	int32 mSelectedSkillIndex = INDEX_NONE;

	/** @brief 승리 후 다음 방을 고르기 전까지 월드맵을 강제 유지하는 잠금. 새 전투 바인딩 시 초기화된다. */
	bool mVictoryWorldMapLocked = false;

	/** @brief WBP에 디자이너 스킨(HUD_* 앵커 위젯)이 있어 좌표/아트를 WBP에서 읽는 모드인지. */
	bool mDesignerSkinActive = false;

	/** @brief 마지막으로 레이아웃 진단 로그를 남긴 뷰포트 크기. 변할 때만 로그해 스팸을 막는다. */
	FVector2D mLastLoggedLayoutViewportSize = FVector2D(-1.0f, -1.0f);

	/** @brief 폴드(좁은 화면비) 변형이 현재 적용 중인지. */
	bool mFoldVariantActive = false;

	/** @brief 폴드 변형 대상 위젯의 베이스 슬롯 캐시(기준 화면비 복원용). */
	TMap<FName, FAnchorData> mAspectVariantBaseSlots;

	/** @brief 폴드에서 턴 칩 줄에 더할 y델타(디자인px, HUD_TurnOrderFold 마커에서 산출). */
	float mFoldTurnOrderDeltaY = 0.0f;

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
