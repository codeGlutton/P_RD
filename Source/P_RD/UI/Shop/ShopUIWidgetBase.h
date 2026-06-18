// @file ShopUIWidgetBase.h
// @brief 상점(런 중 상점방) 화면 WBP가 상속하는 베이스입니다. 뷰모델에 묶여 표시·입력만 담당합니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "ShopUIWidgetBase.generated.h"

class UShopUIModel;

/** @brief 상점 화면 WBP 베이스. 이 클래스를 상속한 WBP가 실제 비주얼을 그린다. */
// 이 베이스를 상속한 WBP는:
// - BindUIModel()로 UShopUIModel에 연결하고,
// - OnShopRefreshed(BlueprintImplementableEvent)에서 GetUIModel()->GetShop()을 읽어 슬롯을 그리고,
// - 슬롯 구매는 BuyItem()(구매 확인 팝업 뒤), 나가기는 Leave()로 의도만 보낸다.
// 위젯은 골드 차감/지급을 하지 않는다. 진실은 게임플레이(ShopGameMode)에 있다.
UCLASS(Abstract)
class P_RD_API UShopUIWidgetBase : public URDUserWidget
{
	GENERATED_BODY()

public:
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
	/** @brief 상점값이 들어왔을 때 호출. WBP가 슬롯 목록/골드를 다시 그린다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Shop|UI")
	void OnShopRefreshed();

	/** @brief 화면 이탈 시 UIModel 델리게이트 구독을 정리한다. */
	virtual void NativeDestruct() override;

private:
	/** @brief UIModel 변경 알림을 WBP 갱신 이벤트로 변환한다. */
	UFUNCTION() void HandleUIChanged();

	/** @brief 현재 UIModel 구독을 해제하고 참조를 비운다. */
	void UnbindUIModel();

protected:
	/** @brief 현재 바인딩된 상점 상태 소유자; 위젯은 이 객체를 소유하지 않고 구독만 한다. */
	UPROPERTY(BlueprintReadOnly, Category = "Shop|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShopUIModel> mUIModel;
};
