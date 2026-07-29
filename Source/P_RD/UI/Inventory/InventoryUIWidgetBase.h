// @file InventoryUIWidgetBase.h
// @brief 인벤토리(런 상태 확인) 화면 WBP가 상속하는 베이스입니다. 뷰모델에 묶여 표시·입력만 담당합니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/Inventory/InventoryUITypes.h"
#include "InventoryUIWidgetBase.generated.h"

class UButton;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWidget;
class UInventoryUIModel;

/** @brief 인벤토리 화면 WBP 베이스. WBP(create_inventory_wbp.py 생성) 위젯 이름은 아래 BindWidget 멤버명과 일치해야 한다. */
// BindUIModel()로 UInventoryUIModel에 연결하면 C++가 BindWidget 위젯에 값을 채운다(메타 + 스킬/장비 아이콘 카드).
// 항목 롱프레스는 LongPressItem()으로 의도만 보낸다. 위젯은 런 상태를 계산/변경하지 않는다.
UCLASS(Abstract)
class P_RD_API UInventoryUIWidgetBase : public URDUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 인벤토리 화면이 다른 UI 위에 뜨도록 팝업 ZOrder를 설정한다. */
	UInventoryUIWidgetBase(const FObjectInitializer& ObjectInitializer);

	/** @brief 인벤토리 뷰모델에 연결하고 갱신 알림을 구독한다. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void BindUIModel(UInventoryUIModel* InUIModel);

	/** @brief WBP가 목록을 그릴 때 현재 UIModel을 읽기 위한 접근자. */
	UFUNCTION(BlueprintPure, Category = "Inventory|UI")
	UInventoryUIModel* GetUIModel() const { return mUIModel; }

	/** @brief 항목 슬롯 롱프레스가 호출. 의도만 뷰모델로 넘긴다. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void LongPressItem(EInventoryItemKind Kind, int32 ItemIndex);

	/** @brief 자동화/블루프린트가 현재 표시 중인 골드를 확인한다. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Read")
	int32 GetDisplayedGold() const;

	/** @brief 현재 화면에 표시할 용병 수. 각 용병 EXP가 독립 행인지 검증하는 데 사용한다. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Read")
	int32 GetMercenaryRowCount() const;

	/** @brief 현재 화면에 표시할 획득 보상 수(스킬+장비+아티팩트). */
	UFUNCTION(BlueprintPure, Category = "Inventory|Read")
	int32 GetAcquiredItemCount() const;

	/** @brief 기본/교체 배경 아트 중 실제 사용할 텍스처가 준비됐는지 확인한다. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Read")
	bool HasBackgroundArt() const;

	/** @brief 아이콘이 비어 있는 스킬/장비/아티팩트에 사용할 기본 아이콘이 모두 준비됐는지 확인한다. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Read")
	bool HasFallbackItemIcons() const;

protected:
	/** @brief 인벤토리값이 들어왔을 때 호출. WBP가 추가 연출을 하고 싶으면 여기서 한다(선택). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|UI")
	void OnInventoryRefreshed();

	/** @brief 닫기 버튼 연결 + 이미 바인딩된 인벤토리가 있으면 즉시 그린다. */
	virtual void NativeConstruct() override;

	/** @brief 열 때마다 실제 런 상태를 다시 읽어 골드/EXP/보상 목록을 최신화한다. */
	virtual void ApplyOpenUI() override;

	/** @brief 화면 이탈 시 UIModel 델리게이트 구독을 정리한다. */
	virtual void NativeDestruct() override;

private:
	/** @brief 닫기 버튼 클릭 → 화면을 닫는다(읽기 전용 화면이라 모델 변경 없음). */
	UFUNCTION() void HandleCloseClicked();

	/** @brief UIModel 변경 알림을 받아 BindWidget 위젯에 값을 채운다. */
	UFUNCTION() void HandleUIChanged();

	/** @brief 현재 UIModel 구독을 해제하고 참조를 비운다. */
	void UnbindUIModel();

	/** @brief 현재 모델의 메타/보유 목록을 BindWidget 위젯에 반영한다. */
	void RefreshView();

	/** @brief 현재 RoomGameMode의 View DTO를 UIModel 스냅샷으로 변환해 갱신한다. */
	void RefreshFromCurrentRoom();

	/** @brief 아직 전용 WBP 레이아웃이 없으면 실사용 가능한 런타임 레이아웃을 만든다. */
	void EnsureFallbackLayout();

	/** @brief 한 섹션(HorizontalBox)을 항목 카드(아이콘+이름)로 채운다. */
	void FillSection(UHorizontalBox* Box, const TArray<FInventoryItemUI>& Items);

	/** @brief 데이터 아이콘이 없으면 종류별 cook-safe 기본 아이콘을 반환한다. */
	UTexture2D* ResolveInventoryIcon(const FInventoryItemUI& Item) const;

	/** @brief 용병별 레벨/EXP/HP 행을 채운다. */
	void FillMercenaries(UVerticalBox* Box, const TArray<FInventoryMercenaryUI>& Mercenaries);

protected:
	// ---- WBP BindWidget (이름은 Tools/UI/create_inventory_wbp.py 의 위젯 이름과 정확히 일치) ----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mMetaText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mPartyLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> mPartyBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mSkillLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> mSkillBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mEquipLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> mEquipBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mArtifactLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> mArtifactBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mCloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mCloseButtonText;

	/** @brief 현재 바인딩된 인벤토리 상태 소유자; 위젯은 이 객체를 소유하지 않고 구독만 한다. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryUIModel> mUIModel;

	/**
	 * @brief 새 아트가 준비되면 WBP 기본값으로 넣는 선택 배경.
	 *
	 * @details 비어 있으면 코드가 어두운 단색 판을 사용하므로 이미지 임포트/경로가 늦어져도
	 *          인벤토리 기능과 모바일 확인은 막히지 않는다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Art", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UTexture2D> mBackgroundArt;

	/**
	 * @brief APK cook 포함을 보장하는 기본 인벤토리 배경 하드 참조.
	 *
	 * @details 생성자 ConstructorHelpers로 /Game/UI/Art/RunFlow/T_Inventory_Background_Current를
	 *          로드해 CDO가 강하게 소유한다. 별도 AlwaysCook 설정 없이도 위젯 클래스와 함께 cook된다.
	 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Art", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> mDefaultBackgroundArt;

	/** @brief 데이터 에셋에 아이콘이 비어 있을 때 쓰는 cook-safe 스킬 기본 아이콘. */
	UPROPERTY(VisibleDefaultsOnly, Category = "Inventory|Art")
	TObjectPtr<UTexture2D> mDefaultSkillIcon;

	/** @brief 데이터 에셋에 아이콘이 비어 있을 때 쓰는 cook-safe 장비 기본 아이콘. */
	UPROPERTY(VisibleDefaultsOnly, Category = "Inventory|Art")
	TObjectPtr<UTexture2D> mDefaultEquipmentIcon;

	/** @brief 데이터 에셋에 아이콘이 비어 있을 때 쓰는 cook-safe 아티팩트 기본 아이콘. */
	UPROPERTY(VisibleDefaultsOnly, Category = "Inventory|Art")
	TObjectPtr<UTexture2D> mDefaultArtifactIcon;

	/** @brief GameMode View를 받아 쓰는 실제 런타임 모델. Preview/테스트에서 외부 모델을 Bind하면 만들지 않는다. */
	UPROPERTY(Transient)
	TObjectPtr<UInventoryUIModel> mRuntimeUIModel;

	/** @brief 코드 fallback 레이아웃 루트. 전용 WBP 위젯 계약이 갖춰지면 만들지 않는다. */
	UPROPERTY(Transient)
	TObjectPtr<UWidget> mFallbackLayoutRoot;
};
