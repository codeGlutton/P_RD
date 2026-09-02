// @file RunOptionsRailWidget.h
// @brief 전투 HUD의 지도·용병·몬스터·설정 레일을 방 공용 화면에서 재사용합니다.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/RoomViewTypes.h"
#include "RunOptionsRailWidget.generated.h"

class UButton;
class UFrontendMapWidget;
class UImage;
class USkillDetailOverlayPresenter;
class UTexture2D;

/**
 * @brief 설정 레일(지도·용병·몬스터·설정)의 저작 좌표. **여기가 유일한 원본이다.**
 *
 * @details 같은 레일을 두 곳이 짓는다 -- 전투 HUD는 편집기 빌더
 * (CombatHUDWidgetBuilder::RepairAuthoredRightHUDLayout)가 WBP에 굽고, 전투
 * 밖 방은 URunOptionsRailWidget이 런타임에 세운다. 전에는 숫자가 양쪽에
 * 따로 적혀 있어서, 레일을 1.2배로 키운 작업이 전투에만 반영되고 상점·지도는
 * 옛 크기로 남았다. 게다가 전투 쪽도 프레임과 아이콘만 키우고 **버튼 사각형은
 * 옛 좌표 그대로** 둬서 이웃 버튼끼리 겹쳤다 -- 아이콘 위에 커서를 올려도
 * 엉뚱한 칸이 잡혔다(0824 검수 3·4번).
 *
 * 그래서 좌표를 이 한 곳으로 모은다. 키울 일이 또 있으면 RailScale만 만진다.
 */
namespace RunOptionsRail
{
	/** @brief 저작 원본(470x173) 대비 배율. 0824 검수에서 1.2배로 확정. */
	inline constexpr float RailScale = 1.2f;

	inline constexpr float BaseWidth = 470.f;
	inline constexpr float BaseHeight = 173.f;
	/** @brief 화면에 놓이는 레일 크기. */
	inline constexpr float Width = BaseWidth * RailScale;
	inline constexpr float Height = BaseHeight * RailScale;

	/** @brief 네 칸 공통. 배열 순서는 지도·용병·몬스터·설정이다. */
	inline constexpr int32 SlotCount = 4;

	/** @brief 아이콘 좌상단(저작 원본 기준). 배율은 아래 헬퍼가 곱한다. */
	inline constexpr float BaseIconPositions[SlotCount][2] = {
		{ 45.f, 44.f }, { 154.f, 36.f }, { 248.f, 48.f }, { 348.f, 48.f } };
	inline constexpr float BaseIconSizes[SlotCount][2] = {
		{ 80.f, 81.f }, { 63.f, 96.f }, { 77.f, 83.f }, { 77.f, 80.f } };
	/** @brief 눌리는 사각형(저작 원본 기준). 서로 겹치지 않는다. */
	inline constexpr float BaseButtonPositions[SlotCount][2] = {
		{ 37.f, 31.f }, { 138.f, 31.f }, { 242.f, 31.f }, { 343.f, 31.f } };
	inline constexpr float BaseButtonSize[2] = { 94.f, 112.f };

	inline FVector2D IconPosition(const int32 Slot)
	{
		return FVector2D(BaseIconPositions[Slot][0],
			BaseIconPositions[Slot][1]) * RailScale;
	}
	inline FVector2D IconSize(const int32 Slot)
	{
		return FVector2D(BaseIconSizes[Slot][0],
			BaseIconSizes[Slot][1]) * RailScale;
	}
	inline FVector2D ButtonPosition(const int32 Slot)
	{
		return FVector2D(BaseButtonPositions[Slot][0],
			BaseButtonPositions[Slot][1]) * RailScale;
	}
	inline FVector2D ButtonSize()
	{
		return FVector2D(BaseButtonSize[0], BaseButtonSize[1]) * RailScale;
	}
}

/**
 * 상점/지도처럼 전투 HUD가 없는 방에서도 같은 상단 메뉴를 제공한다.
 * 외형은 전투 HUD의 동일 텍스처와 좌표를 쓰고, 용병 버튼은
 * WBP_CombatHUD04의 인라인 MercenaryPanel을 GameMode가 내려준
 * View(FPartyRosterView)로 채운다 -- 골드/아티팩트도 같은 판에 실려 오고,
 * 위젯은 게임플레이 모델/AttributeSet/DA 를 직접 읽지 않는다 (PR #426 규칙).
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API URunOptionsRailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URunOptionsRailWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 지도 자체에 붙은 레일이면 지도 버튼을 현재 탭으로 표시하고 입력을 막는다. */
	void SetMapContext(bool bInMapContext);

#if WITH_DEV_AUTOMATION_TESTS
	bool IsMapContextForTest() const { return bMapContext; }
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildRuntimeTree();
	void BindRailInputs();
	void OpenMercenaryPanel();
	void CloseMercenaryPanel();
	void RefreshMercenaryPanel();
	void RefreshInventoryPage();
	void SetInventoryPageShown(bool bShown);
	void ApplyMercenarySkillSlots();
	void SelectPartyMember(int32 MemberIndex);
	void ShowSkillDetail(int32 SlotIndex);
	void ShowArtifactDetail(int32 SlotIndex);
	void BeginDetailPress(bool bArtifact, int32 SlotIndex);
	void EndDetailPress();
	void FireHeldDetail();
	bool EnsureDetailOverlay();
	bool EnsureSkillDetailPresenter();
	void CloseDetailOverlay();
	void PresentDetail(const FText& Name, const FText& Subtitle,
		const FText& Description, UTexture2D* Icon, bool bArtifact);

	UFUNCTION() void HandleMapClicked();
	UFUNCTION() void HandleMapCloseRequested();
	UFUNCTION() void HandleMercenaryClicked();
	UFUNCTION() void HandleMonsterClicked();
	UFUNCTION() void HandleSettingsClicked();
	UFUNCTION() void HandleMercenaryCloseClicked();
	UFUNCTION() void HandleInventoryClicked();
	UFUNCTION() void HandleDetailCloseClicked();
	UFUNCTION() void HandlePartyClicked0();
	UFUNCTION() void HandlePartyClicked1();
	UFUNCTION() void HandlePartyClicked2();
	UFUNCTION() void HandleSkillClicked0();
	UFUNCTION() void HandleSkillClicked1();
	UFUNCTION() void HandleSkillClicked2();
	UFUNCTION() void HandleSkillClicked3();
	UFUNCTION() void HandleSkillClicked4();
	UFUNCTION() void HandleSkillClicked5();
	UFUNCTION() void HandleSkillPressed0();
	UFUNCTION() void HandleSkillPressed1();
	UFUNCTION() void HandleSkillPressed2();
	UFUNCTION() void HandleSkillPressed3();
	UFUNCTION() void HandleSkillPressed4();
	UFUNCTION() void HandleSkillPressed5();
	UFUNCTION() void HandleArtifactClicked0();
	UFUNCTION() void HandleArtifactClicked1();
	UFUNCTION() void HandleArtifactClicked2();
	UFUNCTION() void HandleArtifactClicked3();
	UFUNCTION() void HandleArtifactClicked4();
	UFUNCTION() void HandleArtifactClicked5();
	UFUNCTION() void HandleArtifactClicked6();
	UFUNCTION() void HandleArtifactPressed0();
	UFUNCTION() void HandleArtifactPressed1();
	UFUNCTION() void HandleArtifactPressed2();
	UFUNCTION() void HandleArtifactPressed3();
	UFUNCTION() void HandleArtifactPressed4();
	UFUNCTION() void HandleArtifactPressed5();
	UFUNCTION() void HandleArtifactPressed6();
	UFUNCTION() void HandleDetailReleased();

	UPROPERTY(Transient) TObjectPtr<UButton> MapButton;
	UPROPERTY(Transient) TObjectPtr<UButton> MercenaryButton;
	UPROPERTY(Transient) TObjectPtr<UButton> MonsterButton;
	UPROPERTY(Transient) TObjectPtr<UButton> SettingsButton;
	UPROPERTY(Transient) TObjectPtr<UImage> MapIcon;
	UPROPERTY(Transient) TObjectPtr<UFrontendMapWidget> OpenedMapWidget;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> MercenaryPanelWidget;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> DetailOverlayWidget;
	/** 전투와 같은 리치 스킬/아티팩트 상세를 그리는 공용 프레젠터. */
	UPROPERTY(Transient) TObjectPtr<USkillDetailOverlayPresenter> SkillDetailPresenter;
	/** GameMode가 내려준 파티 명단 View 복사본. 위젯은 모델을 직접 읽지 않는다. */
	UPROPERTY(Transient) FPartyRosterView PartyRoster;
	/** 선택 파티원의 표시 스킬(최대 6칸) View 복사본. */
	UPROPERTY(Transient) TArray<FPartyRosterSkillView> ShownSkillViews;
	/** ShownSkillViews와 나란히 가는 컴포넌트 원본 슬롯 index. 빈 슬롯 걸러내면 어긋나므로 따로 둔다. */
	UPROPERTY(Transient) TArray<int32> ShownSkillSlotIndices;
	/** 인벤토리 페이지의 표시 아티팩트(최대 7칸) View 복사본. */
	UPROPERTY(Transient) TArray<FPartyRosterArtifactView> ShownArtifactViews;
	/** ShownArtifactViews와 나란히 가는 파티 목록 원본 index. 상세 조립 API는 이 값으로 부른다. */
	UPROPERTY(Transient) TArray<int32> ShownArtifactIndices;

	UPROPERTY() TObjectPtr<UTexture2D> OptionsFrameTexture;
	UPROPERTY() TObjectPtr<UTexture2D> MapIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> MercenaryIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> MonsterIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> SettingsIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> MercenaryCardNormalTexture;
	UPROPERTY() TObjectPtr<UTexture2D> MercenaryCardSelectedTexture;

	bool bMapContext = false;
	bool bInventoryPageShown = false;
	int32 SelectedPartyMember = 0;
	FTimerHandle DetailLongPressTimer;
	int32 PressedDetailSlot = INDEX_NONE;
	bool bPressedDetailIsArtifact = false;
	bool bDetailLongPressTriggered = false;
	static constexpr float DetailLongPressSeconds = 0.45f;
};
