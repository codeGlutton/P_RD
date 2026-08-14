// @file ShopUIWidgetBase.h
// @brief 상점(런 중 상점방) 화면 WBP가 상속하는 베이스입니다. 뷰모델에 묶여 표시·입력만 담당합니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/Shop/ShopUITypes.h"
#include "ShopUIWidgetBase.generated.h"

class UButton;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UWidget;
class UWrapBox;
class UShopUIModel;
class UTexture2D;

/** @brief 상점 화면 WBP 베이스. WBP(create_shop_wbp.py 생성) 위젯 이름은 아래 BindWidget 멤버명과 일치해야 한다. */
// 이 베이스를 상속한 WBP는:
// - BindUIModel()로 UShopUIModel에 연결하면 C++가 BindWidget 위젯에 값을 채운다(슬롯=종류별 아이콘+이름+가격),
// - 슬롯 구매는 BuyItem(), 나가기는 Leave()로 의도만 보낸다.
// 위젯은 골드 차감/지급을 하지 않는다. 진실은 게임플레이(ShopGameMode)에 있다.
UCLASS(Abstract)
class P_RD_API UShopUIWidgetBase : public URDUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 상점 화면이 다른 UI 위에 뜨도록 팝업 ZOrder를 설정한다. */
	UShopUIWidgetBase(const FObjectInitializer& ObjectInitializer);

	/** @brief 상점 뷰모델에 연결하고 갱신 알림을 구독한다. */
	UFUNCTION(BlueprintCallable, Category = "Shop|UI")
	void BindUIModel(UShopUIModel* InUIModel);

	/** @brief WBP가 슬롯을 그릴 때 현재 UIModel을 읽기 위한 접근자. */
	UFUNCTION(BlueprintPure, Category = "Shop|UI")
	UShopUIModel* GetUIModel() const { return mUIModel; }

	/** @brief 슬롯 구매가 확정됐을 때 호출(보통 구매 확인 팝업 뒤). 의도만 뷰모델로 넘긴다. */
	UFUNCTION(BlueprintCallable, Category = "Shop|UI")
	void BuyItem(int32 SlotIndex);

	/** @brief 나가기 버튼이 호출. 의도만 뷰모델로 넘긴다. */
	UFUNCTION(BlueprintCallable, Category = "Shop|UI")
	void Leave();

protected:
	/** @brief 상점값이 들어왔을 때 호출. WBP가 추가 연출을 하고 싶으면 여기서 한다(선택). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Shop|UI")
	void OnShopRefreshed();

	/** @brief 나가기 버튼 클릭을 연결하고, 이미 바인딩된 상점이 있으면 즉시 그린다. */
	virtual void NativeConstruct() override;

	/** @brief 화면 이탈 시 UIModel 델리게이트 구독을 정리한다. */
	virtual void NativeDestruct() override;

	/**
	 * Presentation-only texture hooks. The default implementation preserves the
	 * legacy shop artwork; alternate WBP parents can replace chrome without
	 * changing the transaction model or the existing WBP asset.
	 */
	virtual UTexture2D* ResolveItemIcon(const FShopItemUI& Item) const;
	virtual UTexture2D* ResolveUnitIcon(EUnitJobType JobType) const;
	virtual UTexture2D* ResolveUnitSelectionPlate(bool bSelected) const;
	virtual UTexture2D* ResolveSkillSelectionPlate(bool bSelected) const;
	virtual UTexture2D* ResolveOwnedArtifactPlate() const;
	virtual UTexture2D* ResolveTabPlate(bool bSelected) const;

private:
	/** @brief 나가기 버튼 클릭 → Leave 의도 전달 후 화면을 닫는다. */
	UFUNCTION() void HandleCloseClicked();

	/** @brief UIModel 변경 알림을 받아 BindWidget 위젯에 값을 채운다. 거래 도메인 알림만 처리한다. */
	UFUNCTION() void HandleUIChanged(EShopUIDomain Domain);

	UFUNCTION() void HandleArtifactTabClicked();
	UFUNCTION() void HandleSkillTabClicked();
	UFUNCTION() void HandleRestTabClicked();
	UFUNCTION() void HandleRestClicked();
	UFUNCTION() void HandleInventoryClicked();
	UFUNCTION() void HandleArtifactInventoryCloseClicked();
	UFUNCTION() void HandlePreviousClicked();
	UFUNCTION() void HandleNextClicked();
	UFUNCTION() void HandleBuyClicked();
	UFUNCTION() void HandleRailClicked0();
	UFUNCTION() void HandleRailClicked1();
	UFUNCTION() void HandleRailClicked2();
	UFUNCTION() void HandleRailClicked3();
	UFUNCTION() void HandleRailClicked4();
	UFUNCTION() void HandleUnitClicked0();
	UFUNCTION() void HandleUnitClicked1();
	UFUNCTION() void HandleUnitClicked2();
	UFUNCTION() void HandleSkillSlotClicked0();
	UFUNCTION() void HandleSkillSlotClicked1();
	UFUNCTION() void HandleSkillSlotClicked2();
	UFUNCTION() void HandleSkillSlotClicked3();

	/** @brief 현재 UIModel 구독을 해제하고 참조를 비운다. */
	void UnbindUIModel();

	/** @brief 현재 모델의 골드/판매 슬롯을 BindWidget 위젯에 반영한다. */
	void RefreshView();

	/** @brief 확정된 가로형 상점 WBP가 있는지 확인한다. */
	bool HasFinalShopLayout() const;

	/** @brief 확정 WBP의 반복 슬롯을 이름으로 찾아 입력을 연결한다. */
	void CacheFinalShopWidgets();
	void BindFinalShopInputs();

	/** @brief 확정 가로형 레일/상세/스킬 대상 선택기를 현재 스냅샷으로 갱신한다. */
	void RefreshFinalShopView(const FShopUI& Shop);
	void RefreshRailView(const FShopUI& Shop);
	void RefreshSelectedItemView(const FShopUI& Shop);
	void RefreshSkillTargetView(const FShopUI& Shop);
	void RefreshRestView(const FShopUI& Shop);
	void RebuildFilteredItems(const FShopUI& Shop);
	void SetArtifactInventoryOpen(bool bOpen);

	/** @brief 확정 레이아웃의 로컬 선택 상태를 바꾼다. */
	void SetActiveItemKind(EShopItemKind Kind);
	void SelectRailSlot(int32 RailSlotIndex);
	void SelectUnitSlot(int32 UnitViewIndex);
	void SelectSkillSlot(int32 SkillSlotIndex);
	const FShopItemUI* GetSelectedItem(const FShopUI& Shop) const;

	/** @brief 소지 아티펙트(파티 소유)와 파티 유닛 카드를 소지 박스에 반영한다. */
	void RefreshOwnedView(const FShopUI& Shop);

protected:
	// ---- WBP BindWidget (이름은 Tools/UI/create_shop_wbp.py 의 위젯 이름과 정확히 일치) ----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mGoldText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> mItemBox;

	/** @brief 스킬 판매 슬롯 전용 박스. WBP에 있으면 스킬 카드는 여기로, 없으면 mItemBox 폴백 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> mSkillItemBox;

	/** @brief 아티펙트 판매 슬롯 전용 박스. WBP에 있으면 아티펙트 카드는 여기로, 없으면 mItemBox 폴백 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> mArtifactItemBox;

	/** @brief 소지 아티펙트(파티 소유) 표시 박스. 없으면 소지 아티펙트 표시 생략 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> mOwnedArtifactBox;

	/** @brief 파티 유닛 카드(직업/레벨 + 스킬 슬롯) 표시 박스. 없으면 유닛 표시 생략 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> mOwnedUnitBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mCloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mCloseButtonText;

	// ---- 확정 가로형 상점 WBP ----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mArtifactTabButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mSkillTabButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mRestTabButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mArtifactTabText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mSkillTabText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mRestTabText;

	/** Optional presentation plates cached by agreed source-widget names. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> mArtifactTabPlate;

	UPROPERTY(Transient)
	TObjectPtr<UImage> mSkillTabPlate;

	UPROPERTY(Transient)
	TObjectPtr<UImage> mRestTabPlate;

	/** @brief 스킬 상세 카드에서 현재 교체 슬롯을 가리키는 포인터. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> mSkillSelectionPointer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mPreviousButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mNextButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mBuyButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mBuyButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mRestButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mRestButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RestCostText;

	/** @brief 아티팩트 탭에서 파티 보유품을 여는 읽기 전용 인벤토리. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mInventoryButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mInventoryButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> mArtifactInventoryPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mArtifactInventoryCloseButton;

	/** @brief 공용 구매/이전/다음 버튼의 장식까지 모드 전환 시 함께 숨길 Holder. */
	UPROPERTY(Transient)
	TObjectPtr<UWidget> mPreviousHolder;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> mNextHolder;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> mBuyHolder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> mArtifactShopPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> mSkillShopPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> mRestShopPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> mSelectedItemIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mSelectedItemNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mSelectedItemDescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mSelectedItemPriceText;

	/** @brief ShopRail*/
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> mRailButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mRailPlates;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mRailIcons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mRailPriceTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> mUnitSelectButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mUnitSelectPlates;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mUnitSelectIcons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> mSkillSlotButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mSkillSlotPlates;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mSkillSlotIcons;

	/** @brief 휴식 화면의 3인 전/후 HP/AP 표시. 이름은 RestUnit<Field>_0..2 계약을 따른다. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> mRestUnitRowHolders;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mRestUnitPlates;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mRestUnitIcons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mRestUnitHPBeforeTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mRestUnitHPAfterTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mRestUnitAPBeforeTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> mRestUnitAPAfterTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mRestUnitHPBeforeFills;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mRestUnitHPAfterFills;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mRestUnitAPBeforeFills;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mRestUnitAPAfterFills;

	/** @brief 활성 탭에서 Shop.mItems로 돌아가기 위한 인덱스와 레일 배치. */
	TArray<int32> mFilteredShopItemIndices;
	TArray<int32> mRailShopItemIndices;
	EShopItemKind mActiveItemKind = EShopItemKind::Artifact;
	int32 mSelectedFilteredIndex = 0;
	int32 mSelectedUnitViewIndex = 0;
	int32 mSelectedSkillSlotIndex = 0;
	bool mIsArtifactInventoryOpen = false;

	/** @brief 현재 바인딩된 상점 상태 소유자; 위젯은 이 객체를 소유하지 않고 구독만 한다. */
	UPROPERTY(BlueprintReadOnly, Category = "Shop|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShopUIModel> mUIModel;
};
