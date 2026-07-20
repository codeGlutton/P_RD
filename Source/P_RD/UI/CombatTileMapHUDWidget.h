#pragma once

#include "RDMinimal.h"
#include "UI/DiceViewData.h"
#include "UI/Combat/CombatUITypes.h"
#include "UI/RDUserWidget.h"
#include "UI/Reward/RewardUITypes.h"
#include "Components/CanvasPanelSlot.h"   // FAnchorData (폴드 변형 베이스 슬롯 캐시)
#include "Engine/TimerHandle.h"           // FTimerHandle (골드 카운트업 타이머)
#include "Styling/SlateBrush.h"
#include "Widgets/Layout/Anchors.h"

#include "CombatTileMapHUDWidget.generated.h"

class ACombatDiceCaptureActor;
class ACombatDiceRollCaptureActor;
class AActor;
struct FPresentationBarrier;
enum class EWorldWidgetType : uint8;
enum class ESRPGCombatResult : uint8;
class UTexture2D;
class UBorder;
class UButton;
class UCanvasPanel;
class UCombatUIModel;
class UCombatResultOverlayWidget;
class UCinematicWidget;
class UHorizontalBox;
class UIndexedButtonWidget;
class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProgressBar;
class URewardUIModel;
class URewardUIWidgetBase;
class USoundBase;
class USkeletalMeshComponent;
class UTextBlock;
class UTextureRenderTarget2D;
class UVerticalBox;
class UViewport;
class UWidget;
class UUserWidget;

/** @brief 첫 전투에서 설명을 먼저 읽지 않고 실제 UI를 순서대로 클릭하게 하는 HUD 코치 단계. */
enum class EEnemyIntentTutorialStage : uint8
{
	WaitingForIntent,
	OpenAttack,
	SelectSlash,
	StrikeTarget,
	ObserveStrike,
	OpenGrip,
	SelectPull,
	ConfirmDestination,
	ApplyingIntervention,
	ObserveResponse,
	Complete
};

/** @brief 행동군 플라이아웃에서 선택한 실제 조작 방식. 동일 스킬 데이터도 전사 행동의 입력 의도를 구분한다. */
enum class ECombatSubactionMode : uint8
{
	None,
	BasicAttack,
	ShortThrow,
	Stagger,
	Pull,
	Throw,
	Swap,
	Move,
	Charge,
	Leap,
	Whirlwind,
	Shockwave
};

/** @brief 유닛 머리 위 HP바(WBP_CombatUnitHpBar) 한 개의 런타임 위젯 참조 묶음. */
USTRUCT()
struct FUnitHpBarWidget
{
	GENERATED_BODY()

	// HUD RootCanvas에 붙는 WBP 인스턴스(월드→스크린 투영으로 위치 갱신 대상).
	UPROPERTY(Transient) TObjectPtr<UUserWidget> mRoot;
	// HP 채움 이미지(플레이어=초록/적=빨강 텍스처 교체 대상).
	UPROPERTY(Transient) TObjectPtr<UImage> mFillImage;
	// 채움을 % 만큼 "크롭"하기 위해 폭을 조절하는 클립 캔버스의 슬롯(런타임 생성).
	UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> mFillClipSlot;
	// HP 숫자 텍스트.
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mValueText;
	// 적 유닛 머리 위에 현재 공개 계획의 실행순서/흐름/결과를 짧게 표시하는 배지.
	UPROPERTY(Transient) TObjectPtr<UBorder> mIntentBadge;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mIntentText;
	// HP바 왼쪽 방어도 아이콘/수치(런타임 생성). 방어도 0이면 숨긴다.
	UPROPERTY(Transient) TObjectPtr<UImage> mDefenseIcon;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mDefenseText;
	// 채움 이미지의 원본(100%) 폭(디자인 값). 크롭 계산 기준.
	float mFillFullWidth = 0.0f;

	// 상태이상 칸(StatusIcon_01~05 / StatusCountText_01~05) + 초과 표시. Rebuild에서 캐시, Update에서 채운다.
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> mStatusIcons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> mStatusCountTexts;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mStatusOverflowText;
};

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
	void NativeOnInitialized() override;
	void OpenUI(FOnEndUIOpenAnimation Callback) override;

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
	FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** @brief PC/에디터에서 스킬 레일 입력을 짧은 선택 또는 롱프레스 상세로 확정한다. */
	FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** @brief 모바일에서 스킬 레일을 누르기 시작한 위치를 기록한다. */
	FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	FReply NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	/** @brief 모바일에서 스킬 레일 입력을 짧은 선택 또는 롱프레스 상세로 확정한다. */
	FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	/** @brief OpenUI()로 표시될 때 주사위 입장 연출을 다시 시작한다. */
	void ApplyOpenUI() override;

	/** @brief 전투 HUD가 공용 팝업(월드맵/설정/패널)보다 뒤에 깔리도록 낮은 ZOrder를 사용한다. */
	int32 GetViewportZOrder() const override;

private:
	/** @brief WBP에 없는 테스트용 전투 HUD 요소를 RootCanvas 위에 런타임으로 붙인다. */
	void EnsureRuntimeWidgets();
	/** @brief 자동 행동 교대 이후 폐기된 우측 이동/턴 종료 버튼과 스킨 아트를 항상 숨긴다. */
	void HideLegacyCommandWidgets() const;

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

	/** @brief 전투 HUD 레이아웃 진단 로그(뷰포트/비율/스킨/HUD_* 그룹 rect). resize·1920x1080 하드코딩 점검용. */
	// 뷰포트 크기가 바뀔 때만 NativeTick에서 호출한다. 타이틀 LayoutMetrics 로그와 같은 목적.
	void LogCombatLayoutMetrics(const FVector2D& ViewportSize) const;

	/** @brief 스킬 레일 Count개 중 Index 항목의 정규화 영역(렌더/히트테스트 공용). */
	FAnchors GetSkillRailItemRect(int32 Index, int32 Count) const;

	/**
	 * @brief 레일 시각 슬롯 -> 스킬 데이터 index 매핑. 미보유 슬롯이면 INDEX_NONE.
	 * @details 레일은 개별 기술 수가 아니라 행동군 네 칸을 고정 배치한다.
	 *          기본 공격 / 손아귀(당기기·던지기·교환) / 방해 / 이동 순서다.
	 */
	int32 GetSkillDataIndexForRailSlot(int32 RailSlotIndex) const;

	/** @brief 입장 굴림용 실제 물리 테이블 캡처 액터를 준비한다. */
	void EnsureDiceRollPhysicsActor();

	/** @brief 입장 굴림용 물리 캡처 액터를 정리한다. */
	void DestroyDiceRollPhysicsActor();

	/** @brief 주사위 캡처 액터를 UI 전용 위치에 생성한다. */
	ACombatDiceCaptureActor* SpawnDiceCaptureActor(int32 GroupIndex, int32 DiceIndex, int32 RenderTargetSize);

	/** @brief 현재 RunPersistData의 보유 주사위 목록을 전투 HUD 표시용 데이터로 변환한다. */
	void RefreshDiceViewsFromRunData();

	/** @brief 선택 강조가 들어가지 않은 중립 스킬 레일을 런타임으로 다시 만든다. */
	void RebuildSkillRailWidgets();

	/** @brief 보유 스킬 스냅샷(FSkillUI) 기준으로 레일 슬롯 3상태(보유/사용불가/빈칸)를 다시 그린다. */
	void RefreshSkillRailWidgets();
	/** @brief 선택한 공격/손아귀/제압/기동 행동군의 세부 행동 플라이아웃을 만든다. */
	void EnsureActionSubmenuWidgets();
	void RefreshActionSubmenuWidgets();
	void OpenActionFamily(int32 RailSlotIndex);
	UFUNCTION()
	void HandleActionSubmenuClicked(int32 SubactionSlotIndex);

	/** @brief 보유 스킬의 표시 이름을 뷰모델에서 읽는다(없으면 빈 텍스트 - 시안 라벨 폴백 없음). */
	FText GetOwnedSkillLabel(int32 SkillIndex) const;

	/** @brief 보유 주사위 3D 캡처 위젯을 현재 주사위 개수에 맞춰 다시 만든다. */
	void RebuildOwnedDiceCards();

	/** @brief WBP 스킬 레일 위에 투명 입력 버튼을 얹어 짧은 탭과 롱프레스를 받는다. */
	void EnsureSkillInputButtons();

	/** @brief 보유 주사위 3D 캡처의 숫자/색/선택 상태를 현재 굴림 상태에 맞게 갱신한다. */
	void RefreshOwnedDiceCards();

	/** @brief 전투 진입 주사위 굴림을 바로 시작하지 않고 터치 대기 상태로 준비한다. */
	void PrepareIntroDiceRoll();

	/** @brief 전투 진입 주사위 굴림 연출을 시작한다. */
	void StartIntroDiceRoll();

	/** @brief 물리 굴림에서 읽은 결과면을 시안/단독 표시 데이터에 반영한다. */
	void ApplyIntroDicePhysicsResults();

	/** @brief 굴림 연출의 현재 프레임을 계산한다. */
	void UpdateIntroDiceRoll(float InDeltaTime);

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

	/** @brief 전장의 유닛을 먼저 고른 뒤 그 유닛에 가능한 행동만 띄우는 컨텍스트 팔레트를 준비한다. */
	void EnsureContextActionWidgets();
	/** @brief 화면 좌표 아래의 유닛을 찾아 컨텍스트 팔레트를 연다. 유닛을 찾았으면 true. */
	bool TryOpenContextActionsAtScreenPosition(const FVector2D& ScreenPosition);
	/** @brief 화면 좌표 아래 생존 유닛과 발밑 투영점을 찾는다. */
	bool FindUnitAtScreenPosition(const FVector2D& ScreenPosition, int32& OutUnitId, bool& OutIsPlayer, FVector2D& OutUnitScreenPosition) const;
	/** @brief 기본 공격과 방해처럼 방향 선택이 없는 행동을 적 한 번 탭으로 즉시 실행한다. */
	bool TryExecuteTapSkillAtScreenPosition(const FVector2D& ScreenPosition);
	/** @brief 기동 행동 중 빈 타일 탭이 레거시 조준으로 새지 않게 하고, 기사 직접 드래그 시작은 통과시킨다. */
	bool TryExecuteTapTileActionAtScreenPosition(const FVector2D& ScreenPosition);
	/** @brief 적의 위치 개입과 기사의 경로 이동을 같은 모바일 직접 드래그 문법으로 처리한다. */
	bool BeginDirectUnitGesture(const FVector2D& ScreenPosition);
	void UpdateDirectUnitGesture(const FVector2D& ScreenPosition);
	bool EndDirectUnitGesture(const FVector2D& ScreenPosition);
	void SetDirectUnitGestureVisual(bool bVisible, const FVector2D& ScreenPosition = FVector2D::ZeroVector);
	/** @brief 전진/돌진을 고른 즉시 전체 사거리 외곽선과 내부 그라데이션을 갱신한다. */
	void RefreshDirectMoveRangeHighlight();
	/** @brief 손가락 아래 타일까지 실제 전진 경로 또는 8방향 돌진선을 다시 만든다. */
	bool UpdateDirectMovePath(const FVector2D& ScreenPosition);
	/** @brief 현재 드래그 경로/충돌 강조만 지우고 전체 사거리 Aim은 필요할 때 유지한다. */
	void ClearDirectMovePreview(bool bClearAim = false);
	/** @brief 화면 좌표 아래 타일 평면의 정확한 터치 월드 위치를 구한다(타일 중심으로 스냅하지 않음). */
	bool GetDirectGestureFloorWorldLocation(
		const FVector2D& ScreenPosition,
		FVector& OutWorldLocation,
		FTileIndex* OutTileIndex = nullptr) const;
	/** @brief 손아귀로 인접 적을 기사 점유 타일에 놓아 자리 교환하려는 드롭인지 판정한다. */
	bool IsDirectGripSwapDestination(const FVector2D& ScreenPosition, FVector& OutPlayerFloorWorld) const;
	/** @brief 대상 유닛 메시를 복제한 충돌 없는 반투명 드래그 고스트를 준비한다. */
	void EnsureDirectUnitGestureGhost(int32 TargetUnitId);
	void DestroyDirectUnitGestureGhost();
	/** @brief 필요한 주사위를 자동으로 골라 기존 스킬 빌드 파이프라인을 즉시 실행한다. */
	bool ExecuteDirectSkill(int32 SkillIndex, const FVector2D& TargetScreenPosition, const FVector2D* DestinationScreenPosition, int32 DesiredPower);
	/** @brief 스킬과 행동 고유 위력을 선택해 실제 게임플레이 사거리 하이라이트를 즉시 켠다. */
	bool SelectSkillWithActionPower(int32 SkillIndex, int32 DesiredPower);
	/** @brief 컨텍스트 행동을 드래그 전에 실제 조준 상태까지 준비한다. */
	bool PrepareDirectSkill(int32 SkillIndex, const FVector2D& TargetScreenPosition, int32 DesiredPower);
	/** @brief 당기기/던지기 드래그 방향을 게임플레이가 계산한 유효 착지 후보에 스냅한다. */
	bool SelectDirectThrowDestination(const FVector2D& TargetScreenPosition, const FVector2D& DestinationScreenPosition);
	int32 FindDirectSkillIndex(bool FSkillUI::* SkillFlag) const;
	/** @brief 선택된 유닛/현재 주사위/사거리에 맞춰 컨텍스트 행동 목록과 강조를 다시 그린다. */
	void RefreshContextActions();
	/** @brief 유닛 이동과 카메라 이동을 따라 컨텍스트 팔레트 위치를 갱신한다. */
	void UpdateContextActionLayout();
	/** @brief 컨텍스트 선택을 닫고 저장된 월드 터치 좌표를 폐기한다. */
	void CloseContextActions();
	/** @brief 컨텍스트 행동 한 칸을 눌렀을 때 기존 스킬/이동 명령으로 연결한다. */
	UFUNCTION()
	void HandleContextActionClicked(int32 ActionSlotIndex);
	/** @brief 요구 주사위가 모두 선택되면 저장한 적 위치를 기존 조준 입력으로 자동 전달한다. */
	void TrySubmitContextTargetWhenReady();
	/** @brief 튜토리얼이 현재 컨텍스트 행동 버튼을 직접 강조할 수 있게 스킬별 버튼을 찾는다. */
	UWidget* FindContextActionButtonForSkill(int32 SkillIndex) const;

	/** @brief 행동 큐 노드 하나가 해소될 때 호출. 전투 피드에 수치/라벨을 표시한다(머리 위 위치는 게임플레이 follow-up). */
	UFUNCTION()
	void HandleCombatQueueNodeResolved(FCombatQueueNode Node);

	/** @brief 뷰모델 도메인 갱신 알림. 메타/유닛/턴이 바뀌면 상단 상태바를 다시 그린다. */
	UFUNCTION()
	void HandleCombatUIChanged(ECombatUIDomain Domain);

	/** @brief 최신 적 행동을 실행 순서/스킬/대상/경로/결과 행으로 다시 그린다. */
	void RefreshEnemyIntentPanel();

	/** @brief 당기기/던지기 프리뷰 카드와 실행/다시 선택 버튼을 현재 뷰모델 상태에 맞춘다. */
	void RefreshDisplacementPreview();
	/** @brief 타일 색에 의존하지 않는 사슬/투척선·방향 화살표·착지 유령·충돌 표식을 투영한다. */
	void UpdateDisplacementPreviewVisuals(float InDeltaTime);
	UFUNCTION()
	void HandleDisplacementConfirmClicked();
	UFUNCTION()
	void HandleDisplacementCancelClicked();

	/** @brief 적 intent 실행순서를 전장 경로와 HUD 배지에서 같이 쓸 색으로 변환한다. */
	static FLinearColor GetEnemyIntentExecutionColor(int32 ExecutionOrder);

	/** @brief 머리 위 intent 배지에 쓸 짧은 행동 흐름/결과 문구를 만든다. */
	static FText GetEnemyIntentWorldLabel(const FEnemyIntentUI& Intent);

	/** @brief 실제 행동군/세부 행동/드래그/적 전체 행동 상태에서 튜토리얼 단계를 맞추고 안내 패널을 갱신한다. */
	void UpdateEnemyIntentTutorial();

	/** @brief 실제 조작 위젯을 감싸는 테두리/화살표와 4단계 진행 표시를 매 프레임 갱신한다. */
	void UpdateEnemyIntentTutorialVisuals(float InDeltaTime);

	/** @brief 현재 튜토리얼 단계에 대응하는 실제 조작 대상(행동군/세부 행동/적/행동 순서판)을 찾는다. */
	UWidget* ResolveEnemyIntentTutorialFocusWidget() const;

	/** @brief 튜토리얼 포커스 테두리와 화살표 조각을 일괄 표시하거나 숨긴다. */
	void SetEnemyIntentTutorialOverlayVisible(bool bVisible) const;

	/** @brief 텍스트 없는 4단계 진행 표시의 완료/현재/대기 색을 갱신한다. */
	void RefreshEnemyIntentTutorialProgress() const;

	/** @brief 완료 안내가 끝나면 코치마크를 닫고 전투 입력 표시를 원상 복구한다. */
	UFUNCTION()
	void HandleEnemyIntentTutorialContinue();

	/** @brief 뷰모델의 플레이어 메타(Lv/HP/Gold)를 상단 상태바 텍스트로 반영한다. */
	void RefreshCombatStatusBar() const;

	/** @brief 디자이너 스킨 시, concept value 칸(HUD_M_lv/hp/gold_value 앵커)에 Lv/HP/Gold 텍스트를 칸 크기에 맞춰 그린다. */
	void RefreshSkinValueLabels() const;

	/** @brief 스크린 좌표가 LV 값 마커(HUD_M_lv_value) 위인지 지오메트리로 판정한다. */
	bool IsScreenPositionOverLevelValue(const FVector2D& ScreenPosition) const;

	/** @brief LV 꾹 누름 동안 표시할 경험치 회색 패널을 LV 마커 아래에 생성한다. */
	void EnsureExpHoldPanel();

	/** @brief 경험치 패널 표시/숨김. 표시 시 현재 경험치 값을 채운다. */
	void SetExpHoldPanelVisible(bool bVisible);

	/** @brief 골드 칸 표시값을 갱신한다. 증가면 코인 사운드와 함께 차르륵 카운트업, 감소/최초는 즉시 반영. */
	// 카운트업 상태는 표시 캐시(mutable)라 const 갱신 경로(RefreshSkinValueLabels)에서 불러도 된다.
	void UpdateGoldValueLabel(int32 NewGold) const;

	/** @brief 카운트업 타이머 틱 — 남은 차이에 비례해 감속하며 목표 골드로 수렴한다. */
	void TickGoldCountUp() const;

	/** @brief HUD_M_gold_value 마커 TextBlock에 골드 숫자를 그린다. */
	void SetGoldValueLabelText(int32 Gold) const;

	/** @brief 장비 슬롯 칩(탑바 좌측 하단)을 뷰모델 장비 뷰로 다시 만든다. */
	void RebuildEquipmentBar();

	/** @brief 스킨 모드: 탑바 레벨 아래에 장착 장비 아이콘(최대 3)을 그린다. 위치·아이콘은 임시(디자이너 조정 예정). */
	void RebuildEquipmentIcons();

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
	 * @brief 탑바 배경판(TopBar_Backdrop)을 현재 지도/전투 HUD 연출 규칙에 맞춰 숨김 상태로 유지한다.
	 *
	 * @details
	 * 월드맵이 열리는 경로가 여러 갈래(내비 토글/승리 강제/복원)라 개별 훅 대신 틱에서 상태를 본다.
	 * 지도 자체가 풀스크린 배경이므로 이 배경판을 켜면 지도 위 상단에 색상 띠가 생긴다.
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

	/** @brief 승패 결과 영상을 열고, 영상 종료 뒤 보상/계속 버튼 흐름으로 이어간다. */
	// 판정 직후 바로가 아니라 짧은 텀(딜레이) 후 StartCombatResultCinematic으로 실제 연출을 시작한다.
	void BeginCombatResultPresentation(TSharedPtr<FPresentationBarrier> Barrier, ESRPGCombatResult Result);

	/** @brief 텀(딜레이) 후 실제 결과 연출 시작 — 징글 재생 + 결과 영상 오픈. */
	void StartCombatResultCinematic();

	/** @brief 결과 영상/오버레이 위젯 인스턴스를 준비한다. */
	void EnsureCombatResultWidgets();

	/** @brief 승패 영상 재생이 끝났을 때 결과별 후속 UI를 연다. */
	void HandleCombatResultVideoFinished(UCinematicWidget* CinematicWidget);

	/** @brief 승리 보상 수령 버튼 처리. */
	UFUNCTION()
	void HandleCombatResultRewardConfirmed();

	/** @brief 보상 행 클릭 지급 요청 처리. */
	UFUNCTION()
	void HandleCombatRewardClaimRequested(ERewardClaimKind ClaimKind, int32 ChoiceIndex);

	/** @brief 패배 계속 버튼 처리. */
	void HandleCombatResultContinueConfirmed();

	/** @brief 결과 영상 위젯을 닫고 후속 콜백을 실행한다. */
	void CloseCombatResultCinematic(FSimpleDelegate Callback);

	/** @brief 결과 영상 표시 중 지도탭과 같은 HUD 숨김 상태를 적용/해제한다. */
	void SetCombatResultViewActive(bool bActive, bool bRestoreCombatControls = true);

	/** @brief 결과 타입에 맞는 mp4 설정 경로를 반환한다. */
	FString GetCombatResultVideoPath(ESRPGCombatResult Result) const;

	/** @brief 승리 후 다음 방 선택이 가능한 상태로 월드맵을 연다(OpenUI 완료 시 barrier 해제). */
	void OpenWorldMapAfterPlayerWin(TSharedPtr<FPresentationBarrier> Barrier);

	/** @brief 승리 후 지도 잠금이 유지되는 동안 월드맵을 다시 연다. */
	void RestoreVictoryWorldMap();

	/** @brief 설정 패널이 열려 있으면 닫힘 완료 뒤 승리 후 월드맵을 복원한다. */
	void CloseSettingsPanelAndRestoreVictoryWorldMap();

	/** @brief 월드맵 닫기 요청을 승리 잠금 상태에 맞게 처리한다. */
	UFUNCTION()
	void HandleWorldMapCloseRequested();

	/** @brief 풀스크린 지도 뷰 동안 탑바를 제외한 전투 컨트롤(스킬레일/주사위/턴순서/이동·턴종료)을 숨김/복원한다. */
	void SetCombatPlayControlsVisible(bool bVisible);

	/** @brief 지도/설정/패널을 닫고 전투로 복귀할 때 숨겨졌던 전투 컨트롤을 복원한다(모든 닫기 경로 공통). */
	void RestoreCombatControlsIfHidden();

	/** @brief 설정 패널 Back 요청을 승리 잠금 상태에 맞게 처리한다. */
	UFUNCTION()
	void HandleSettingsBackRequested();

	/** @brief 지도 열림 동안 탑바 뒤 월드 비침을 가리는 배경판(시안 빌더가 WBP에 생성, 초기 Collapsed) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TopBar_Backdrop;

	UPROPERTY(Transient)
	TObjectPtr<UCinematicWidget> mCombatResultCinematicWidget;

	UPROPERTY(Transient)
	TObjectPtr<UCombatResultOverlayWidget> mCombatResultOverlayWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Reward")
	TSubclassOf<URewardUIWidgetBase> mRewardWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<URewardUIWidgetBase> mCombatRewardWidget;

	UPROPERTY(Transient)
	TObjectPtr<URewardUIModel> mCombatRewardUIModel;

	TSharedPtr<FPresentationBarrier> mCombatResultBarrier;

	/** @brief 승패 판정 → 결과 영상 시작 사이의 텀 타이머. */
	FTimerHandle mCombatResultStartDelayTimerHandle;
	ESRPGCombatResult mCombatResult = ESRPGCombatResult::PlayerLose;
	bool mCombatResultFlowActive = false;

	/** @brief 뷰모델의 유닛 수에 맞춰 머리 위 HP바 위젯을 다시 만든다. */
	void RebuildUnitHpBars();

	/** @brief 각 유닛의 월드 위치를 화면에 투영해 HP바 위치/비율/색을 매 프레임 갱신한다. */
	void UpdateUnitHpBars();

	/** @brief HpFillImage를 ClipToBounds 캔버스로 감싸, 폭을 % 만큼 줄이면 우측이 잘리는 크롭 채움을 만든다. */
	void SetupUnitHpBarFillClip(FUnitHpBarWidget& Bar);

	/** @brief WBP의 상태 슬롯(StatusIcon_0N/StatusCountText_0N/Overflow)을 캐시하고 숨김으로 둔다. */
	void CacheUnitHpBarStatusSlots(FUnitHpBarWidget& Bar) const;

	/** @brief 유닛 HP바 밑 상태이상 칸을 DTO(mStatusEffects)로 채운다(아이콘/스택수/오버플로). UpdateUnitHpBars에서 매 프레임 호출. */
	void UpdateUnitHpBarStatus(FUnitHpBarWidget& Bar, const FUnitUI& Unit) const;

	/** @brief 상태이상 태그 → 아이콘 텍스처(로그용 mLogIcon* 재사용). 전용 아이콘 없는 태그는 nullptr. */
	UTexture2D* ResolveStatusIcon(const FGameplayTag& StatusTag) const;

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

	/** @brief 턴 전환 영상 안내를 재생하고, 필요하면 종료 뒤 주사위 팝업을 연다. */
	bool PlayTurnChangeIntro(bool bOpenDiceAfterIntro);

	/** @brief 턴 전환 영상 안내를 닫고 대기 중인 후속 UI(주사위)를 이어간다. */
	void FinishTurnChangeIntro();

	/** @brief 턴 전환 알파 텍스처 에셋 프레임들을 준비한다. */
	bool EnsureTurnChangeFrameTextures();

	/** @brief 턴 전환 프레임 텍스처를 전투 진입 시 비동기 프리로드한다(첫 배너 동기 로드 히치 제거). */
	void PreloadTurnChangeFrameTextures();

	/** @brief 턴 전환 영상 안내 위젯들을 보이거나 숨긴다. */
	void SetTurnChangeIntroVisibility(bool bVisible) const;

	/** @brief 중앙 원에 표시할 현재 턴 수를 계산한다. */
	int32 GetTurnChangeDisplayNumber() const;

	/** @brief 턴 전환 텍스처 에셋의 /Game 패키지 경로를 만든다. */
	FString ResolveTurnChangeFrameAssetPath(int32 FrameIndex) const;

	/** @brief 턴 전환 텍스처 에셋을 로드한다. */
	UTexture2D* LoadTurnChangeFrameTexture(const FString& FrameAssetPath) const;

	/** @brief 현재 경과 시간에 맞춰 턴 전환 프레임을 넘긴다. */
	void UpdateTurnChangeIntro(float InDeltaTime);

	/** @brief 지정한 턴 전환 프레임을 Image Brush에 반영한다. */
	void ApplyTurnChangeFrame(int32 FrameIndex);

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

	/** @brief 장비 슬롯 투명 버튼이 눌렸을 때 공통 누름 상태로 진입한다(스킬과 동일 방식). */
	UFUNCTION()
	void HandleEquipButtonPressed(int32 SlotIndex);

	/** @brief 장비 슬롯 입력 해제 시 누름 상태를 리셋한다. */
	UFUNCTION()
	void HandleEquipButtonReleased();

	/** @brief 장비 슬롯 누름을 시작한다(오래 누르면 상세). */
	void BeginEquipPress(int32 SlotIndex);

	/** @brief 장비 슬롯 누름 시간을 누적해 롱프레스 상세 표시를 판단한다. */
	void UpdateEquipPress(float InDeltaTime);

	/** @brief 스킬을 선택한다. 다른 스킬로 바뀌면 주사위 배치 후보를 초기화한다. */
	void SelectSkillForAssignment(int32 SkillIndex);

	/** @brief 스킬 상세(concept_09 오버레이)를 표시한다. */
	void ShowSkillDetail(int32 SkillIndex);

	/** @brief 유닛 상세(concept_09 오버레이)를 표시한다. */
	void ShowUnitDetail(int32 UnitId);

	/** @brief 장비 상세(concept_09 오버레이)를 표시한다. */
	void ShowEquipmentDetail(int32 SlotIndex);

	/** @brief concept_09 상세 오버레이 요소(스크림/프레임/아이콘/이름/타입/설명/닫기)의 표시 상태를 일괄 설정한다. */
	void SetDetailOverlayVisible(bool bVisible) const;

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

	/** @brief 유닛 HP바 왼쪽 방어도 아이콘 텍스처(생성자 프리로드). */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mUnitDefenseIconTexture;

	/** @brief 골드 획득 코인 사운드(카운트업 시작 시 1회). 생성자에서 하드레퍼런스 프리로드(#300 컨벤션). */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> mCoinGainSound;

	/** @brief 승리/패배 결과 징글. 결과 연출 시작과 동시에 1회 재생한다. */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> mVictoryJingleSound;
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> mDefeatJingleSound;

	/** @brief 지도 열기 사운드(책장 넘김). 월드맵이 실제로 열릴 때 1회. */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> mMapOpenSound;

	/** @brief 스킬-주사위 배치에서 주사위를 고를 때 나는 나무 주사위 사운드. */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> mDiceChooseSound;

	/** @brief 스킬 레일에서 유효한 스킬을 선택할 때 재생하는 사운드. */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> mSkillSelectSound;

	/** @brief 보상에서 경험치를 받았을 때 상승음. */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> mExpGainSound;

	/** @brief LV 칸을 누르는 중인가 — true인 동안 LV 아래 경험치 패널을 표시한다. */
	bool mLevelValueTouched = false;

	/** @brief LV 꾹 누름 동안 표시되는 경험치 회색 패널과 텍스트. */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> mExpHoldPanel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mExpHoldText;

	// 골드 카운트업 상태 3종은 '화면 표시 캐시'라 mutable — const 그리기 경로에서 갱신해도 논리 상태 불변.
	/** @brief 골드 칸에 현재 표시 중인 값. INDEX_NONE이면 아직 최초 표시 전(카운트업 없이 즉시 그림). */
	mutable int32 mDisplayedGold = INDEX_NONE;

	/** @brief 카운트업이 수렴할 목표 골드. */
	mutable int32 mGoldCountUpTarget = 0;

	/** @brief 골드 카운트업 반복 타이머. */
	mutable FTimerHandle mGoldCountUpTimerHandle;

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

	/** @brief 기존 전투 화면 우상단에 얹는 비모달 적 행동 예고 패널과 행 컨테이너. */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> mEnemyIntentPanel;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> mEnemyIntentList;

	/** @brief 위치 개입 프리뷰의 명시적 확정 카드. 같은 타일 재클릭을 대체한다. */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> mDisplacementConfirmPanel;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> mDisplacementConfirmContent;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mDisplacementConfirmTitle;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mDisplacementConfirmSummary;
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> mDisplacementConfirmButtons;
	UPROPERTY(Transient)
	TObjectPtr<UButton> mDisplacementCancelButton;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mDisplacementCancelText;
	UPROPERTY(Transient)
	TObjectPtr<UButton> mDisplacementExecuteButton;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mDisplacementExecuteText;

	/** @brief 월드 위에 투영하는 굵은 궤적, 8방향 화살표, 대상/착지/충돌 표식. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mDisplacementPathSegments;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mDisplacementDirectionLabels;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mDisplacementTargetLabel;
	UPROPERTY(Transient)
	TObjectPtr<UImage> mDisplacementLandingGhost;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mDisplacementLandingLabel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mDisplacementCollisionLabel;
	/** @brief 드롭 이후 적 전원의 예상 위치(반투명 초상)와 공격 위험 타일. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mResponseGhostImages;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mResponseThreatLabels;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mResponseSummaryLabel;

	/** @brief 화면 상단의 짧은 2줄 설명과 4단계 진행 표시. 전투 입력을 막지 않는다. */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> mEnemyIntentTutorialPanel;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> mEnemyIntentTutorialContent;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mEnemyIntentTutorialTitle;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mEnemyIntentTutorialText;
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> mEnemyIntentTutorialFlow;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mEnemyIntentTutorialFlowCards;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mEnemyIntentTutorialFlowTexts;
	UPROPERTY(Transient)
	TObjectPtr<UButton> mEnemyIntentTutorialContinueButton;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mEnemyIntentTutorialContinueText;
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> mEnemyIntentTutorialProgress;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mEnemyIntentTutorialProgressDots;

	/** @brief RootCanvas 위에 놓여 실제 조작 대상을 감싸는 4개 테두리와 3개 화살표 선분. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mEnemyIntentTutorialFocusEdges;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mEnemyIntentTutorialArrowParts;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mEnemyIntentTutorialDimPanels;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mEnemyIntentTutorialPointerLabel;

	EEnemyIntentTutorialStage mEnemyIntentTutorialStage = EEnemyIntentTutorialStage::WaitingForIntent;
	float mEnemyIntentTutorialStageElapsed = 0.0f;
	float mEnemyIntentTutorialPulseTime = 0.0f;
	bool mEnemyIntentTutorialDismissed = false;
	bool mEnemyIntentTutorialStrikeSubmitted = false;
	bool mEnemyIntentTutorialSawStrikeEnemyTurn = false;
	bool mEnemyIntentTutorialStrikeCycleComplete = false;
	bool mEnemyIntentTutorialInterventionSubmitted = false;
	bool mEnemyIntentTutorialPullCompleted = false;
	bool mEnemyIntentTutorialSawEnemyTurn = false;
	int32 mEnemyIntentTutorialIntervenedEnemyUnitId = INDEX_NONE;

	/** @brief 장비 칩(탑바 좌측 하단) 배경/문구 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mEquipmentChips;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mEquipmentChipTexts;

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

	/** @brief 유닛 머리 위에 월드→스크린 투영으로 띄우는 HP바(WBP_CombatUnitHpBar 인스턴스, 유닛 뷰 순서와 1:1) */
	UPROPERTY(Transient)
	TArray<FUnitHpBarWidget> mUnitHpBars;

	/** @brief HP바 WBP 클래스/채움 텍스처(빨강=적, 초록=아군) 지연 로드 캐시 */
	UPROPERTY(Transient)
	TSubclassOf<UUserWidget> mUnitHpBarWidgetClass;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mUnitHpFillRedTexture;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mUnitHpFillGreenTexture;

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
		int32 mTurnIndex = INDEX_NONE;                // 요청이 속한 턴 인덱스. 후속 UI 연출에서 사용할 수 있도록 보존
		int32 mActionIndex = INDEX_NONE;              // 요청이 속한 액션 인덱스. 후속 UI 연출에서 사용할 수 있도록 보존
		int32 mMotionIndex = INDEX_NONE;              // 요청이 속한 모션 인덱스. 기존 모션 종료 처리에도 사용
		float mElapsed = 0.0f;                        // 스폰 후 누적 시간(실행 로그의 상승/페이드/수명 판단용)
		bool mIsPreview = false;                      // true면 자동 소멸 안 함(MotionFinished/Clear로만 제거)
		float mStackOffsetY = 0.0f;                   // 미리보기 겹침 방지용 세로 쌓기 오프셋(px, 위로 +)
		bool mIsDismissing = false;                   // 모션 종료로 퇴장 중 — 오른쪽으로 흐르며 페이드아웃 후 제거
		float mDismissElapsed = 0.0f;                 // 퇴장 연출 누적 시간
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

	/** @brief 턴 전환 영상 안내의 풀스크린 딤 배경 */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> mTurnChangeBackdropPanel;

	/** @brief 턴 전환 알파 텍스처 에셋 프레임을 그리는 Image */
	UPROPERTY(Transient)
	TObjectPtr<UImage> mTurnChangeVideoImage;

	/** @brief 턴 전환 안내 중 입력이 전투 HUD로 새지 않게 막는 투명 레이어 */
	UPROPERTY(Transient)
	TObjectPtr<UButton> mTurnChangeInputBlocker;

	/** @brief 영상 중앙 원 안에 턴 수를 정렬하는 투명 패널 */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> mTurnChangeTurnTextPanel;

	/** @brief 영상 중앙 원 안에 표시할 턴 수 텍스트 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mTurnChangeTurnText;

	/** @brief 턴 전환 알파 텍스처 에셋 프레임 캐시 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> mTurnChangeFrameTextures;

	/** @brief 턴 전환 프레임 비동기 프리로드 핸들(로드된 에셋을 배너 사용 전까지 붙잡는 핀). */
	TSharedPtr<struct FStreamableHandle> mTurnChangeFramePreloadHandle;

	/** @brief mTurnChangeVideoImage에 물릴 현재 텍스처 프레임 브러시 */
	FSlateBrush mTurnChangeFrameBrush;

	/** @brief 턴 전환 안내 누적 시간 */
	float mTurnChangeIntroElapsed = 0.0f;

	/** @brief 현재 표시 중인 턴 전환 프레임 index */
	int32 mTurnChangeCurrentFrameIndex = INDEX_NONE;

	/** @brief 턴 전환 안내가 현재 재생 중인지 여부 */
	bool mTurnChangeIntroPlaying = false;

	/**
	 * @brief 턴 전환 배너 강제 종료 보험 타이머.
	 * @details 정상 종료(FinishTurnChangeIntro)는 NativeTick의 경과시간 판정에만 의존하는데,
	 *          배너 재생 중 HUD Tick이 멈추면(위젯 숨김 등) 라운드 배리어(mRoundChangeBarrier)가
	 *          영영 안 풀려 그 라운드가 진행 불능이 된다. 재생시간+여유가 지나면 무조건 종료시킨다.
	 */
	FTimerHandle mTurnChangeSafetyTimerHandle;

	/** @brief 턴 전환 안내 종료 직후 주사위 팝업을 열어야 하는지 여부 */
	bool mPendingDiceRollAfterTurnIntro = false;

	/** @brief 라운드 시작 배너를 재생하는 동안 잡아두는 배리어 — 배너가 끝나면(FinishTurnChangeIntro) 놓아 그 라운드 첫 턴을 진행시킨다. */
	TSharedPtr<FPresentationBarrier> mRoundChangeBarrier;

	/** @brief 배너 표시 경과 시간 */
	float mTurnRoundBannerElapsed = 0.0f;

	/** @brief 마지막으로 배너를 띄운 라운드(같은 라운드 내 턴 전환 반복 방지) */
	int32 mLastShownTurnRound = 0;

	/** @brief 보유 주사위 3D 캡처 RenderTarget Image 위젯 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mOwnedDiceImages;

	/**
	 * @brief 보유 주사위 카드 캡처를 전담하는 공유 캡처 액터(항상 1대).
	 * @details 다이별로 SceneCapture+라이트 액터를 상주시키면 보유 개수만큼 무한 누적되므로,
	 *          카메라는 1대만 두고 다이를 순서대로 구성해 다이별 RT(mOwnedDiceRenderTargets)에 찍는다.
	 */
	UPROPERTY(Transient)
	TObjectPtr<ACombatDiceCaptureActor> mOwnedDiceSharedCaptureActor;

	/** @brief 다이별 표시용 투명 RenderTarget(256x256). 다이별 상주는 이것과 캡처 머티리얼뿐이다. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextureRenderTarget2D>> mOwnedDiceRenderTargets;

	/** @brief 다이별 캡처 머티리얼(RT 알파→UI 투명도). 카드 Image 브러시가 참조한다. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> mOwnedDiceCaptureMaterials;

	/** @brief 변경되지 않은 보유 주사위는 3D 장면을 다시 캡처하지 않기 위한 시각 상태 해시. */
	TArray<uint32> mOwnedDiceVisualHashes;

	/** @brief 보유 주사위 선택 입력을 받는 투명 버튼 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UIndexedButtonWidget>> mOwnedDiceCardWidgets;

	/** @brief 각 주사위 아래에 종류와 선택/사용 상태를 표시하는 짧은 라벨. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mOwnedDiceTypeTexts;

	/** @brief 보유 주사위를 스킬과 분리해 보여주는 하단 트레이 배경/제목. */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> mDiceTrayPanel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mDiceTrayTitleText;

	/** @brief 선택한 유닛 옆에 뜨는 작은 행동 팔레트. 버튼 index는 mContextActionSkillIndices의 슬롯이다. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mContextActionTitleText;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mContextActionPanels;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mContextActionIcons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mContextActionTexts;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UIndexedButtonWidget>> mContextActionButtons;
	/** @brief 팔레트 슬롯 -> 실제 FSkillUI index. -2는 MOVE 명령이다. */
	TArray<int32> mContextActionSkillIndices;
	/** @brief 현재 팔레트가 가리키는 유닛과 그 유닛을 실제로 눌렀던 Slate 절대좌표. */
	int32 mContextTargetUnitId = INDEX_NONE;
	bool mContextTargetIsPlayer = false;
	FVector2D mContextTargetScreenPosition = FVector2D::ZeroVector;
	/** @brief 같은 스킬/주사위 갱신 알림에서 월드 조준을 두 번 보내지 않는 latch. */
	int32 mContextSelectedSkillIndex = INDEX_NONE;
	bool mContextTargetSubmitted = false;
	/** @brief 팔레트에서 먼저 고른 뒤 같은 유닛을 드래그할 때만 실행되는 행동. -2는 이동이다. */
	int32 mDirectArmedSkillIndex = INDEX_NONE;
	int32 mDirectArmedTargetUnitId = INDEX_NONE;

	/** @brief 전장 유닛을 누른 채 끌어 행동을 고르는 모바일 직접 조작 상태. */
	bool mDirectUnitGestureActive = false;
	bool mDirectUnitGestureDragged = false;
	int32 mDirectUnitGestureTargetId = INDEX_NONE;
	bool mDirectUnitGestureTargetIsPlayer = false;
	FVector2D mDirectUnitGestureStart = FVector2D::ZeroVector;
	FVector2D mDirectUnitGestureCurrent = FVector2D::ZeroVector;
	FVector2D mDirectUnitGestureTargetScreen = FVector2D::ZeroVector;
	bool mHasDirectThrowCandidate = false;
	FVector mDirectThrowCandidateWorld = FVector::ZeroVector;
	/** @brief 좌측 손아귀 행동군에서 시작된 제스처와 기사 타일 교환 프리뷰 상태. */
	bool mDirectGripGesture = false;
	bool mDirectGripCanSwap = false;
	bool mDirectGripSwapPreview = false;
	/** @brief 기사 드래그 이동의 현재 확정 후보. 첫 원소는 기사 출발 칸이다. */
	TArray<FTileIndex> mDirectMovePath;
	FTileIndex mDirectMoveLandingTile = FTileIndex::Invalid;
	FTileIndex mDirectMoveImpactTile = FTileIndex::Invalid;
	FText mDirectMoveImpactLabel;
	bool mDirectMoveIsCharge = false;
	bool mDirectMoveIsLeap = false;
	int32 mDirectGestureGhostTargetId = INDEX_NONE;
	UPROPERTY(Transient) TObjectPtr<UBorder> mDirectUnitGestureLine;
	UPROPERTY(Transient) TObjectPtr<UBorder> mDirectUnitGestureHandle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mDirectUnitGestureLabel;
	UPROPERTY(Transient) TObjectPtr<AActor> mDirectUnitGestureGhostActor;
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> mDirectUnitGestureGhostMesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> mDirectUnitGestureGhostMaterial;

	/** @brief 카드형 스킬 레일 전체를 묶는 좌측 도크 배경/제목. */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> mSkillDockPanel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mSkillDockTitleText;

	/** @brief 중립 상태로 표시하는 런타임 스킬 카드 배경 */
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

	/** @brief 행동군을 누르면 레일 오른쪽에 펼쳐지는 실제 전사 행동 카드. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mActionSubmenuTitleText;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mActionSubmenuPanels;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mActionSubmenuIcons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mActionSubmenuTexts;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UIndexedButtonWidget>> mActionSubmenuButtons;
	TArray<int32> mActionSubmenuSkillIndices;
	TArray<int32> mActionSubmenuDesiredPowers;
	TArray<ECombatSubactionMode> mActionSubmenuModes;
	TArray<FText> mActionSubmenuNames;
	int32 mExpandedActionFamily = INDEX_NONE;
	int32 mActiveActionFamily = INDEX_NONE;
	ECombatSubactionMode mSelectedSubactionMode = ECombatSubactionMode::None;
	FText mSelectedSubactionName;
	int32 mSelectedSubactionDesiredPower = 6;

	/** @brief 현재 선택된 스킬 index */
	int32 mSelectedSkillIndex = INDEX_NONE;

	/** @brief 승리 후 다음 방을 고르기 전까지 월드맵을 강제 유지하는 잠금. 새 전투 바인딩 시 초기화된다. */
	bool mVictoryWorldMapLocked = false;

	/** @brief WBP에 디자이너 스킨(HUD_* 앵커 위젯)이 있어 좌표/아트를 WBP에서 읽는 모드인지. */
	bool mDesignerSkinActive = false;

	/** @brief 마지막으로 레이아웃 진단 로그를 남긴 뷰포트 크기. 변할 때만 로그해 스팸을 막는다. */
	FVector2D mLastLoggedLayoutViewportSize = FVector2D(-1.0f, -1.0f);

	/** @brief 지금 누르고 있는 스킬 index */
	int32 mPressedSkillIndex = INDEX_NONE;
	/** @brief 지금 누르고 있는 네 행동군 레일 슬롯. */
	int32 mPressedSkillRailIndex = INDEX_NONE;

	/** @brief 현재 프레임에서 스킬 레일 입력을 추적 중인지 여부 */
	bool mSkillPressing = false;

	/** @brief 같은 누름에서 탭 선택과 롱프레스 상세가 동시에 발생하지 않게 막는 latch */
	bool mSkillDetailOpenedFromPress = false;

	/** @brief 현재 스킬 누름의 누적 시간. NativeTick에서만 증가해 입력 이벤트 순서에 덜 민감하게 둔다. */
	float mSkillPressElapsed = 0.0f;

	/** @brief 스킬 상세를 열기 위해 필요한 누름 시간. [합의필요] 모바일 손맛 기준으로 확정되면 설정화 대상. */
	float mSkillLongPressThreshold = 0.45f;

	/** @brief 풀스크린 지도 뷰가 열려 전투 컨트롤을 숨긴 상태. */
	bool mCombatControlsHidden = false;

	/** @brief 장비 슬롯 위에 얹는 투명 롱프레스 감지 버튼(마커 HUD_M_equip_0..2 위치). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<class UIndexedButtonWidget>> mEquipSlotButtons;

	/** @brief 지금 누르고 있는 장비 슬롯 index(스킬과 동일한 press 누적 방식). */
	int32 mPressedEquipSlot = INDEX_NONE;

	/** @brief 현재 장비 슬롯 입력을 추적 중인지. */
	bool mEquipPressing = false;

	/** @brief 같은 누름에서 롱프레스 상세가 중복 발생하지 않게 막는 latch. */
	bool mEquipDetailOpenedFromPress = false;

	/** @brief 현재 장비 누름 누적 시간(NativeTick에서 증가). */
	float mEquipPressElapsed = 0.0f;

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
	float mIntroDiceAlignDelay = 0.12f;

	/** @brief 정렬 시작 전 대기 누적 시간. */
	float mIntroDiceAlignTimer = 0.0f;

	/** @brief 주사위가 한 줄로 정렬되는 애니메이션 진행 중인지. */
	bool mIntroDiceAligning = false;

	/** @brief 정렬이 끝나 탭으로 닫기를 기다리는 상태인지. */
	bool mIntroDiceAligned = false;

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

	/** @brief 스킬 상세가 열린 동안 바깥 터치를 받는 투명 버튼(= concept_09 닫기 히트영역) */
	UPROPERTY(Transient)
	TObjectPtr<UButton> mSkillDetailDismissButton;

	/* 공용 상세창 — 디자이너 WBP(WBP_CombatDetailOverlay)를 인스턴스화해 named widget에 데이터만 채운다.
	 * 스킬/유닛/장비 상세가 같은 오버레이를 공유한다. 레이아웃/아트는 WBP가 소유, C++은 내용만 채운다. */

	/** @brief WBP_CombatDetailOverlay 인스턴스(전체 화면). */
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> mDetailOverlay;

	/** @brief 상세 아이콘(스킬 아이콘 / 유닛 초상화 / 장비 아이콘) — WBP의 DetailIconImage. */
	UPROPERTY(Transient)
	TObjectPtr<class UImage> mDetailIconImage;

	/** @brief 상세 제목(이름) — WBP의 DetailTitleText. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mDetailTitleText;

	/** @brief 상세 부제(타입/메타: 주사위·레벨·희귀도 등) — WBP의 DetailSubtitleText. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mDetailSubtitleText;

	/** @brief 상세 본문(설명/패시브/키워드) — WBP의 DetailBodyText. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mDetailBodyText;

	/** @brief WBP_CombatDetailOverlay 클래스 레퍼런스. 생성자에서 로드. */
	UPROPERTY(Transient)
	TSubclassOf<UUserWidget> mDetailOverlayClass;

	/** @brief 플로팅 로그 종류별 아이콘 텍스처(생성자에서 로드). HP는 피해/회복 색으로 갈린다. */
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconHpDamage;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconHpRecovery;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconGetMove;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconGetDefense;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconAgility;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconFortification;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconVulnerability;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconWeakness;

private:
	/** @brief 로그 의미(아이콘 종류+색) → 실제 텍스처. HP는 색(Damage/Heal)으로 피해/회복 아이콘을 가른다. 미준비 종류는 nullptr. */
	UTexture2D* ResolveFloatingLogIcon(EFloatingLogIconType IconType, EFloatingLogColorType ColorType) const;
};
