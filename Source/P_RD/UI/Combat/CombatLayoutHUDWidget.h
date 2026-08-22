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
struct FCombatSkillCutInRequest;

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
	/**
	 * @brief 채움이 Overlay(HpFillImageMount)에 감싸진 WBP 용 드레인 경로.
	 *
	 * 감싼 뒤로 채움의 Slot 이 OverlaySlot 이 되어 위의 캔버스 슬롯 방식이
	 * 조용히 죽었다 -- 숫자만 줄고 바는 가득이었다(0811 제보). 이때는 왼쪽
	 * 정렬 + 희망 크기(DesiredSizeOverride)로 폭을 줄인다.
	 */
	UPROPERTY(Transient) bool mFillUsesDesiredSize = false;
	UPROPERTY(Transient) float mFillFullHeight = 0.0f;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> mValueText;
	/** @brief 방어도 아이콘과 수치. 0 이면 감춘다. */
	UPROPERTY(Transient) TObjectPtr<class UImage> mDefenseIcon;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> mDefenseText;
	/**
	 * @brief 바 테두리(백플레이트). 걸린 상태에 따라 색을 물들인다.
	 *
	 * 월드 위 상태 아이콘은 유닛과 타일을 가려서 껐다. 대신 테두리 색으로
	 * "좋은 것/나쁜 것이 걸렸다" 만 알린다 -- 무엇인지는 요약판이 맡는다.
	 */
	UPROPERTY(Transient) TObjectPtr<class UImage> mBackplateImage;
	/**
	 * @brief HP바 밑에 붙는 상태 띠. 왼쪽이 이로운 것, 오른쪽이 해로운 것.
	 *
	 * @details 처음에는 바 뒤에서 숨쉬듯 밝아지는 빛으로 했는데, 깜빡이는
	 * 표시는 시선을 뺏고(접근성 지침도 금한다) 유닛이 여럿이면 화면이
	 * 소란해진다는 조사 결과가 나왔다(0806). 대신 얇은 띠를 깔고 **자리로**
	 * 종류를 가른다 -- 색만으로 뜻을 나르지 않는다. 둘 다 걸리면 반씩 나눈다.
	 */
	UPROPERTY(Transient) TObjectPtr<class UImage> mStatusRailBuff;
	UPROPERTY(Transient) TObjectPtr<class UImage> mStatusRailDebuff;
	/**
	 * @brief 프레임 글로우. 버프=금, 디버프=보라로 판 뒤에서 은은하게 빛난다.
	 *
	 * 0806 조사에서 "숨쉬는(깜빡이는) 빛" 은 시선을 뺏어 폐기했으므로,
	 * 이 글로우는 **정적**이다 -- 켜짐/꺼짐과 색만 바뀐다(0811 결정).
	 * 종류 구분의 접근성은 아래 상태 띠(자리로 가름)가 계속 맡는다.
	 */
	UPROPERTY(Transient) TObjectPtr<class UImage> mFrameGlowImage;
	/** @brief 다 찼을 때의 폭. 자를 기준이 된다. */
	float mFillFullWidth = 0.0f;

	UPROPERTY(Transient) TArray<TObjectPtr<class UImage>> mStatusIcons;
	UPROPERTY(Transient) TArray<TObjectPtr<class UImage>> mStatusFrames;
	UPROPERTY(Transient) TArray<TObjectPtr<class UTextBlock>> mStatusCountTexts;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> mStatusOverflowText;
};
enum class ERewardClaimKind : uint8;

class UButton;
class UMockCombatDriver;
class UImage;
class UProgressBar;
class URewardUIModel;
class USkillDetailOverlayPresenter;
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

	/** @brief 전투 종료 보상 화면이 읽고 claim 요청을 보낼 모델을 연결한다. */
	void BindRewardUIModel(URewardUIModel* InUIModel);

	/** @brief 판 탭 알림까지 같이 구독한다. */
	/** @brief 이 배치안이 화면에 표시할 파티 인원. 기획상 최대 3명. */
	static constexpr int32 PartySlotCount = 3;

	/** @brief 턴 순서에 표시할 최대 인원. */
	static constexpr int32 TurnSlotCount = 10;

	/** @brief 좌하단 AP 위에 한 줄로 보이는 파티 공용 아티팩트 수. */
	static constexpr int32 ArtifactSlotCount = 6;

	/** @brief 커맨드 레일 칸 수. 이동 + 기본공격 + 스킬 4개. */
	static constexpr int32 CommandSlotCount = 6;

	/** @brief 누른 동안 줄어드는 정도. 더 줄이면 눌린 게 아니라 튄 것으로 보인다. */
	static constexpr float PressedScale = 0.95f;

	/** @brief PR457 상세 겹이 현재 열려 있는가. 입력 흐름과 자동화 검증이 함께 쓴다. */
	bool IsDetailOverlayShown() const;

	/** @brief 몬스터 탭이 지금 열려 있는가. 입력 흐름과 자동화 검증이 함께 쓴다. */
	bool IsMonsterTabShown() const;

#if WITH_DEV_AUTOMATION_TESTS
	/** @brief 외부 UI 자산 없이 상세 왕복을 검증할 때 쓸 위젯 클래스를 넣는다. */
	void SetDetailOverlayWidgetClassForTest(TSubclassOf<UUserWidget> WidgetClass)
	{
		mDetailOverlayWidgetClass = WidgetClass;
	}

	/** @brief 몬스터 탭의 실제 버튼 배선을 자동화에서 확인한다. */
	UUserWidget* GetMonsterTabWidgetForTest() const
	{
		return mMonsterTabWidget;
	}
	UUserWidget* GetDetailOverlayWidgetForTest() const
	{
		return mDetailOverlayWidget;
	}
	class USkillTacticalDiagramWidget* GetSkillTacticalDiagramForTest() const
	{
		return mSkillTacticalDiagramWidget;
	}
	UUserWidget* GetSkillDetailContentForTest() const
	{
		return mSkillDetailContentWidget;
	}
	const FSkillDetailUI& GetRenderedSkillDetailForTest() const
	{
		return mRenderedSkillDetailForTest;
	}
	FString GetDetailChipValueForTest(int32 ChipIndex) const;
	FString GetDetailSubtitleForTest() const;

	/** @brief 상태 소켓 타이머를 기다리지 않고 발화시켜 상세 왕복을 검증한다. */
	void TriggerStatusLongPressForTest(bool bAlly, int32 SlotIndex);
	/** @brief 상태 소켓 긴 누름 타이머가 실제로 대기 중인지. */
	bool IsStatusLongPressPendingForTest() const;
	/** @brief 멀티터치/늦은 Release 검증용 현재 후보 식별자. */
	bool IsStatusPressActiveForTest(bool bAlly, int32 SlotIndex) const;

	/** @brief 몬스터 스킬 롱프레스를 타이머 대기 없이 발화한다. */
	void TriggerMonsterSkillLongPressForTest(int32 SlotIndex);
	/** @brief 몬스터 스킬 후보 타이머/식별자를 자동화에서 확인한다. */
	bool IsMonsterSkillLongPressPendingForTest() const;
	bool IsMonsterSkillPressActiveForTest(int32 SlotIndex) const;
	/** @brief 두 독립 viewport 모달의 실제 Z-order 계약. */
	int32 GetMonsterTabViewportZOrderForTest() const;
	int32 GetDetailOverlayViewportZOrderForTest() const;
	void CloseDetailOverlayForTest() { HideDetailOverlay(false); }
	/** 실제 공용 상세 WBP에 지정 아티팩트를 채워 캡처하는 테스트 진입점. */
	void ShowArtifactDetailForTest(int32 SlotIndex)
	{
		ShowArtifactDetailOverlay(SlotIndex);
	}
	void ShowMercenaryInventoryForTest()
	{
		HandleInventoryClicked();
	}
	/** @brief 자동 턴 초점이 실제 카드 고리 앵커를 보냈는지 검증한다. */
	FVector2D GetCommandRingAnchorForTest() const { return ComputeCommandRingAnchor(); }
	/** @brief 패배에 승리 징글이 연결되지 않는 결과 음향 선택 계약. */
	USoundBase* SelectCombatResultJingleForTest(bool bPlayerWin) const
	{
		return SelectCombatResultJingle(bPlayerWin);
	}
	/** @brief 보상 완료 직후 지도가 HUD를 덮은 상태를 재현한다. */
	void EnterVictoryWorldMapStateForTest()
	{
		SetCombatResultViewActive(true, false);
		mVictoryWorldMapLocked = true;
	}
	void RestorePostVictoryHUDAndInputForTest()
	{
		RestorePostVictoryHUDAndInput();
	}
	bool IsVictoryWorldMapLockedForTest() const
	{
		return mVictoryWorldMapLocked;
	}
	void SetVictoryWorldMapForTest(class UFrontendMapWidget* InWorldMap)
	{
		mVictoryWorldMap = InWorldMap;
	}
#endif

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

public:
	/**
	 * @brief 화면에서 실제로 걷어낼 때 상세 겹(프레젠터 소유)을 함께 걷는다.
	 *
	 * @details 상세 겹 정리를 NativeDestruct에 두면 안 된다 — NativeDestruct는
	 * Slate 수명 이벤트라, 위젯 렌더러(FWidgetRenderer)가 캡처용 임시 Slate
	 * 트리를 버릴 때도 불린다. 그때 프레젠터를 끊으면 살아 있는 HUD의
	 * 상세 배선(전술판 버튼 등)이 소리 없이 죽는다. 진짜 제거 경로인
	 * RemoveFromParent(CloseUI→ApplyCloseUI, 뷰포트/레벨 정리)에서만 걷는다.
	 */
	virtual void RemoveFromParent() override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
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
	void RefreshMercenaryInventory();
	void SelectMercenaryInventoryArtifact(int32 SlotIndex);

	/** @brief 프리미엄 용병 셸을 패널 최하단에 한 번 만들고 구식 판을 숨긴다. */
	void EnsureMercenaryRosterShell();
	/** @brief 구형 용병 WBP에도 스킬 슬롯의 투명 탭 영역을 보충한다. */
	void EnsureMercenarySkillButtons();

	/** @brief 전투 HUD의 용병 메뉴에서 보유 용병 패널을 펴거나 접는다. */
	void SetMercenaryPanelShown(bool bShown);
	/** @brief 용병 상세와 같은 판 안에서 인벤토리 페이지를 전환한다. */
	void SetMercenaryInventoryShown(bool bShown);

	/** @brief 보유 용병 패널이 지금 열려 있는가. */
	bool IsMercenaryPanelShown() const;
	/** @brief 전투판과 카메라 입력을 가려야 하는 모달이 하나라도 열려 있는가. */
	bool IsWorldInputModalShown() const;
	/** @brief raw touch를 직접 읽는 전투 카메라 Pawn에 현재 모달 잠금을 반영한다. */
	void RefreshWorldGestureInputBlock();

	/** @brief 전투 HUD의 몬스터 메뉴에서 몬스터 탭(WBP_MonsterTab_Marchbound)을 펴거나 접는다. */
	void SetMonsterTabShown(bool bShown);

	/**
	 * @brief 상단 레일 넷째 버튼(톱니)으로 공용 설정 팝업을 연다.
	 *
	 * @details 설정은 전투 HUD 의 하위 화면이 아니라 타이틀과 함께 쓰는 월드
	 * 위젯(InGameSettings)이다. 그래서 여기서 만들지 않고 서브시스템이 준비한
	 * 것을 열기만 한다 -- 타이틀과 인게임이 다른 인스턴스를 쓰면 설정이 갈린다.
	 */
	UFUNCTION() void HandleSettingsMenuClicked();
	/** @brief 설정 패널의 Back 요청을 받아 패널을 닫는다. */
	UFUNCTION() void HandleSettingsPanelBackRequested();
	/** @brief 설정 패널의 저장 후 종료 요청을 UIModel로 전달한다. */
	UFUNCTION() void HandleSettingsPanelSaveAndExitRequested();
	/** @brief 확인을 마친 런 포기 요청을 기존 전투 UIModel 경로로 전달한다. */
	UFUNCTION() void HandleSettingsPanelAbandonRunConfirmed();
	/** @brief 저장 또는 프론트엔드 전환 실패 시 설정판 입력과 상태를 복구한다. */
	UFUNCTION() void HandleSaveAndExitCompleted(bool bSuccess);
	/** @brief 런 포기 또는 프론트엔드 전환 실패 시 설정판 입력과 상태를 복구한다. */
	UFUNCTION() void HandleAbandonRunCompleted(bool bSuccess);

	/* ── 아티팩트 상세 ──────────────────────────────────────────────────
	 *
	 * 아티팩트 칸은 그림만 있고 누를 수가 없었다. 이름과 그림만 봐서는 무슨
	 * 효과인지 알 길이 없어서, 꾹 누르면 상세 겹을 스킬/유닛과 같은 판으로
	 * 띄운다. 값은 이미 내려와 있는 PlayerMeta.mArtifacts 를 그대로 읽는다 --
	 * 따로 청하지 않으므로 왕복이 없다.
	 */
	void ShowArtifactDetailOverlay(int32 SlotIndex);
	void HandleArtifactLongPress(int32 SlotIndex);
	UFUNCTION() void HandleArtifactPressed_0();
	UFUNCTION() void HandleArtifactPressed_1();
	UFUNCTION() void HandleArtifactPressed_2();
	UFUNCTION() void HandleArtifactPressed_3();
	UFUNCTION() void HandleArtifactPressed_4();
	UFUNCTION() void HandleArtifactPressed_5();
	UFUNCTION() void HandleArtifactReleased();
	void BeginArtifactPress(int32 SlotIndex);

	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> mArtifactButtons;
	FTimerHandle mArtifactLongPressTimerHandle;
	int32 mArtifactPressedSlot = INDEX_NONE;

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
	 * @brief 턴이 실제로 열린 동안에만 카드를 보여 준다.
	 *
	 * @details
	 * 턴 종료 알림 뒤에도 TurnUI에는 방금 끝난 유닛이 잠시 남는다. 유닛 id만
	 * 보고 표시 여부를 정하면 그 틈에 플레이어 스킬 카드가 그대로 남는다.
	 * 시작/종료 알림을 별도 상태로 받아 그 공백을 닫는다.
	 */
	void HandleTurnPresentationBegin(TSharedPtr<FPresentationBarrier> Barrier);
	void HandleTurnPresentationEnd(TSharedPtr<FPresentationBarrier> Barrier);

	/**
	 * @brief 행동이 노는 동안 카드를 접어 둔다.
	 *
	 * @details
	 * 걸어가는 중에 카드가 도로 떠서, 아직 안 끝난 것을 끝난 것처럼 보였다.
	 * 행동은 표현이 다 끝난 뒤에야 끝났다고 알려 온다 -- 이동은 마지막 칸에
	 * 도착한 뒤에 MarkActionCompleted 를 부른다.
	 *
	 * 빌드 액션 종료와 실제 스킬 액션 시작은 연달아 온다. 종료 알림에서 바로
	 * 펴면 그 둘 사이 한 프레임 동안 카드가 공격 위에 번쩍인다. 종료는 다음
	 * 틱에 확정하고, 그 전에 다음 액션/턴 알림이 오면 예약을 무효화한다.
	 *
	 * 배리어는 **붙잡지 않는다.** 붙잡으면 게임플레이가 화면을 기다린다.
	 */
	void HandleActionPresentationBegin(TSharedPtr<FPresentationBarrier> Barrier);
	void HandleActionPresentationEnd(TSharedPtr<FPresentationBarrier> Barrier);
	void CompleteActionPresentationEnd(uint64 PresentationSerial);

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

	/** @brief 이 시간(초)을 넘게 누르고 있으면 긴 누름으로 본다. */
	static constexpr float LongPressSeconds = 0.5f;

	/**
	 * @brief 판을 오래 눌렀다. 그 자리를 상세 요청으로 보낸다.
	 *
	 * @details
	 * 뗄 때가 아니라 누르고 있는 도중에 발화한다 -- 뗄 때 판정하면 "길게
	 * 눌렀다"는 감각과 화면 반응 사이가 벌어진다. 발화한 누름은 소비되어
	 * 뗄 때(FinishBoardPress) 탭으로 한 번 더 처리되지 않는다.
	 *
	 * 어느 유닛인지는 게임플레이가 트레이스로 푼다. 상세는 SetUnitDetail 로
	 * 되돌아오고, 적이면 위협 범위도 판에 같이 칠려 온다.
	 */
	void HandleBoardLongPress();
	FTimerHandle mBoardLongPressTimerHandle;

	/** @brief 카드를 오래 눌렀다. 그 스킬의 상세를 요청하고 이어질 클릭은 삼킨다. */
	void HandleCommandLongPress(int32 SlotIndex);
	FTimerHandle mCommandLongPressTimerHandle;

	/**
	 * @brief 긴 누름이 발화했으면 뒤따라 오는 클릭(뗌)을 한 번 무시한다.
	 *
	 * 버튼 클릭은 뗄 때 온다. 상세를 열어 준 누름이 뗌에서 선택으로 한 번 더
	 * 처리되면, 설명을 보려던 손이 스킬을 골라 버린다.
	 */
	bool mSwallowNextCommandClick = false;

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
	/** @brief 스킬 단추 -- 차례 유닛의 카드를 펴고 접는 정문(0807). */
	UFUNCTION() void HandleSkillToggleClicked();

	UFUNCTION() void HandlePartyClicked_0();
	UFUNCTION() void HandlePartyClicked_1();
	UFUNCTION() void HandlePartyClicked_2();
	void HandlePartyClicked(int32 SlotIndex);
	/** @brief 용병 3인 목록 아래 WBP 인벤토리 탭을 눌렀다. */
	UFUNCTION() void HandleInventoryClicked();
	UFUNCTION() void HandleMercenaryInventoryArtifactClicked_0();
	UFUNCTION() void HandleMercenaryInventoryArtifactClicked_1();
	UFUNCTION() void HandleMercenaryInventoryArtifactClicked_2();
	UFUNCTION() void HandleMercenaryInventoryArtifactClicked_3();
	UFUNCTION() void HandleMercenaryInventoryArtifactClicked_4();
	UFUNCTION() void HandleMercenaryInventoryArtifactClicked_5();
	UFUNCTION() void HandleMercenaryInventoryArtifactClicked_6();
	UFUNCTION() void HandleMercenaryInventoryArtifactClicked_7();
	UFUNCTION() void HandleMercenaryInventoryArtifactClicked_8();
	UFUNCTION() void HandleMercenaryInventoryArtifactClicked_9();
	UFUNCTION() void HandleMercenaryInventoryArtifactClicked_10();

	/** @brief 상단 용병 메뉴를 눌러 보유 용병 패널을 토글한다. */
	UFUNCTION() void HandleMercenaryMenuClicked();
	/** @brief 상단 몬스터 메뉴로 몬스터 탭을 토글한다. 탭 WBP가 없으면 첫 생존 적의 정보 패널로 대신한다. */
	UFUNCTION() void HandleMonsterMenuClicked();

	UFUNCTION() void HandleMonsterTabRowClicked_0();
	UFUNCTION() void HandleMonsterTabRowClicked_1();
	UFUNCTION() void HandleMonsterTabRowClicked_2();
	void HandleMonsterTabRowClicked(int32 RowIndex);
	/** @brief 몬스터 탭 WBP의 뒤로 버튼으로 모달을 닫는다. */
	UFUNCTION() void HandleMonsterTabBackClicked();

	/** @brief 보유 용병 패널의 닫기 단추를 눌렀다. */
	UFUNCTION() void HandleMercenaryCloseClicked();

	/** @brief 상단 지도 메뉴를 눌러 현재 런 지도를 조회용으로 토글한다. */
	UFUNCTION() void HandleWorldMapMenuClicked();

	/**
	 * @brief 용병 패널 SlotIndex 에 서 있는 유닛의 id.
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

	/** @brief 직전 차례의 유닛. 차례가 바뀔 때만 카드를 편다. */
	int32 mLastTurnUnitId = INDEX_NONE;

	/** @brief 턴바가 마지막으로 본 라운드. 한 유닛 전투에서도 페이지를 되감는다. */
	int32 mLastTurnBarRound = INDEX_NONE;

	/**
	 * @brief 턴 순서 줄에서 지금 보고 있는 창의 시작 자리.
	 *
	 * @details
	 * 칸이 열인데 도는 유닛이 더 많을 수 있다. 양끝 넘김칸을 눌러 창을
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

	/** @brief BeginTurn부터 EndTurn까지 실제 턴이 열린 상태인가. */
	bool mIsTurnActive = false;

	/**
	 * @brief 늦게 도착한 행동 종료 예약을 무효화하는 일련번호.
	 *
	 * BuildAction 종료 직후 SkillAction 시작처럼 같은 프레임에 알림이 이어질
	 * 수 있어, bool 하나만으로는 사이 프레임의 재노출을 막을 수 없다.
	 */
	uint64 mActionPresentationSerial = 0;

	/** @brief 붙인 턴/행동 알림 구독. 뗄 때 쓴다. */
	FDelegateHandle mTurnBeginHandle;
	FDelegateHandle mTurnEndHandle;
	FDelegateHandle mActionBeginHandle;
	FDelegateHandle mActionEndHandle;
	FDelegateHandle mBeginCombatHandle;
	FDelegateHandle mEndCombatHandle;
	FDelegateHandle mBeginRoundHandle;
	FDelegateHandle mSkillCutInHandle;

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

	/** @brief 메뉴 넷. 왼쪽부터 지도 · 용병 · 빈칸 · 설정. */
	UPROPERTY() TArray<TObjectPtr<UButton>> mMenuButtons;

	/** @brief 보유 용병만 보여 주는 전투 내 읽기 전용 패널. 고용은 상점 몫이다. */
	UPROPERTY() TObjectPtr<UWidget> mMercenaryPanel;

	/** @brief 프리미엄 셸. 소프트 참조라 에셋 import 전에도 CDO 로드가 깨지지 않는다. */
	UPROPERTY()
	TSoftObjectPtr<UTexture2D> mMercenaryRosterShellTexture;
	/** @brief 좌측 용병 선택 카드의 일반/현재 턴 상태 프레임. */
	UPROPERTY()
	TSoftObjectPtr<UTexture2D> mMercenaryCardNormalTexture;
	UPROPERTY()
	TSoftObjectPtr<UTexture2D> mMercenaryCardSelectedTexture;

	/** @brief MercenaryPanel 최하단에 런타임으로 붙인 프리미엄 셸 그림. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> mMercenaryRosterShellImage;

	/** @brief 용병 패널의 현재 보유 골드 숫자와 닫기 단추. */
	UPROPERTY() TObjectPtr<UTextBlock> mMercenaryGoldText;
	UPROPERTY() TObjectPtr<UButton> mMercenaryCloseButton;
	/** @brief 좌측 목록에서 현재 턴 용병을 크게 보여 주는 중앙/우측 요약. */
	UPROPERTY() TObjectPtr<UImage> mMercenaryHeroPortrait;
	UPROPERTY() TObjectPtr<UTextBlock> mMercenaryDetailName;
	UPROPERTY() TObjectPtr<UTextBlock> mMercenaryDetailHP;
	UPROPERTY() TObjectPtr<UTextBlock> mMercenaryDetailAP;
	UPROPERTY() TObjectPtr<UTextBlock> mMercenaryDetailSpeed;
	UPROPERTY() TObjectPtr<UWidget> mMercenaryDetailSection;

	/** @brief 네 번째 로스터 탭이 여는 용병 패널 내부 인벤토리 페이지. */
	UPROPERTY() TObjectPtr<UWidget> mMercenaryInventoryPage;
	UPROPERTY() TObjectPtr<UImage> mMercenaryInventoryPlate;
	UPROPERTY() TObjectPtr<UTextBlock> mMercenaryInventoryGoldText;
	UPROPERTY() TArray<TObjectPtr<UWidget>> mMercenaryInventoryArtifactFrames;
	UPROPERTY() TArray<TObjectPtr<UImage>> mMercenaryInventoryArtifactIcons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> mMercenaryInventoryArtifactNames;
	UPROPERTY() TArray<TObjectPtr<UButton>> mMercenaryInventoryArtifactButtons;
	bool mMercenaryInventoryShown = false;

	/** @brief 함께 커지는 겹. 스킬 카드 여섯과 AP 막대. */
	UPROPERTY() TObjectPtr<class UScaleBox> mCommandLayer;
	UPROPERTY() TObjectPtr<class UScaleBox> mPartyLayer;


	/** @brief 확정 단추 묶음. 공격 범위가 뜬 그때만 편다. */
	UPROPERTY() TObjectPtr<UWidget> mConfirmPanel;
	UPROPERTY() TObjectPtr<UButton> mConfirmButton;

	/** @brief 턴 종료 글자. 무르는 중에는 "취소" 로 바뀐다. */
	UPROPERTY() TObjectPtr<UTextBlock> mEndTurnLabel;

	/** @brief 가운데 AP 막대. 지금 차례인 유닛 것을 그린다. */
	UPROPERTY() TObjectPtr<UWidget> mTurnAPRoot;
	UPROPERTY() TObjectPtr<UTextBlock> mTurnAPText;
	UPROPERTY() TArray<TObjectPtr<UWidget>> mTurnAPPips;
	UPROPERTY() TArray<TObjectPtr<UWidget>> mTurnAPPipsUsed;

	/** @brief WBP_MercenaryPanel의 3인 목록 아래 인벤토리 탭. */
	UPROPERTY() TObjectPtr<UButton> mMercenaryInventoryButton;

	/** @brief 좌하단 AP 위의 파티 공용 아티팩트 그림. 별도 프레임은 쓰지 않는다. */
	UPROPERTY() TArray<TObjectPtr<UImage>> mArtifactIcons;
	UPROPERTY() TArray<TObjectPtr<UWidget>> mArtifactFrames;

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
	 * 동안 전투가 멈추고, 놓는 순간 다음으로 넘어간다. 승리는 연출 없이
	 * 배리어를 즉시 놓아 보상을 열고, 패배만 결과 영상을 재생한다.
	 *
	 * 옛 HUD 가 이 배리어의 유일한 주인이었다. 그래서 옛것을 지우기 전에
	 * 이쪽부터 옮겨야 했다. 안 옮기고 지웠으면 이겨도 아무 일이 안 일어난다.
	 */
	void BeginCombatResultPresentation(TSharedPtr<FPresentationBarrier> Barrier, bool IsPlayerWin);
	USoundBase* SelectCombatResultJingle(bool bPlayerWin) const;
	void StartCombatResultCinematic();
	void EnsureCombatResultWidgets();
	void HandleCombatResultVideoFinished(class UCinematicWidget* CinematicWidget);
	UFUNCTION() void HandleCombatResultOpenRequested();
	UFUNCTION() void HandleCombatResultRewardConfirmed();
	UFUNCTION() void HandleRewardConcept03Completed(int32 ArtifactIndex);
	UFUNCTION() void HandleCombatRewardClaimConfirmed(ERewardClaimKind ClaimKind, int32 ChoiceIndex);
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

	/* 미리보기 로그는 예측 전용 모델(USimulationPreviewUIModel)에서 따로 받는다.
	 * 실전 로그와 저장 자리가 달라, 미리보기를 버릴 때 실전 표시가 같이 지워지지 않는다. */
	/** @brief 미리보기 배치 수신. 이전 미리보기 표시를 걷고 새 배치를 즉시 스폰한다. */
	UFUNCTION() void HandleSimulationPreviewBatch(FCombatEventBatchUI Batch);
	/** @brief 미리보기가 통째로 버려졌다. 미리보기 표시만 걷는다(실전 로그는 그대로). */
	UFUNCTION() void HandleSimulationPreviewCleared();
	/** @brief 대기 큐·화면에서 mIsPreview==true 인 로그만 퇴장시킨다(전체 클리어의 미리보기판). */
	void RetireSimulationPreviewFloatingLogs();
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

	/* ── 라운드 시작 고지 ─────────────────────────────────────────────── */
	/**
	 * @brief 라운드 시작 배너를 틀지. 격투게임식 라운드 고지를 기본으로 켠다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Layout")
	bool mPlayRoundBanner = true;

	UPROPERTY(Transient) TObjectPtr<class UButton> mTurnChangeInputBlocker;

	/* ── 전투/턴 시작 고지 ───────────────────────────────────────────────
	 * 전투 시작, 라운드, 유닛 턴을 각 프레젠테이션 배리어로 직렬화한다.
	 * 특히 턴 고지가 끝나기 전에는 스킬 UI를 열지 않는다.
	 */
	enum class ECombatAnnouncementKind : uint8
	{
		None,
		CombatStart,
		RoundStart,
		TurnStart
	};
	void EnsureCombatAnnouncementWidgets();
	bool PlayCombatAnnouncement(const FText& Text, ECombatAnnouncementKind Kind,
		TSharedPtr<FPresentationBarrier> Barrier);
	void FinishCombatAnnouncement();
	void CompleteTurnPresentationBegin();
	void UpdateCombatAnnouncement(float DeltaTime);
	FText GetCurrentTurnAnnouncementText() const;

	UPROPERTY(Transient) TObjectPtr<class UBorder> mCombatAnnouncementRoot;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> mCombatAnnouncementText;
	TSharedPtr<FPresentationBarrier> mCombatAnnouncementBarrier;
	FTimerHandle mCombatAnnouncementTimerHandle;
	ECombatAnnouncementKind mCombatAnnouncementKind = ECombatAnnouncementKind::None;
	float mCombatAnnouncementElapsed = 0.0f;
	float mCombatAnnouncementDuration = 1.2f;
	bool mCombatAnnouncementPlaying = false;
	bool mInitialFocusAnchorRegistered = false;
	/** @brief 마지막으로 앵커를 등록했을 때의 판 크기. 크기가 바뀌면 다시 등록한다. */
	FVector2D mLastFocusAnchorLocalSize = FVector2D::ZeroVector;

	/* ── 스킬 실행 직전 컷인 ────────────────────── */
	void HandlePrePlaySkillCutIn(
		const FCombatSkillCutInRequest& Request,
		TSharedPtr<FPresentationBarrier> Barrier);
	bool EnsureSkillCutInWidget();
	void FinishSkillCutIn();

	UPROPERTY(Transient)
	TObjectPtr<class USkillCutInWidget> mSkillCutInWidget;
	TSharedPtr<FPresentationBarrier> mSkillCutInBarrier;
	TArray<TSharedPtr<FPresentationBarrier>> mOverlappingSkillCutInBarriers;
	FTimerHandle mSkillCutInSafetyTimerHandle;
	bool mSkillCutInPlaying = false;

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

	// [RDBOT] 직전에 내보낸 텔레메트리 줄. 같으면 다시 찍지 않는다(매 틱 도배 방지).
	FString mLastBotTelemetry;

	UPROPERTY(Transient) TArray<FCombatUnitHpBarWidget> mUnitHpBars;
	UPROPERTY(Transient) TObjectPtr<class UCanvasPanel> mRootCanvas;
	UPROPERTY() TSubclassOf<UUserWidget> mUnitHpBarWidgetClass;
	UPROPERTY() TObjectPtr<UTexture2D> mUnitHpFillRedTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mUnitHpFillGreenTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mUnitDefenseIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mUnitStatusSlotTexture;
	/** @brief 버프/디버프 프레임 글로우(흰색 원본, 런타임에 금/보라로 물들인다). */
	UPROPERTY() TObjectPtr<UTexture2D> mUnitHpGlowTexture;

	/** @brief 상태이상 딱지 그림. 전용 그림이 없는 태그는 빈 칸으로 둔다. */
	/**
	 * @brief 이동 커맨드 그림.
	 *
	 * 이동은 스킬 표에 없어서 아이콘이 늘 비어 있었고, 그 칸이 흰 사각으로
	 * 떠 있었다. 런타임에는 그림을 못 불러오므로(PR#300) 여기서 잡아 둔다.
	 */
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mCommandMoveIconTexture;

	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconHpDamage;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconHpRecovery;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconGetMove;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconGetDefense;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconVigor;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconFortification;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconVulnerability;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mLogIconWeakness;
	/** @brief 플로팅 로그 글꼴(F_HUD_Oswald). 없으면 엔진 기본 글꼴로 남는다. */
	UPROPERTY(Transient) TObjectPtr<class UFont> mFloatingLogFont;

	/* ── 상세 패널 (롱프레스 정보) ──────────────────────────────────────
	 *
	 * 판의 유닛을 길게 누르면 유닛 상세(적이면 위협 범위도 판에 칠린다),
	 * 카드를 길게 누르면 스킬 상세가 뜬다.
	 *
	 * 패널은 **정보만 보여 주는 겹**(HitTestInvisible)이다. 눌림을 받으면
	 * 화면 전체를 덮는 한 장이 되어 지도가 겪던 문제(OpenUI 주석)를 반복한다.
	 * 닫는 탭은 HUD(HandleBoardPressed)가 받아 처리한다 -- 아무 데나 톡 치면
	 * 닫힌다.
	 */
	/*
	 * 풍부한 스킬 상세 렌더(수치 메달·범위 버튼·전술 WBP)는 프레젠터가 맡는다
	 * (SkillDetailOverlayPresenter). HUD 는 겹 클래스/그림을 생성자에서 물어
	 * 프레젠터에 넘기고, UIModel 을 읽어야 하는 유닛/상태/아티팩트 상세만
	 * 직접 그린다 -- 그때도 겹 부품은 프레젠터가 캐시한 같은 것을 쓴다.
	 */
	/** @brief 프레젠터를 한 번 만들고 설정을 최신으로 맞춘다. */
	bool EnsureDetailPresenter();
	/** @brief AP/쿨타임 배지의 실제 HUD 브러시를 프레젠터에 알려 주는 조회부. */
	UTexture2D* ResolveDetailStatTexture(FName SourceWidgetName, UTexture2D* Fallback);
	bool EnsureDetailOverlayWidget();
	void ShowUnitInspection();
	void ShowUnitDetailOverlay();
	void ShowSkillDetailOverlay();
	/** 스킬과 이동이 동일한 상세 WBP 렌더 경로를 사용하도록 하는 공용 함수. */
	void ShowSkillDetailOverlay(const FSkillDetailUI& Detail);

	/** @brief 스킬 상세 프레젠터. 겹 위젯과 그 부품의 실소유자다. */
	UPROPERTY(Transient) TObjectPtr<USkillDetailOverlayPresenter> mDetailPresenter;

	/**
	 * @brief 이동 카드의 상세를 띄운다.
	 *
	 * @details
	 * 이동에는 스킬 데이터가 없어 게임플레이에 청할 것이 없다. 대신 이미 내려와
	 * 있는 값(지금 차례인 유닛의 남은 행동력)으로 화면이 조립한다 -- 칸당 비용을
	 * 화면이 쥐는 것은 카드에 적는 "AP 1/칸"과 같은 자리라 새 규칙이 아니다.
	 */
	void ShowMoveDetailOverlay();

	/**
	 * @brief 상세 패널을 닫는다.
	 * @param bNotifyGameplay 참이면 위협 범위 칠도 걷으라고 게임플레이에 알린다.
	 *        턴 전환처럼 게임플레이가 스스로 걷는 자리에서는 거짓으로 닫는다.
	 */
	void HideDetailOverlay(bool bNotifyGameplay);

	/**
	 * @brief 요약판의 상태 아이콘을 오래 누르면 뜨는 상태이상 상세.
	 *
	 * 스킬 상세와 같은 판을 쓴다: 제목 띠에 상태 이름, 소켓에 상태 아이콘,
	 * 설명 칸에 효과 글. 격자·칩은 걷는다.
	 */
	void ShowStatusDetailOverlay(const FGameplayTag& StatusTag, int32 StackCount);
	void HandleStatusClicked(bool bAlly, int32 SlotIndex);
	void BeginStatusPress(bool bAlly, int32 SlotIndex);
	void EndStatusPress(bool bAlly, int32 SlotIndex);
	void HandleStatusLongPress(bool bAlly, int32 SlotIndex);
	void CancelStatusPress();
	UFUNCTION() void HandleAllyStatusPressed_0();
	UFUNCTION() void HandleAllyStatusPressed_1();
	UFUNCTION() void HandleAllyStatusPressed_2();
	UFUNCTION() void HandleEnemyStatusPressed_0();
	UFUNCTION() void HandleEnemyStatusPressed_1();
	UFUNCTION() void HandleEnemyStatusPressed_2();
	UFUNCTION() void HandleAllyStatusReleased_0();
	UFUNCTION() void HandleAllyStatusReleased_1();
	UFUNCTION() void HandleAllyStatusReleased_2();
	UFUNCTION() void HandleEnemyStatusReleased_0();
	UFUNCTION() void HandleEnemyStatusReleased_1();
	UFUNCTION() void HandleEnemyStatusReleased_2();

	/** @brief 상세 판의 명시적인 닫기 버튼 처리. */
	UFUNCTION() void HandleDetailCloseCatchClicked();

	/** @brief 스킬을 보는 동안 그 스킬 주인을 화면 가운데로 데려온다. */
	void FocusCameraOnTurnUnit();

	/** @brief 카드 고리 가운데 자리(0~1 비율). 카메라 초점이 놓일 곳. */
	FVector2D ComputeCommandRingAnchor() const;

	/**
	 * @brief 이 유닛을 화면 가운데로 데려오라고 청한다.
	 * @param bWithCommandRing 스킬 카드가 함께 뜨는 경우 true -- 그때만 카드
	 *        고리 가운데로 세부조정한다. 카드가 안 뜨면 화면 한가운데(0807).
	 */
	void RequestCameraFocus(int32 UnitId, bool bWithCommandRing);

	/**
	 * @brief 다음 유닛 상세 응답으로는 상세창을 띄우지 않는다.
	 *
	 * @details 카드를 그 용병 것으로 갈아 끼우려면 InspectUnit 을 청해야 하는데,
	 * 그 응답이 상세창까지 함께 연다. 판에서 아군을 누르거나 턴 칸을 누른
	 * 손은 카드만 원한 손이다 -- 상세창은 길게 눌러 살펴볼 때만 뜬다(0806).
	 */
	bool mSuppressNextUnitDetailOverlay = false;

	/**
	 * @brief 턴 칸을 눌렀다. 스킬 UI를 건드리지 않고 그 유닛만 화면 가운데로 잡는다.
	 *
	 * @details 판 위의 유닛은 작고 매 턴 자리가 바뀐다. 턴 칸은 늘 같은 자리에
	 * 있어 "이 사람을 찾겠다" 는 손이 여기서 나온다. 행동 선택은 전투판의
	 * 유닛을 직접 누르는 흐름과 분리한다.
	 */
	void HandleTurnTokenClicked(int32 SlotIndex);
	UFUNCTION() void HandleTurnTokenClicked_0();
	UFUNCTION() void HandleTurnTokenClicked_1();
	UFUNCTION() void HandleTurnTokenClicked_2();
	UFUNCTION() void HandleTurnTokenClicked_3();
	UFUNCTION() void HandleTurnTokenClicked_4();
	UFUNCTION() void HandleTurnTokenClicked_5();
	UFUNCTION() void HandleTurnTokenClicked_6();
	UFUNCTION() void HandleTurnTokenClicked_7();
	UFUNCTION() void HandleTurnTokenClicked_8();
	UFUNCTION() void HandleTurnTokenClicked_9();

	/** @brief 턴 칸마다 지금 누구를 그리고 있는지. 칸 클릭이 이걸 읽는다. */
	TArray<int32> mTurnSlotUnitIds;

	/** @brief 적 요약판의 다음 스킬 소켓 클릭 → 그 스킬 상세. */
	UFUNCTION() void HandleEnemyNextSkillClicked();
	int32 mEnemyShownUnitId = INDEX_NONE;
	int32 mEnemyShownNextSkillIndex = INDEX_NONE;

	/** @brief 용병탭 스킬 소켓 클릭 → 스킬 상세 (0번은 이동 상세). */
	void HandleMercenarySkillClicked(int32 SlotIndex);
	UFUNCTION() void HandleMercenarySkillClicked_0();
	UFUNCTION() void HandleMercenarySkillClicked_1();
	UFUNCTION() void HandleMercenarySkillClicked_2();
	UFUNCTION() void HandleMercenarySkillClicked_3();
	UFUNCTION() void HandleMercenarySkillClicked_4();
	UFUNCTION() void HandleMercenarySkillClicked_5();

	/** @brief 용병 목록에서 고른 줄. INDEX_NONE 이면 지금 차례인 용병을 본다. */
	int32 mMercenarySelectedSlot = INDEX_NONE;

	// 상태 아이콘 긴 누름이 어느 상태를 가리키는지 -- 요약판 갱신 때 채운다.
	TArray<FStatusEffectUI> mAllyShownStatuses;
	TArray<FStatusEffectUI> mEnemyShownStatuses;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> mAllyStatusButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> mEnemyStatusButtons;
	FTimerHandle mStatusLongPressTimerHandle;
	int32 mStatusPressedSlot = INDEX_NONE;
	bool mStatusPressedAlly = false;
	bool mStatusPressActive = false;

	/** @brief 상세 패널 WBP(WBP_CombatDetailOverlay). 이름으로 찾고 없는 것은 건너뛴다. */
	UPROPERTY() TSubclassOf<UUserWidget> mDetailOverlayWidgetClass;
	/**
	 * @brief 프레젠터가 만든 겹 인스턴스의 비친 포인터.
	 *
	 * 소유·생성·배선은 프레젠터가 한다. HUD 에 남은 유닛/상태/아티팩트 상세
	 * 경로가 예전 코드 그대로 읽도록 EnsureDetailOverlayWidget() 이 비춘다.
	 */
	UPROPERTY(Transient) TObjectPtr<UUserWidget> mDetailOverlayWidget;

	/* ── 몬스터 탭 (WBP_MonsterTab_Marchbound) ──────────────────────────
	 *
	 * 상단 몬스터 메뉴로 여닫는 전체 화면 모달이다. 행 3칸은 살아 있는 적을
	 * 나온 차례대로 채우고, 행을 누르면 오른쪽 상세가 그 몬스터로 바뀐다.
	 * 목록/스탯은 FUnitUI 값으로 바로 채우고, 스킬 이름은 RequestInspectUnit
	 * 응답(FUnitDetailUI)이 채운다 -- 상세 응답이 탭이 열린 동안 오면 PR457
	 * 상세 겹 대신 이 탭이 받는다.
	 */
	bool EnsureMonsterTabWidget();
	void RefreshMonsterTab();
	void RefreshMonsterTabDetail();
	void BeginMonsterSkillPress(int32 SlotIndex);
	void EndMonsterSkillPress(int32 SlotIndex);
	void HandleMonsterSkillLongPress(int32 SlotIndex);
	void HandleMonsterSkillClicked(int32 SlotIndex);
	void CancelMonsterSkillPress();
	UFUNCTION() void HandleMonsterSkillClicked_0();
	UFUNCTION() void HandleMonsterSkillClicked_1();
	UFUNCTION() void HandleMonsterSkillClicked_2();
	UFUNCTION() void HandleMonsterSkillClicked_3();
	UFUNCTION() void HandleMonsterSkillPressed_0();
	UFUNCTION() void HandleMonsterSkillPressed_1();
	UFUNCTION() void HandleMonsterSkillPressed_2();
	UFUNCTION() void HandleMonsterSkillPressed_3();
	UFUNCTION() void HandleMonsterSkillReleased_0();
	UFUNCTION() void HandleMonsterSkillReleased_1();
	UFUNCTION() void HandleMonsterSkillReleased_2();
	UFUNCTION() void HandleMonsterSkillReleased_3();
	static constexpr int32 MonsterTabViewportZOrder = 55;
	static constexpr int32 DetailOverlayViewportZOrder = 70;
	UPROPERTY() TSubclassOf<UUserWidget> mMonsterTabWidgetClass;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> mMonsterTabWidget;
	/** @brief 행 순서대로 담은 표시 대상 적 id. 클릭 행→유닛 매핑의 단일 출처. */
	TArray<int32> mMonsterTabUnitIds;
	int32 mMonsterTabSelectedRow = 0;
	/** @brief 마지막으로 상세를 청한 몬스터 id. 유닛 갱신마다 재요청하지 않기 위한 가드. */
	int32 mMonsterTabInspectedUnitId = INDEX_NONE;
	/** @brief 보이는 슬롯→FUnitDetailSkillUI::mSkillIndex 왕복 식별자. */
	TArray<int32> mMonsterTabSkillIndices;
	FTimerHandle mMonsterSkillLongPressTimerHandle;
	int32 mMonsterSkillPressedSlot = INDEX_NONE;
	bool mMonsterSkillPressActive = false;
	// 아래 넷도 프레젠터 캐시의 비친 포인터다. 값 채우기는 HUD 상세 경로가 한다.
	UPROPERTY(Transient) TObjectPtr<UImage> mDetailIconImage;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mDetailTitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mDetailSubtitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mDetailBodyText;
	/** 장문 한글 상세 전용 OFL 폰트. 쿡 유지용 하드 참조로 들고 프레젠터에 넘긴다. */
	UPROPERTY() TObjectPtr<class UFont> mReadableDetailFont;
	/** @brief 프레젠터의 서체 전환으로 위임한다. 유닛/상태 상세가 계속 부른다. */
	void ApplyReadableDetailTypography(bool bReadable);
	#if WITH_DEV_AUTOMATION_TESTS
	FSkillDetailUI mRenderedSkillDetailForTest;
	#endif

	/* ── 상세창 수치 칩과 범위 그림 ──────────────────────────────────────
	 *
	 * 예전에는 "AP 1 · 쿨타임 2턴 · 피해 6~10" 을 부제 한 줄에 이어 붙였다.
	 * 눈이 구분자를 세어야 했고, 사거리는 글자로만 적혀 있어 십자가 어느 칸까지
	 * 인지 머릿속으로 그려야 했다. 칩으로 끊고 칸으로 그린다.
	 *
	 * 값은 전부 UI 모델(FSkillUI/FSkillTargetingUI/FUnitUI)에서 읽는다.
	 * 게임모드를 직접 보지 않는다 -- PR#426 의 UI-GameMode 분리 규칙이다.
	 *
	 * 칩/격자의 실제 그리기는 프레젠터로 옮겼다. 아래는 남은 상세 경로용
	 * 위임 래퍼다.
	 */
	void SetDetailChip(int32 ChipSlot, const FText& Label, const FText& Value);
	void ClearDetailChips();
	void ClearDetailGrids();

	/* ── 이미지 시안 기반 통합 스킬 미리보기 ─────────────────────────────
	 * 수치 메달·범위 버튼·전술 WBP 는 프레젠터가 짓고 갱신한다. HUD 에는
	 * 실험용 SceneCapture 경로만 남는다 -- 월드 카메라를 만지는 일이라
	 * DTO 만 보는 프레젠터로 옮기지 않았다.
	 */
	/** @brief 메인 전투 카메라와 같은 장면을 렌더 타깃으로 받아 상세 WBP에 붙인다. */
	bool StartSkillWorldPreview();
	/** @brief 전투 카메라 이동/확대 상태를 상세창의 캡처 카메라에 동기화한다. */
	void SyncSkillWorldPreviewCamera(bool bCaptureImmediately = false);
	/** @brief 상세창을 닫을 때 캡처를 멈춘다. 액터/렌더 타깃은 재사용한다. */
	void StopSkillWorldPreview();
	/** @brief HUD 파괴 시 런타임 캡처 리소스를 완전히 해제한다. */
	void ReleaseSkillWorldPreview();

	/** 첨부 시안형 범위 디오라마 클래스. 쿡 유지용으로 들고 프레젠터에 넘긴다. */
	UPROPERTY() TSubclassOf<class USkillTacticalDiagramWidget>
		mSkillTacticalDiagramWidgetClass;
	/** 프레젠터가 만든 범위판 인스턴스의 비친 포인터. 시험 접근자용. */
	UPROPERTY(Transient) TObjectPtr<USkillTacticalDiagramWidget>
		mSkillTacticalDiagramWidget;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> mSkillDetailContentWidget;
	/** 실제 전투 장면이 표시되는 WBP Image(프레젠터 소유의 비친 포인터).
	 * SceneCapture2D의 RenderTarget을 브러시로 쓴다. */
	UPROPERTY(Transient) TObjectPtr<UImage> mSkillWorldPreviewImage;
	UPROPERTY(Transient) TObjectPtr<class UTextureRenderTarget2D> mSkillWorldPreviewRenderTarget;
	UPROPERTY(Transient) TObjectPtr<class ASceneCapture2D> mSkillWorldPreviewCapture;
	UPROPERTY(Transient) TObjectPtr<class UCameraComponent> mSkillWorldPreviewSourceCamera;
	bool mSkillWorldPreviewActive = false;

	// 생성자 하드 참조라 패키징에서도 통합 미리보기 부품이 빠지지 않는다.
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualRingTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualCellNormalTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualCellSelectedTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualAPIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualDamageIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualCooldownIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualCriticalIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualCasterIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualTargetIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillRangeButtonTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillRangeButtonSelectedTexture;

	/* ── 오른쪽 열의 세 덩어리 ─────────────────────────────────────────
	 *
	 * 사거리 칸은 스킬 상세만, 스킬 칸은 유닛 상세만, 효과·조작 글은 아티팩트와
	 * 이동만 쓴다. 실제 켜고 끄기와 열 재배치는 프레젠터가 맡고, 아래는 남은
	 * 상세 경로용 위임 래퍼다.
	 */
	/** @brief 세 덩어리 중 하나만 켠다. 나머지는 끈다. */
	void ShowDetailRightBlock(const UWidget* Wanted);
	/** @brief 상세 종류에 맞춰 3열 또는 아티팩트 전용 2열로 실제 열을 재배치한다. */
	void ApplyDetailColumnLayout(bool bArtifactTwoColumn);

	/* 아래는 프레젠터 캐시의 비친 포인터. 유닛/상태/아티팩트 상세가 읽는다. */
	/** @brief 수치 칩 묶음. 아티팩트에는 칩이 없어 통째로 끈다. */
	UPROPERTY(Transient) TObjectPtr<UWidget> mDetailStatBlock;
	UPROPERTY(Transient) TObjectPtr<UWidget> mDetailSkillBlock;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mDetailExtraHeading;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mDetailExtraText;
	UPROPERTY(Transient) TObjectPtr<UWidget> mDetailDivider0;
	UPROPERTY(Transient) TObjectPtr<UWidget> mDetailDivider1;

	/* ── 상세창 스킬 칸 ─────────────────────────────────────────────────
	 *
	 * 유닛 상세창 아래에 그 유닛의 스킬을 늘어놓고, 탭하면 그 스킬 상세로
	 * 넘어간다. 적을 살펴보는 이유의 절반이 "무엇으로 때리나" 라서다.
	 *
	 * **칸은 C++ 가 짓는다.** 스킬 수가 유닛마다 달라 WBP 에 미리 놓을 수가
	 * 없다 -- 머리 위 HP 바와 같은 이유다.
	 */
	static constexpr int32 DetailSkillSlotCount = 6;

	/** @brief 스킬 칸 줄을 상세 패널 안에 한 번만 짓는다. */
	void BuildDetailSkillRow();
	/** @brief 지금 상세 스냅샷의 스킬로 칸을 채운다. 남는 칸은 접는다. */
	void RefreshDetailSkillRow();
	void SetDetailSkillRowShown(bool bShown);
	void HandleDetailSkillClicked(int32 IconIndex);

	UFUNCTION() void HandleDetailSkillClicked_0();
	UFUNCTION() void HandleDetailSkillClicked_1();
	UFUNCTION() void HandleDetailSkillClicked_2();
	UFUNCTION() void HandleDetailSkillClicked_3();
	UFUNCTION() void HandleDetailSkillClicked_4();
	UFUNCTION() void HandleDetailSkillClicked_5();

	/**
	 * @brief 스킬 칸을 담은 것. 판이 칸을 미리 만들어 뒀으면 그 호스트 캔버스이고,
	 *        옛 판이면 여기서 만든 가로 상자다. 통째로 껐다 켜는 데만 쓴다.
	 */
	UPROPERTY(Transient) TObjectPtr<class UWidget> mDetailSkillRow;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> mDetailSkillButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> mDetailSkillIcons;
	/** @brief 그림이 없는 스킬은 이름을 적어 준다. 몬스터 스킬은 아이콘이 없는 것이 많다. */
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> mDetailSkillLabels;
	/** @brief 칸 -> 그 유닛의 스킬 index. 빈 칸은 INDEX_NONE. */
	TArray<int32> mDetailSkillIndices;

	/** @brief 승리 뒤 다음 스테이지로 진행 요청한다. */
	void RequestToEnterNextStage();
	/** @brief 승리 뒤 다음 방을 고르도록 지도를 연다. */
	void OpenWorldMapForNextRoom();
	/** @brief 승리 후 지도 BACK을 처리하고 보상판은 닫힌 상태로 유지한다. */
	void CloseWorldMapAfterVictory();

	/** @brief 승리 지도 BACK 뒤 전투 HUD와 입력을 빈 화면 없이 복원한다. */
	void RestorePostVictoryHUDAndInput();

	/** @brief 전투 중 현재 런 지도를 방 선택 없이 조회용으로 연다. */
	void OpenWorldMapForCombatReview();

	/** @brief 조회용 지도를 닫고 전투 HUD와 입력 모드를 복원한다. */
	void CloseWorldMapForCombatReview();

	/** @brief 지도 닫기 완료 뒤 전투 입력을 안전하게 되돌린다. */
	void RestoreCombatInputAfterWorldMap();

	/** @brief 조회 지도는 닫고, 승리 지도 BACK은 전투 HUD를 복구한다. */
	UFUNCTION()
	void HandleWorldMapCloseRequested();

	/** @brief 결과 화면이 뜬 동안 조작을 감춘다. */
	void SetCombatControlsShown(bool bShown);

	/** @brief 전투가 끝났다. 전원 쓰러짐 연출을 기다린 뒤 승리/패배 결과를 시작한다. */
	void HandleEndCombatUI(TSharedPtr<FPresentationBarrier> Barrier);

	UPROPERTY(Transient) TObjectPtr<class UCinematicWidget> mCombatResultCinematicWidget;
	UPROPERTY(Transient) TObjectPtr<class UCombatResultOverlayWidget> mCombatResultOverlayWidget;
	TSharedPtr<FPresentationBarrier> mCombatResultBarrier;
	FTimerHandle mCombatResultStartDelayTimerHandle;
	bool mIsPlayerWin = false;
	bool mCombatResultFlowActive = false;
	/** 승리 보상 완료 뒤 다음 방을 고를 때까지 조회용 지도 진입을 막는다. */
	bool mVictoryWorldMapLocked = false;

	/** @brief 마지막 사망 판정 뒤 쓰러짐 애니메이션을 보여 줄 최소 시간. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Result", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float mDeathAnimationResultDelaySeconds = 1.8f;

	/** @brief 공용 서브시스템 초기화가 늦은 경로에서 조회 지도를 직접 만들 때 쓸 클래스. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Navigation")
	TSubclassOf<class UFrontendMapWidget> mWorldMapWidgetClass;

	/** @brief MAP 메뉴로 연 조회용 지도. 승리 후 방 선택 지도와 상태를 구분한다. */
	UPROPERTY(Transient) TObjectPtr<class UFrontendMapWidget> mCombatReviewWorldMap;
	/** @brief 승리 BACK 후에도 같은 다음 방 선택 지도를 다시 열기 위한 인스턴스. */
	UPROPERTY(Transient) TObjectPtr<class UFrontendMapWidget> mVictoryWorldMap;
	bool mCombatReviewWorldMapOpen = false;
	bool mShowMouseCursorBeforeCombatReviewMap = true;

	/** @brief 패배 화면 WBP. 하드 레퍼런스로 들어야 Cook에서 안 빠진다. */
	UPROPERTY()
	TSubclassOf<class UCombatResultOverlayWidget> mDefeatWidgetClass;

	/** @brief 보상 화면. 결과 영상이 끝나면 그 위에 뜬다. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Result")
	TSubclassOf<class URewardSettlementWidgetBase> mRewardWidgetClass;
	UPROPERTY(Transient) TObjectPtr<class URewardSettlementWidgetBase> mCombatRewardWidget;
	/** 실제 승리 플로우에서 사용하는 신규 RewardConcept03 WBP 인스턴스. */
	UPROPERTY(Transient) TObjectPtr<class URewardConcept03Widget> mCombatRewardConceptWidget;
	UPROPERTY(Transient) TObjectPtr<class URewardUIModel> mCombatRewardUIModel;

	/** @brief 경험치가 차오를 때 나는 소리. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Result")
	TObjectPtr<USoundBase> mExpGainSound;

	/** @brief 검증된 승리 징글. 패배 음원은 검증된 자산이 생길 때까지 재생하지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Result")
	TObjectPtr<USoundBase> mVictoryJingleSound;

	/** @brief 확정 단추와 턴 종료 글자를 지금 단계에 맞춘다. */
	void RefreshActionButtons();

	/* ── 쓸 행동력 미리 보이기 ────────────────────────────────────────────
	 *
	 * 카드를 고르면 그 몫만큼 칸이 숨쉬듯 빛난다. 쓰면 뒤부터 없어지므로
	 * 뒤에서부터 빛낸다.
	 *
	 * 스킬 비용과 이동 경로 비용은 게임플레이가 pending action으로 내려 준다.
	 * 화면은 경로나 스킬 데이터를 다시 계산하지 않는다.
	 */
	int32 GetPendingActionCost() const;
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
		/** @brief 새 용병탭의 가로형 일반/선택 카드 프레임. */
		TObjectPtr<UImage> Plate;
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
		/** @brief TurnPanel의 직계 형제인 투명 클릭 받이. */
		TObjectPtr<UButton> Button;
		TObjectPtr<UImage> Portrait;
		TObjectPtr<UTextBlock> Name;
		TObjectPtr<UWidget> Current;
		/** @brief 이 슬롯 앞에서 라운드가 바뀔 때만 보이는 세로 막대와 R# 표기. */
		TObjectPtr<UWidget> RoundDivider;
		TObjectPtr<UTextBlock> RoundLabel;
	};

	/** @brief 빈 아군 칸을 접지 않고 "비어 있음"으로 그린다. */
	void ClearPartySlot(const FPartySlotWidgets& Widgets);

	TArray<FPartySlotWidgets> mPartySlots;
	TArray<FCommandSlotWidgets> mCommandSlots;
	TArray<FTurnSlotWidgets> mTurnSlots;

	/** @brief 현재 라운드를 카드와 분리해 표시하는 왼쪽 상단 패널. */
	TObjectPtr<UWidget> mRoundPanel;
	TObjectPtr<UTextBlock> mRoundText;
	/** @brief ROUND 배지 아래의 큰 두 자리 라운드 숫자("01"). 구형 WBP 면 없다. */
	TObjectPtr<UTextBlock> mRoundNumberText;
	TObjectPtr<UTextBlock> mObjectiveText;

	TObjectPtr<UWidget> mEnemyPanel;
	TObjectPtr<UImage> mEnemyPortrait;
	TObjectPtr<UTextBlock> mEnemyName;
	TObjectPtr<UProgressBar> mEnemyHPBar;
	TObjectPtr<UTextBlock> mEnemyHPText;
	TObjectPtr<UTextBlock> mEnemyAPText;
	TObjectPtr<UTextBlock> mEnemyCritText;
	TObjectPtr<UTextBlock> mEnemySpeedText;
	TObjectPtr<UTextBlock> mEnemyStatusText;
	TObjectPtr<UTextBlock> mEnemyForecastText;
	TObjectPtr<UWidget> mEnemyNextSkillFrame;
	TObjectPtr<UImage> mEnemyNextSkillIcon;
	TArray<TObjectPtr<UWidget>> mEnemyStatusFrames;
	TArray<TObjectPtr<UImage>> mEnemyStatusIcons;
	TArray<TObjectPtr<UTextBlock>> mEnemyStatusCounts;

	TObjectPtr<UWidget> mAllyPanel;
	TObjectPtr<UImage> mAllyPortrait;
	TObjectPtr<UTextBlock> mAllyName;
	TObjectPtr<UProgressBar> mAllyHPBar;
	TObjectPtr<UTextBlock> mAllyHPText;
	TObjectPtr<UTextBlock> mAllyAPText;
	TObjectPtr<UTextBlock> mAllySpeedText;
	TObjectPtr<UTextBlock> mAllyStatusText;
	TArray<TObjectPtr<UWidget>> mAllyStatusFrames;
	TArray<TObjectPtr<UImage>> mAllyStatusIcons;
	TArray<TObjectPtr<UTextBlock>> mAllyStatusCounts;

	TObjectPtr<UButton> mEndTurnButton;
	TObjectPtr<UButton> mSkillToggleButton;
	TObjectPtr<UWidget> mSkillTogglePlate;
	TObjectPtr<UWidget> mSkillToggleLabel;

	/** @brief 미리보기용 가짜 전투 드라이버. 실제 전투에서는 null이다. */
	UPROPERTY(Transient) TObjectPtr<UMockCombatDriver> mPreviewDriver;

	/** @brief 캐시가 끝났는지. NativeConstruct에서 한 번만 돈다. */
	bool mCached = false;
};
