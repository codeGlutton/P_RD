#include "UI/Common/CommonPopupWidget.h"

/** @details 제목/본문을 보관하고 WBP에 알림 레이아웃을 띄우라는 신호를 보낸다. */
void UCommonPopupWidget::ShowAlert(const FText& Title, const FText& Message)
{
	mTitle = Title;
	mMessage = Message;
	OnShowAlert();
}

/** @details 제목/본문을 보관하고 WBP에 예/아니오 확인 레이아웃을 띄우라는 신호를 보낸다. */
void UCommonPopupWidget::ShowConfirm(const FText& Title, const FText& Message)
{
	mTitle = Title;
	mMessage = Message;
	OnShowConfirm();
}

/** @details 로딩 메시지를 보관하고 WBP에 로딩 오버레이를 띄우라는 신호를 보낸다. */
void UCommonPopupWidget::ShowLoading(const FText& Message)
{
	mMessage = Message;
	OnShowLoading();
}

/** @details WBP에 로딩 오버레이를 숨기라는 신호를 보낸다. */
void UCommonPopupWidget::HideLoading()
{
	OnHideLoading();
}

/** @details 확인창 '예' 결과(true)를 호출자에게 알린다. 실제 닫힘 연출은 WBP가 처리한다. */
void UCommonPopupWidget::ConfirmYes()
{
	OnConfirmResult.Broadcast(true);
}

/** @details 확인창 '아니오' 결과(false)를 호출자에게 알린다. */
void UCommonPopupWidget::ConfirmNo()
{
	OnConfirmResult.Broadcast(false);
}
