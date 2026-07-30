#pragma once

/**
 * @brief 배치안 평가용 전투 HUD.
 *
 * @details
 * 전투 UI를 새로 만들면서 배치안 여러 개를 실제로 돌려보고 고르기 위한 위젯이다.
 * 계약(Source/P_RD/UI/Combat/UI_API_CONTRACT.md)이 규정한 방식 그대로,
 * UCombatUIWidgetBase를 상속해 UCombatUIModel 하나만 보고 그린다. 게임플레이를
 * 직접 참조하지 않고, 탭은 Request*()로 의도만 보낸다.
 *
 * **배치를 WBP가 소유**한다. 위젯은 전부 이름으로 찾고, 없으면 그냥 건너뛴다.
 * 그래서 배치안마다 WBP만 새로 만들면 되고, 어떤 안이 어떤 요소를 빼도 된다.
 *
 * 머리 위 HP 바·플로팅 로그·라운드 배너처럼 월드 자리를 따라가야 하는 것만
 * C++가 루트 캔버스에 직접 짓는다. 그것들은 WBP 에 미리 놓을 수가 없다 --
 * 개수가 유닛 수에 따라 변한다.
 *
 * 한 배치안 = WBP 하나. BP_CombatGameMode에서 어느 WBP를 쓸지만 바꾼다.
 */

#include "RDMinimal.h"
#include "UI/Combat/CombatUIWidgetBase.h"

#include "UI/Combat/CombatUITypes.h"

#include "CombatLayoutHUDWidget.generated.h"

struct FPresentationBarrier;

/**
 * @brief 유닛 머리 위 HP 바 한 개가 들고 있는 위젯들.
 *
 * @details
 * 옛 HUD 의 FUnitHpBarWidget 을 그대로 옮겼다. 이름을 바꾼 것은 옛 헤더가
 * 지워지기 전까지 같은 모듈에 USTRUCT 두 개가 같은 이름으로 있을 수 없기
 * 때문이다.
 */
USTRUCT()
struct FCombatUnitHpBarWidget
{
	GENERATED_BODY()

	/** @brief 루트 캔버스에 붙는 WBP 알맹이. 월드 자리를 화면으로 옮겨 붙인다. */
	UPROPERTY(Transient) TObjectPtr<UUserWidget> mRoot;
	/** @brief 채움 그림. 아군은 초록, 적은 빨강으로 갈아 끼운다. */
	UPROPERTY(Transient) TObjectPtr<class UImage> mFillImage;
	/** @brief 채움을 비율만큼 잘라 내려고 폭을 줄이는 칸. */
	UPROPERTY(Transient) TObjectPtr<class UCanvasPanelSlot> mFillClipSlot;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> mValueText;
	/** @brief 방어도 아이콘과 수치. 0 이면 감춘다. */
	UPROPERTY(Transient) TObjectPtr<class UImage> mDefenseIcon;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> mDefenseText;
	/** @brief 다 찼을 때의 폭. 자를 기준이 된다. */
	float mFillFullWidth = 0.0f;

	UPROPERTY(Transient) TArray<TObjectPtr<class UImage>> mStatusIcons;
	UPROPERTY(Transient) TArray<TObjectPtr<class UTextBlock>> mStatusCountTexts;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> mStatusOverflowText;
};
enum class ERewardClaimKind : uint8;

class UButton;
class UMockCombatDriver;
class UImage;
class UProgressBar;
class UTextBlock;
class UWidget;

UCLASS()
class P_RD_API UCombatLayoutHUDWidget : public UCombatUIWidgetBase
{
	GENERATED_BODY()

public:
	UCombatLayoutHUDWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * @brief 열면서 화면 전체를 덮지 않게 한다.
	 *
	 * @details
	 * URDUserWidget::ApplyOpenUI() 가 열 때마다 Visible 로 세운다. 그러면 이
	 * 위젯이 화면 전체를 덮는 한 장이 되어, 버튼이 아닌 곳의 클릭까지 전부
	 * 받아 버린다. 받은 클릭은 플레이어 컨트롤러까지 안 내려가고, 지도는 그
	 * 클릭을 영영 못 본다.
	 *
	 * 열고 나서 표시 상태를 확정한다.
	 */
	virtual void OpenUI(FOnEndUIOpenAnimation Callback = FOnEndUIOpenAnimation()) override;

	/** @brief 도메인 갱신 말고 행동 표현 알림도 같이 듣는다. */
	virtual void BindUIModel(UCombatUIModel* InUIModel) override;
	virtual void UnbindUIModel() override;

	/** @brief 판 탭 알림까지 같이 구독한다. */
	/** @brief 이 배치안이 화면에 표시할 파티 인원. 기획상 최대 3명. */
	static constexpr int32 PartySlotCount = 3;

	/** @brief 턴 순서에 표시할 최대 인원. */
	static constexpr int32 TurnSlotCount = 6;

	/** @brief 커맨드 레일 칸 수. 이동 + 기본공격 + 스킬 4개. */
	static constexpr int32 CommandSlotCount = 6;

	/** @brief 누른 동안 줄어드는 정도. 더 줄이면 눌린 게 아니라 튄 것으로 보인다. */
	static constexpr float PressedScale = 0.95f;


public:
	/**
	 * @brief 게임플레이가 안 붙었을 때 가짜 전투 상태로 그린다.
	 *
	 * @details
	 * 배치안 10개는 서로 비교하려고 만드는 것이라 같은 전투 장면을 그려야 한다.
	 * 실제 전투를 띄우면 진행 상황에 따라 화면이 달라져 비교가 안 되고, 아직
	 * 게임플레이가 UIModel을 채우지도 않는다. 그래서 위젯이 스스로 UIModel과
	 * UMockCombatDriver를 만들어 고정된 장면을 세운다.
	 *
	 * 실제 전투에 연결되는 순간(BindUIModel이 먼저 불린 경우) 이 경로는 건너뛴다.
	 * 편집기에서 WBP 를 열었을 때와 찍는 시험이 이것으로 화면을 채운다 --
	 * 게임모드가 없는 자리라 이걸 끄면 빈 HUD 가 나온다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Layout")
	bool mUsePreviewData = true;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnUIRefreshed(ECombatUIDomain Domain) override;



private:
	/** @brief WBP에서 이름으로 위젯을 찾아 캐시한다. 없는 것은 null로 둔다. */
	void CacheAuthoredWidgets();

	/** @brief 버튼 클릭을 UIModel의 Request*로 연결한다. */
	void WireCommands();

	/** @brief 붙은 UIModel이 없을 때 미리보기 전투 장면을 세운다. */
	void StartPreviewIfUnbound();

	void RefreshParty();
	void RefreshTurnOrder();
	void RefreshCommands();
	void RefreshEnemy();
	void RefreshMeta();

	/**
	 * @brief 명령 카드를 펴거나 접는다.
	 *
	 * 게임 상태가 아니라 화면 상태다. UIModel 로 안 보낸다 -- 게임플레이는
	 * 카드가 보이는지 알 필요가 없다.
	 */
	void SetCommandsShown(bool bShown);

	/**
	 * @brief 카드를 실제로 켜고 끈다.
	 *
	 * 보이는 조건이 둘이다. 사용자가 펴 두었고(mCommandsShown), **조준 중이
	 * 아닐 것.** 조준에 들어가면 카드는 저절로 비켜야 한다 -- 사거리가 칠해진
	 * 판을 봐야 하는데 카드가 그 판 한가운데를 덮고 있다.
	 *
	 * 조준이 끝나면 저절로 돌아온다. 돌아오라고 따로 시키는 곳이 없다.
	 */
	void RefreshCommandVisibility();

	/** @brief 지금 조준 중인가. 조준 중에는 카드가 비켜 있다. */
	bool IsAiming() const;

	/** @brief 지금 차례가 아군인가. 적 차례에는 카드를 안 보여 준다. */
	bool IsPlayerTurn() const;

	/** @brief 지금 차례인 유닛. 없으면 nullptr. */
	const FUnitUI* FindTurnUnit() const;

	/**
	 * @brief 행동이 노는 동안 카드를 접어 둔다.
	 *
	 * @details
	 * 걸어가는 중에 카드가 도로 떠서, 아직 안 끝난 것을 끝난 것처럼 보였다.
	 * 행동은 표현이 다 끝난 뒤에야 끝났다고 알려 온다 -- 이동은 마지막 칸에
	 * 도착한 뒤에 MarkActionCompleted 를 부른다. 그래서 이 두 알림 사이를
	 * 접어 두면 애니메이션이 끝날 때까지 카드가 안 뜬다.
	 *
	 * 배리어는 **붙잡지 않는다.** 붙잡으면 게임플레이가 화면을 기다린다.
	 */
	void HandleActionPresentationBegin(TSharedPtr<FPresentationBarrier> Barrier);
	void HandleActionPresentationEnd(TSharedPtr<FPresentationBarrier> Barrier);

	/**
	 * @brief 판을 톡 쳤다는 알림. 화면을 한 단계 뒤로 되돌린다.
	 *
	 * 월드 입력은 카메라가 갖는다. 카메라가 끌었는지 톡 쳤는지 가려서 이
	 * 신호를 쏘고, 게임플레이와 화면이 나란히 듣는다. HUD 가 입력을 먼저
	 * 채가면 지도가 안 움직인다.
	 */
	/**
	 * @brief 카드 밖을 눌렀을 때. 좌표를 넘기고 카드를 접는다.
	 *
	 * 자식 버튼이 먼저 가져가므로 여기까지 온 눌림은 버튼이 아닌 곳이다.
	 * 그래서 아군 칸을 눌러 다시 펼 때 같은 클릭이 도로 접지 않는다.
	 */
	void HandleBoardPressed(const FVector2D& ScreenPosition);

	/** @brief 뗀 자리로 톡 친 것인지 가른다. */
	void FinishBoardPress(const FVector2D& ScreenPosition);

	/** @brief 이만큼 안에서 움직였으면 톡 친 것으로 본다(px). */
	static constexpr float BoardTapSlack = 24.f;

	FVector2D mPressOrigin = FVector2D::ZeroVector;
	bool mPressMoved = false;

	/** @brief 누름이 아직 안 끝났나. 터치와 마우스가 겹쳐 와도 한 번만 처리한다. */
	bool mPressActive = false;

	/**
	 * @brief 이 자리가 HUD 위인가.
	 *
	 * @details
	 * 판과 글자는 SelfHitTestInvisible 이라 눌림이 그대로 뿌리까지 내려온다.
	 * 그래서 적 안내판이나 라운드 판을 눌러도 "판을 눌렀다" 로 셌고, 카드가
	 * 열렸다. 눌린 자리가 어느 묶음 안인지 재서 가린다.
	 *
	 * 버튼은 여기 안 온다 -- 버튼이 먼저 가져간다.
	 * @param ScreenPosition 눌린 화면 자리
	 * @return HUD 위면 참
	 */
	bool IsOverChrome(const FVector2D& ScreenPosition) const;

	/** @brief 눌림을 삼킬 묶음들. 판·안내판·아군 칸 같은 것. */
	UPROPERTY() TArray<TObjectPtr<UWidget>> mChromeWidgets;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InTouchEvent) override;
	virtual FReply NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InTouchEvent) override;
	virtual FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InTouchEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UFUNCTION() void HandleCommandClicked_0();
	UFUNCTION() void HandleCommandClicked_1();
	UFUNCTION() void HandleCommandClicked_2();
	UFUNCTION() void HandleCommandClicked_3();
	UFUNCTION() void HandleCommandClicked_4();
	UFUNCTION() void HandleCommandClicked_5();
	UFUNCTION() void HandleEndTurnClicked();

	UFUNCTION() void HandlePartyClicked_0();
	UFUNCTION() void HandlePartyClicked_1();
	UFUNCTION() void HandlePartyClicked_2();
	void HandlePartyClicked(int32 SlotIndex);

	/**
	 * @brief 하단 용병 칸 SlotIndex 에 서 있는 유닛의 id.
	 *
	 * 칸은 아군을 나온 차례대로 채운다 -- RefreshParty() 와 같은 순서로 세야
	 * 눌린 칸과 유닛이 어긋나지 않는다.
	 */
	int32 PartyUnitIdAt(int32 SlotIndex) const;

	/**
	 * @brief 사용자가 카드를 펴 두었나. 턴이 시작되면 펴진다.
	 *
	 * 이것이 곧 보이는 여부는 아니다. 조준 중에는 펴 둔 채로도 안 보인다 --
	 * RefreshCommandVisibility() 를 보라.
	 */
	bool mCommandsShown = true;

	/** @brief 직전에 찜해 둔 대상. 바뀌면 카드를 편다. */
	int32 mLastTargetUnitId = INDEX_NONE;

	/** @brief 직전 차례의 유닛. 차례가 바뀔 때만 카드를 편다. */
	int32 mLastTurnUnitId = INDEX_NONE;

	/**
	 * @brief 턴 순서 줄에서 지금 보고 있는 창의 시작 자리.
	 *
	 * @details
	 * 칸이 여섯인데 도는 유닛이 더 많을 수 있다. 양끝 넘김칸을 눌러 창을
	 * 옮긴다. **차례가 바뀌면 0으로 되돌린다** -- 줄 자체가 한 칸 밀리므로,
	 * 옮겨 둔 창을 그대로 두면 다음 턴에 엉뚱한 곳을 보고 있다.
	 */
	int32 mTurnWindowStart = 0;

	/** @brief 양끝 넘김칸. 그쪽에 가려진 수를 적는다. 없으면 빈칸. */
	UPROPERTY() TObjectPtr<UButton> mTurnPageLeft;
	UPROPERTY() TObjectPtr<UButton> mTurnPageRight;
	UPROPERTY() TObjectPtr<UTextBlock> mTurnPageLeftText;
	UPROPERTY() TObjectPtr<UTextBlock> mTurnPageRightText;

	UFUNCTION() void HandleTurnPageLeftClicked();
	UFUNCTION() void HandleTurnPageRightClicked();

	/** @brief 행동 표현이 도는 중인가. 도는 동안 카드를 접는다. */
	bool mIsActionPlaying = false;

	/** @brief 붙인 행동 알림 구독. 뗄 때 쓴다. */
	FDelegateHandle mActionBeginHandle;
	FDelegateHandle mActionEndHandle;
	FDelegateHandle mEndCombatHandle;
	FDelegateHandle mBeginRoundHandle;

	/** @brief 커맨드 칸 하나를 눌렀을 때. 0번은 이동, 나머지는 스킬. */
	void RequestCommand(int32 SlotIndex);

	// 묶음은 아래 private 에 있다. 쓰는 자리가 먼저라 이름만 미리 알린다.
	struct FPartySlotWidgets;

	/**
	 * @brief 아군 칸 하나에 상태이상을 그린다.
	 *
	 * @details
	 * 홈은 늘 서 있고 그림만 갈린다. 걸린 것이 없으면 홈을 흐리게 둔다 --
	 * 감추면 카드 위가 뻥 뚫려서 원래 그런 칸인지 사라진 것인지 모른다.
	 */
	void RefreshPartyStatus(const FPartySlotWidgets& Widgets,
		const FUnitUI& Unit) const;

	/** @brief 상태이상 태그에 맞는 그림. 없으면 nullptr. */
	static UTexture2D* StatusIconFor(const FGameplayTag& StatusTag);

	/** @brief 아군 칸 하나에 AP 를 그린다. 숫자판 + 낱개 열. */
	void RefreshPartyActionPoints(const FPartySlotWidgets& Widgets,
		const FUnitUI& Unit) const;

	/**
	 * @brief 누르는 동안 살짝 줄어들게 한다.
	 *
	 * @details
	 * 어두워지는 것은 버튼 브러시가 맡는다(굽는 쪽 ghost_button). 여기서는
	 * 줄어드는 것만 한다 -- 크기는 브러시로 못 바꾼다.
	 *
	 * 버튼이 아니라 **누른 것이 무엇인지 보여 줄 묶음**을 줄인다. 버튼만
	 * 줄이면 투명한 판이 줄어들 뿐 화면에서는 아무 일도 안 일어난다.
	 */
	void BindPressFeedback(UButton* Button, UWidget* Target);
	UFUNCTION() void HandleAnyPressed();
	UFUNCTION() void HandleAnyReleased();

	/** @brief 눌린 버튼 -> 줄일 묶음. 누른 것을 놓을 때 되돌리려고 든다. */
	UPROPERTY() TMap<TObjectPtr<UButton>, TObjectPtr<UWidget>> mPressTargets;

	/** @brief 지금 줄여 둔 묶음. 놓으면 되돌린다. */
	UPROPERTY() TObjectPtr<UWidget> mPressedTarget = nullptr;

	/** @brief 메뉴 넷. 왼쪽부터 지도 · 스킬 · 가방 · 설정. */
	UPROPERTY() TArray<TObjectPtr<UButton>> mMenuButtons;

	/** @brief 함께 커지는 겹. 스킬 카드 여섯과 용병칸 · AP 막대. */
	UPROPERTY() TObjectPtr<class UScaleBox> mCommandLayer;
	UPROPERTY() TObjectPtr<class UScaleBox> mPartyLayer;

	/** @brief 직전에 잰 화면 크기. 바뀔 때만 다시 잰다. */
	FVector2D mLastViewport = FVector2D::ZeroVector;

	/** @brief 아무리 좁아도 이보다는 안 키운다. 넘으면 판을 덮는다. */
	static constexpr float MaxScreenScale = 1.6f;

	/** @brief 확정 단추 묶음. 공격 범위가 뜬 그때만 편다. */
	UPROPERTY() TObjectPtr<UWidget> mConfirmPanel;
	UPROPERTY() TObjectPtr<UButton> mConfirmButton;

	/** @brief 턴 종료 글자. 무르는 중에는 "취소" 로 바뀐다. */
	UPROPERTY() TObjectPtr<UTextBlock> mEndTurnLabel;

	/** @brief 가운데 AP 막대. 지금 차례인 유닛 것을 그린다. */
	UPROPERTY() TObjectPtr<UTextBlock> mTurnAPText;
	UPROPERTY() TArray<TObjectPtr<UWidget>> mTurnAPPips;
	UPROPERTY() TArray<TObjectPtr<UWidget>> mTurnAPPipsUsed;

	UFUNCTION() void HandleConfirmClicked();

	/**
	 * @brief 좁은 화면에서 HUD 를 키운다.
	 *
	 * @details
	 * 16:9 보다 좁아질수록(폴드처럼 세로로 긴 화면) 판이 작아 보인다. 가로가
	 * 기준인데 그 가로가 줄기 때문이다.
	 *
	 * 그래서 **얼마나 좁아졌나**를 그대로 배율로 쓴다. 16:9 면 1.0 이고,
	 * 4:3 이면 1.33 이다. 위쪽 줄(라운드·턴 순서·메뉴)은 안 건드린다 -- 그쪽은
	 * 가로로 길어서 키우면 서로 부딪힌다.
	 */
	void RefreshScreenScale();

	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

	/* ── 전투 결과·보상 (옛 HUD 에서 옮김) ─────────────────────────────
	 *
	 * 승패가 갈리면 프레임워크가 **배리어를 쥐어 준다.** 그것을 붙잡고 있는
	 * 동안 전투가 멈추고, 놓는 순간 다음으로 넘어간다 -- 결과 영상과 보상
	 * 화면이 그 사이에 뜬다.
	 *
	 * 옛 HUD 가 이 배리어의 유일한 주인이었다. 그래서 옛것을 지우기 전에
	 * 이쪽부터 옮겨야 했다. 안 옮기고 지웠으면 이겨도 아무 일이 안 일어난다.
	 */
	void BeginCombatResultPresentation(TSharedPtr<FPresentationBarrier> Barrier, bool IsPlayerWin);
	void StartCombatResultCinematic();
	void EnsureCombatResultWidgets();
	void HandleCombatResultVideoFinished(class UCinematicWidget* CinematicWidget);
	UFUNCTION() void HandleCombatResultOpenRequested();
	UFUNCTION() void HandleCombatResultRewardConfirmed();
	UFUNCTION() void HandleCombatRewardClaimRequested(ERewardClaimKind ClaimKind, int32 ChoiceIndex);
	UFUNCTION() void HandleCombatResultContinueConfirmed();
	void CloseCombatResultCinematic(FSimpleDelegate Callback);
	void SetCombatResultViewActive(bool bActive, bool bRestoreCombatControls = true);
	FString GetCombatResultVideoPath(bool IsPlayerWin) const;

	/* ── 플로팅 로그 (옛 HUD 에서 옮김) ──────────────────────────────────
	 *
	 * 맞은 자리 위로 피해 숫자가 뜬다. 이것 없이는 몇 대 맞았는지, 얼마나
	 * 아팠는지가 화면에 안 남는다.
	 *
	 * 실행 로그는 몰릴 때가 있어 큐에 쌓았다가 간격을 두고 하나씩 띄운다.
	 * 미리보기 로그는 큐를 안 타고 바로 뜬다.
	 */
	UFUNCTION() void HandleCombatFloatingLog(FCombatFloatingLogRequest Request);
	UFUNCTION() void HandleCombatFloatingLogMotionFinished(int32 MotionIndex);
	UFUNCTION() void HandleCombatFloatingLogsCleared();
	void UpdateFloatingCombatLogQueue(float InDeltaTime);
	void UpdateFloatingCombatLogs(float InDeltaTime);
	void SpawnFloatingCombatLogAtWorld(const FCombatFloatingLogRequest& Request);
	void RemoveFloatingCombatLogsByMotionIndex(int32 MotionIndex);
	UTexture2D* ResolveFloatingLogIcon(EFloatingLogIconType IconType, EFloatingLogColorType ColorType) const;

	/** @brief 아직 안 뜬 대기 칸. 원본 요청만 들고 있다. */
	struct FQueuedFloatingCombatLogEntry
	{
		FCombatFloatingLogRequest mRequest;
		int32 mArrivalOrder = 0;   // 같은 순서일 때 받은 차례를 지킨다
	};

	/** @brief 지금 떠 있는 로그 한 건. 위젯 수명은 캔버스가 쥔다. */
	struct FFloatingCombatLogEntry
	{
		TObjectPtr<UWidget> mRoot;
		FVector mWorldLocation = FVector::ZeroVector;
		int32 mTurnIndex = INDEX_NONE;
		int32 mActionIndex = INDEX_NONE;
		int32 mMotionIndex = INDEX_NONE;
		float mElapsed = 0.0f;
		bool mIsPreview = false;      // 참이면 저절로 안 사라진다
		float mStackOffsetY = 0.0f;   // 미리보기끼리 안 겹치게 쌓는 값
		bool mIsDismissing = false;
		float mDismissElapsed = 0.0f;
	};
	TArray<FQueuedFloatingCombatLogEntry> mPendingFloatingCombatLogs;
	TArray<FFloatingCombatLogEntry> mFloatingCombatLogs;
	int32 mNextFloatingCombatLogArrivalOrder = 0;
	float mFloatingCombatLogQueueCooldown = 0.0f;

	/* ── 라운드 배너 (옛 HUD 에서 옮김) ──────────────────────────────────
	 *
	 * 라운드가 오르면 배리어를 붙잡고 33장짜리 배너를 튼다. 다 돌면 놓아서
	 * 그 라운드 첫 턴이 진행된다. **못 틀면 즉시 놓는다** -- 안 그러면
	 * 라운드가 영영 안 넘어간다.
	 */
	/**
	 * @brief 라운드 시작 배너를 틀지.
	 *
	 * 껐다. 턴 템포를 끊는다는 판단이다 -- 배너가 도는 1초 남짓 동안 배리어를
	 * 붙잡아 게임이 멈춰 서 있다.
	 *
	 * 끄더라도 배리어는 그대로 넘겨받아 즉시 놓는다. 안 놓으면 라운드가 안
	 * 넘어간다. 다시 켜려면 이 값만 참으로 되돌리면 된다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Layout")
	bool mPlayRoundBanner = false;

	bool PlayTurnChangeIntro();
	void FinishTurnChangeIntro();
	bool EnsureTurnChangeFrameTextures();
	void PreloadTurnChangeFrameTextures();
	void EnsureTurnChangeWidgets();
	void SetTurnChangeIntroVisibility(bool bVisible) const;
	int32 GetTurnChangeDisplayNumber() const;
	FString ResolveTurnChangeFrameAssetPath(int32 FrameIndex) const;
	UTexture2D* LoadTurnChangeFrameTexture(const FString& FrameAssetPath) const;
	void UpdateTurnChangeIntro(float InDeltaTime);
	void ApplyTurnChangeFrame(int32 FrameIndex);

	UPROPERTY(Transient) TObjectPtr<class UImage> mTurnChangeVideoImage;
	UPROPERTY(Transient) TObjectPtr<class UButton> mTurnChangeInputBlocker;
	UPROPERTY(Transient) TObjectPtr<class UBorder> mTurnChangeTurnTextPanel;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> mTurnChangeTurnText;
	UPROPERTY(Transient) TObjectPtr<class UBorder> mTurnChangeBackdropPanel;
	UPROPERTY(Transient) TArray<TObjectPtr<UTexture2D>> mTurnChangeFrameTextures;
	TSharedPtr<struct FStreamableHandle> mTurnChangeFramePreloadHandle;
	FSlateBrush mTurnChangeFrameBrush;
	float mTurnChangeIntroElapsed = 0.0f;
	int32 mTurnChangeCurrentFrameIndex = INDEX_NONE;
	bool mTurnChangeIntroPlaying = false;

	/**
	 * @brief 배너를 강제로 끝내는 보험 타이머.
	 *
	 * 정상 종료는 Tick 의 경과 시간 판정뿐이다. 배너 도중 HUD 가 접혀 Tick 이
	 * 멈추면 배리어가 영영 안 풀린다 -- 그 라운드가 진행 불능이 된다.
	 */
	FTimerHandle mTurnChangeSafetyTimerHandle;
	TSharedPtr<FPresentationBarrier> mRoundChangeBarrier;
	int32 mLastShownTurnRound = 0;

	/* ── 유닛 머리 위 HP 바 (옛 HUD 에서 옮김) ────────────────────────────
	 *
	 * 아군 칸에는 파티 셋의 체력이 뜨지만 **적 체력은 볼 자리가 없다.** 머리
	 * 위 바가 그 자리다 -- 이것 없이는 누구를 먼저 칠지 고를 수가 없다.
	 *
	 * 월드 자리를 매 프레임 화면으로 옮겨 붙이므로 Tick 에서 돈다.
	 */
	void RebuildUnitHpBars();
	void UpdateUnitHpBars();
	void SetupUnitHpBarFillClip(FCombatUnitHpBarWidget& Bar);
	void CacheUnitHpBarStatusSlots(FCombatUnitHpBarWidget& Bar) const;
	void UpdateUnitHpBarStatus(FCombatUnitHpBarWidget& Bar, const FUnitUI& Unit) const;
	UTexture2D* ResolveStatusIcon(const FGameplayTag& StatusTag) const;

	UPROPERTY(Transient) TArray<FCombatUnitHpBarWidget> mUnitHpBars;
	UPROPERTY(Transient) TObjectPtr<class UCanvasPanel> mRootCanvas;
	UPROPERTY() TSubclassOf<UUserWidget> mUnitHpBarWidgetClass;
	UPROPERTY() TObjectPtr<UTexture2D> mUnitHpFillRedTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mUnitHpFillGreenTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mUnitDefenseIconTexture;

	/** @brief 상태이상 딱지 그림. 전용 그림이 없는 태그는 빈 칸으로 둔다. */
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconHpDamage;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconHpRecovery;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconGetMove;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconGetDefense;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconAgility;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconFortification;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconVulnerability;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconWeakness;

	/** @brief 승리 뒤 다음 방을 고르도록 지도를 연다. */
	void OpenWorldMapForNextRoom();

	/** @brief 결과 화면이 뜬 동안 조작을 감춘다. */
	void SetCombatControlsShown(bool bShown);

	/** @brief 전투가 끝났다. 결과 연출을 시작한다. */
	void HandleEndCombatUI(TSharedPtr<FPresentationBarrier> Barrier);

	UPROPERTY(Transient) TObjectPtr<class UCinematicWidget> mCombatResultCinematicWidget;
	UPROPERTY(Transient) TObjectPtr<class UCombatResultOverlayWidget> mCombatResultOverlayWidget;
	TSharedPtr<FPresentationBarrier> mCombatResultBarrier;
	FTimerHandle mCombatResultStartDelayTimerHandle;
	bool mIsPlayerWin = false;
	bool mCombatResultFlowActive = false;
	bool mVictoryWorldMapLocked = false;

	/** @brief 보상 화면. 결과 영상이 끝나면 그 위에 뜬다. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Result")
	TSubclassOf<class URewardUIWidgetBase> mRewardWidgetClass;
	UPROPERTY(Transient) TObjectPtr<class URewardUIWidgetBase> mCombatRewardWidget;
	UPROPERTY(Transient) TObjectPtr<class URewardUIModel> mCombatRewardUIModel;

	/** @brief 경험치가 차오를 때 나는 소리. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Result")
	TObjectPtr<USoundBase> mExpGainSound;

	/** @brief 승리·패배 징글. 없으면 소리 없이 지나간다. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Result")
	TObjectPtr<USoundBase> mVictoryJingleSound;
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Result")
	TObjectPtr<USoundBase> mDefeatJingleSound;

	/** @brief 확정 단추와 턴 종료 글자를 지금 단계에 맞춘다. */
	void RefreshActionButtons();

	/* ── 쓸 행동력 미리 보이기 ────────────────────────────────────────────
	 *
	 * 카드를 고르면 그 몫만큼 칸이 숨쉬듯 빛난다. 쓰면 뒤부터 없어지므로
	 * 뒤에서부터 빛낸다.
	 *
	 * **이동은 아직 한 칸 값만 안다.** 카드에 적힌 값(mActionPointCost)이
	 * 계약에 있는 전부다 -- 몇 칸을 갈지에 따라 달라지는 값은 게임플레이만
	 * 아는 것이라, 그 칸이 생기면 여기 대신 넣으면 된다.
	 */
	int32 GetSelectedSkillCost() const;
	void RefreshPendingAPGlow(float DeltaTime);

	/** @brief 한 번 숨쉬는 데 걸리는 빠르기(라디안/초). */
	static constexpr float APGlowSpeed = 4.2f;

	int32 mPendingAPCost = 0;
	int32 mShownAPLeft = 0;
	float mAPGlowPhase = 0.f;

	/** @brief 가운데 AP 막대를 지금 차례인 유닛으로 채운다. */
	void RefreshTurnActionPoints();

private:
	/** @brief 파티 카드 한 장이 쓰는 위젯 묶음. 없는 것은 null. */
	struct FPartySlotWidgets
	{
		TObjectPtr<UWidget> Root;
		/**
		 * @brief 판과 프레임을 뺀 내용 전체.
		 *
		 * @details
		 * 빈 칸을 그릴 때 내용만 통째로 감춘다. 위젯을 하나씩 감추면 계약에
		 * 없는 장식 -- 초상화 테, 꺼진 보석 바탕 -- 이 남아서 빈 칸에 유령
		 * 고리와 유령 보석이 뜬다. 실제로 그렇게 보였다.
		 */
		TObjectPtr<UWidget> Content;
		TObjectPtr<UImage> Portrait;
		TObjectPtr<UTextBlock> Name;
		TObjectPtr<UProgressBar> HPBar;
		TObjectPtr<UTextBlock> HPText;
		TObjectPtr<UTextBlock> APText;
		/** @brief AP 가 낱개 자리보다 많을 때만 켜는 아이콘. 옆에 "x N" 이 붙는다. */
		TObjectPtr<UWidget> APPlate = nullptr;
		/** @brief 아직 남은 칸. 밝은 그림. */
		TArray<TObjectPtr<UWidget>> APPips;
		/** @brief 이미 쓴 칸. 흐린 그림. 같은 자리에 겹쳐 있다. */
		TArray<TObjectPtr<UWidget>> APPipsUsed;
		TObjectPtr<UTextBlock> StatusText;
		/** @brief 늘 서 있는 빈 홈. 상태가 없으면 흐리게 둔다. */
		TArray<TObjectPtr<UWidget>> StatusFrames;
		/** @brief 홈 안에 들어가는 그림. 걸린 순서대로 앞에서부터 켠다. */
		TArray<TObjectPtr<UImage>> StatusIcons;
		/** @brief 상태이상 글자 옆 아이콘. 글자와 같이 켜지고 꺼진다. */
		TObjectPtr<UWidget> StatusIcon;
	};

	/** @brief 커맨드 칸 한 개가 쓰는 위젯 묶음. */
	struct FCommandSlotWidgets
	{
		TObjectPtr<UWidget> Root;
		TObjectPtr<UButton> Button;
		TObjectPtr<UImage> Icon;
		TObjectPtr<UTextBlock> Name;
		TObjectPtr<UTextBlock> Cost;
		TObjectPtr<UTextBlock> CostLine;
		TObjectPtr<UTextBlock> Cooldown;
		/** @brief 쿨타임 글자 옆 아이콘. 글자와 같이 켜지고 꺼진다. */
		TObjectPtr<UWidget> CooldownIcon;
		TObjectPtr<UTextBlock> Damage;
		TObjectPtr<UWidget> Disabled;
	};

	/** @brief 턴 순서 토큰 한 개. */
	struct FTurnSlotWidgets
	{
		TObjectPtr<UWidget> Root;
		TObjectPtr<UImage> Portrait;
		TObjectPtr<UTextBlock> Name;
		TObjectPtr<UWidget> Current;
	};

	/** @brief 빈 아군 칸을 접지 않고 "비어 있음"으로 그린다. */
	void ClearPartySlot(const FPartySlotWidgets& Widgets);

	TArray<FPartySlotWidgets> mPartySlots;
	TArray<FCommandSlotWidgets> mCommandSlots;
	TArray<FTurnSlotWidgets> mTurnSlots;

	TObjectPtr<UTextBlock> mRoundText;
	TObjectPtr<UTextBlock> mObjectiveText;

	TObjectPtr<UWidget> mEnemyPanel;
	TObjectPtr<UImage> mEnemyPortrait;
	TObjectPtr<UTextBlock> mEnemyName;
	TObjectPtr<UProgressBar> mEnemyHPBar;
	TObjectPtr<UTextBlock> mEnemyHPText;
	TObjectPtr<UTextBlock> mEnemyDefenseText;
	TObjectPtr<UTextBlock> mEnemyStatusText;
	TObjectPtr<UTextBlock> mEnemyForecastText;

	TObjectPtr<UButton> mEndTurnButton;

	/** @brief 미리보기용 가짜 전투 드라이버. 실제 전투에서는 null이다. */
	UPROPERTY(Transient) TObjectPtr<UMockCombatDriver> mPreviewDriver;

	/** @brief 캐시가 끝났는지. NativeConstruct에서 한 번만 돈다. */
	bool mCached = false;
};
