// @file InventoryUIWidgetBase.h
// @brief 인벤토리(런 상태 확인) 화면 WBP가 상속하는 베이스입니다. 뷰모델에 묶여 표시·입력만 담당합니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/Inventory/InventoryUITypes.h"
#include "InventoryUIWidgetBase.generated.h"

class UInventoryUIModel;

/** @brief 인벤토리 화면 WBP 베이스. 이 클래스를 상속한 WBP가 실제 비주얼을 그린다. */
// 이 베이스를 상속한 WBP는:
// - BindUIModel()로 UInventoryUIModel에 연결하고,
// - OnInventoryRefreshed(BlueprintImplementableEvent)에서 GetUIModel()->GetInventory()를 읽어 목록을 그리고,
// - 항목 롱프레스는 RequestItemLongPress()로 의도만 보낸다.
// 위젯은 런 상태를 계산/변경하지 않는다. 진실은 게임플레이에 있다.
UCLASS(Abstract)
class P_RD_API UInventoryUIWidgetBase : public URDUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 인벤토리 뷰모델에 연결하고 갱신 알림을 구독한다. */
	// 같은 WBP 인스턴스가 재사용될 수 있으므로 기존 구독을 먼저 끊는다.
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void BindUIModel(UInventoryUIModel* InUIModel);

	/** @brief WBP가 목록을 그릴 때 현재 UIModel을 읽기 위한 접근자. */
	UFUNCTION(BlueprintPure, Category = "Inventory|UI")
	UInventoryUIModel* GetUIModel() const { return mUIModel; }

	/** @brief 항목 슬롯 롱프레스가 호출. 의도만 뷰모델로 넘긴다. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void LongPressItem(EInventoryItemKind Kind, int32 ItemIndex);

protected:
	/** @brief 인벤토리값이 들어왔을 때 호출. WBP가 다이스/스킬/장비 목록을 다시 그린다. */
	// C++은 표시 데이터 전달까지만 담당하고, 실제 레이아웃/연출은 WBP가 결정한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|UI")
	void OnInventoryRefreshed();

	/** @brief 화면 이탈 시 UIModel 델리게이트 구독을 정리한다. */
	virtual void NativeDestruct() override;

private:
	/** @brief UIModel 변경 알림을 WBP 갱신 이벤트로 변환한다. */
	UFUNCTION() void HandleUIChanged();

	/** @brief 현재 UIModel 구독을 해제하고 참조를 비운다. */
	void UnbindUIModel();

protected:
	/** @brief 현재 바인딩된 인벤토리 상태 소유자; 위젯은 이 객체를 소유하지 않고 구독만 한다. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryUIModel> mUIModel;
};
