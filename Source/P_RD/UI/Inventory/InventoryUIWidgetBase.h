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
class UTextBlock;
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

protected:
	/** @brief 인벤토리값이 들어왔을 때 호출. WBP가 추가 연출을 하고 싶으면 여기서 한다(선택). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|UI")
	void OnInventoryRefreshed();

	/** @brief 닫기 버튼 연결 + 이미 바인딩된 인벤토리가 있으면 즉시 그린다. */
	virtual void NativeConstruct() override;

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

	/** @brief 한 섹션(HorizontalBox)을 항목 카드(아이콘+이름)로 채운다. */
	void FillSection(UHorizontalBox* Box, const TArray<FInventoryItemUI>& Items);

protected:
	// ---- WBP BindWidget (이름은 Tools/UI/create_inventory_wbp.py 의 위젯 이름과 정확히 일치) ----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mMetaText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mSkillLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> mSkillBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mEquipLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> mEquipBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mCloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mCloseButtonText;

	/** @brief 현재 바인딩된 인벤토리 상태 소유자; 위젯은 이 객체를 소유하지 않고 구독만 한다. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryUIModel> mUIModel;
};
