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
 * 기존 UCombatTileMapHUDWidget과 나란히 존재한다. 그쪽은 위젯을 C++에서
 * 만들어 붙이므로 WBP를 바꿔도 배치가 바뀌지 않는다. 이 클래스는 반대로
 * **배치를 WBP가 소유**한다. 위젯은 전부 이름으로 찾고, 없으면 그냥 건너뛴다.
 * 그래서 배치안마다 WBP만 새로 만들면 되고, 어떤 안이 어떤 요소를 빼도 된다.
 *
 * 한 배치안 = WBP 하나. BP_CombatGameMode에서 어느 WBP를 쓸지만 바꾼다.
 */

#include "RDMinimal.h"
#include "UI/Combat/CombatUIWidgetBase.h"

#include "CombatLayoutHUDWidget.generated.h"

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

	/** @brief 판 탭 알림까지 같이 구독한다. */
	/** @brief 이 배치안이 화면에 표시할 파티 인원. 기획상 최대 3명. */
	static constexpr int32 PartySlotCount = 3;

	/** @brief 턴 순서에 표시할 최대 인원. */
	static constexpr int32 TurnSlotCount = 6;

	/** @brief 커맨드 레일 칸 수. 이동 + 기본공격 + 스킬 4개. */
	static constexpr int32 CommandSlotCount = 6;

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
	 * 배치안이 정해지면 이 기본값을 false로 내리면 된다.
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

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InTouchEvent) override;

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

	/** @brief 커맨드 칸 하나를 눌렀을 때. 0번은 이동, 나머지는 스킬. */
	void RequestCommand(int32 SlotIndex);

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
		TObjectPtr<UWidget> Selected;
		TObjectPtr<UImage> Portrait;
		TObjectPtr<UTextBlock> Name;
		TObjectPtr<UProgressBar> HPBar;
		TObjectPtr<UTextBlock> HPText;
		TObjectPtr<UTextBlock> APText;
	/** @brief AP 가 낱개 자리보다 많을 때만 켜는 아이콘. 옆에 "x N" 이 붙는다. */
	UPROPERTY() TObjectPtr<UWidget> APIcon = nullptr;
		TArray<TObjectPtr<UWidget>> APPips;
		TObjectPtr<UTextBlock> StatusText;
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
		TObjectPtr<UWidget> Selected;
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
