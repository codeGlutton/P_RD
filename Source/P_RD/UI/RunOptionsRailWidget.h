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
 * 상점/지도처럼 전투 HUD가 없는 방에서도 같은 상단 메뉴를 제공한다.
 * 외형은 전투 HUD의 동일 텍스처와 좌표를 쓰고, 용병 버튼은
 * WBP_CombatHUD04의 인라인 MercenaryPanel을 GameMode가 내려준
 * View(FPartyRosterView/FInventoryView)로 채운다 -- 위젯은 게임플레이
 * 모델/AttributeSet/DA 를 직접 읽지 않는다 (PR #426 규칙).
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
	UPROPERTY(Transient) TArray<FInventoryArtifactView> ShownArtifactViews;
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
