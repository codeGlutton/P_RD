// @file CommonPopupWidget.h
// @brief 화면 공통 팝업(알림/확인/로딩)을 한 위젯으로 통일한 베이스입니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "CommonPopupWidget.generated.h"

/** @brief 확인창 결과(예/아니오)를 호출자에게 알리는 델리게이트. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPopupConfirmResult, bool, bAccepted);

/** @brief 알림/확인/로딩 팝업을 공통으로 띄우는 위젯 베이스. WBP가 상속해 실제 비주얼을 그린다. */
// 흩어져 있던 LoadingNotify/Confirm 을 한 계약으로 모은다. 다른 화면(상점 구매확인 등)이 재사용한다.
// 사용: 호출자가 ShowAlert/ShowConfirm/ShowLoading 을 부르면 C++가 텍스트를 보관하고
//       OnShow*(BlueprintImplementableEvent)로 WBP에 연출 신호를 준다. 확인창은 WBP의 예/아니오 버튼이
//       ConfirmYes()/ConfirmNo()를 부르면 OnConfirmResult 로 결과를 호출자에게 돌려준다.
UCLASS(Abstract)
class P_RD_API UCommonPopupWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 확인 1버튼 알림을 띄운다. */
	UFUNCTION(BlueprintCallable, Category = "Popup")
	void ShowAlert(const FText& Title, const FText& Message);

	/** @brief 예/아니오 확인창을 띄운다. 결과는 OnConfirmResult 로 돌아온다. */
	UFUNCTION(BlueprintCallable, Category = "Popup")
	void ShowConfirm(const FText& Title, const FText& Message);

	/** @brief 로딩 오버레이를 띄운다(방 이동 프리로드 등). HideLoading 으로 닫는다. */
	UFUNCTION(BlueprintCallable, Category = "Popup")
	void ShowLoading(const FText& Message);

	/** @brief 로딩 오버레이를 닫는다. */
	UFUNCTION(BlueprintCallable, Category = "Popup")
	void HideLoading();

	/** @brief WBP의 확인창 '예' 버튼이 호출. 결과(true)를 호출자에게 알린다. */
	UFUNCTION(BlueprintCallable, Category = "Popup")
	void ConfirmYes();

	/** @brief WBP의 확인창 '아니오'(또는 닫기) 버튼이 호출. 결과(false)를 호출자에게 알린다. */
	UFUNCTION(BlueprintCallable, Category = "Popup")
	void ConfirmNo();

	/** @brief WBP가 표시할 현재 제목/본문을 읽는다. */
	UFUNCTION(BlueprintPure, Category = "Popup") const FText& GetTitle() const { return mTitle; }
	UFUNCTION(BlueprintPure, Category = "Popup") const FText& GetMessage() const { return mMessage; }

	/** @brief 확인창 결과(예=true/아니오=false). 호출자(상점 등)가 구독한다. */
	UPROPERTY(BlueprintAssignable, Category = "Popup")
	FOnPopupConfirmResult OnConfirmResult;

protected:
	/** @brief 알림을 띄우라는 신호. WBP가 알림 레이아웃을 보이고 본문을 그린다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Popup") void OnShowAlert();
	/** @brief 확인창을 띄우라는 신호. WBP가 예/아니오 레이아웃을 보인다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Popup") void OnShowConfirm();
	/** @brief 로딩 오버레이를 보이라는 신호. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Popup") void OnShowLoading();
	/** @brief 로딩 오버레이를 숨기라는 신호. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Popup") void OnHideLoading();

private:
	/** @brief 현재 팝업 제목/본문 스냅샷. WBP가 GetTitle/GetMessage 로 읽는다. */
	UPROPERTY(Transient) FText mTitle;
	UPROPERTY(Transient) FText mMessage;
};
