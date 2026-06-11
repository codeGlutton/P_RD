#include "UI/RDUserWidget.h"

/**
 * @brief 위젯 생성 직후에는 공통 OpenUI() 요청 전까지 보이지 않게 둔다.
 */
URDUserWidget::URDUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Collapsed);
}

/**
 * @brief 뷰포트 등록과 열기 연출을 한 경로로 묶는다.
 *
 * @details
 * 호출자는 HUD인지 팝업인지, 이미 뷰포트에 있는지, 애니메이션이 있는지 몰라도 OpenUI()만 호출하면 된다.
 * 이미 열린 상태에서 다시 호출된 경우에도 후속 로직은 이어져야 하므로 콜백은 즉시 실행한다.
 */
void URDUserWidget::OpenUI(FOnEndUIOpenAnimation Callback)
{
	if (IsOpened())
	{
		if (Callback.IsBound())
		{
			Callback.Execute(this);
		}
		return;
	}

	OnEndUIOpenAnimation = MoveTemp(Callback);
	mLifecycleState = ERDUserWidgetLifecycleState::Opening;
	ApplyOpenUI();
	PlayOpenUIAnimation();
}

/**
 * @brief 닫기 연출과 실제 닫힘 처리를 한 경로로 묶는다.
 *
 * @details
 * 닫힌 위젯을 닫는 요청은 성공한 닫힘으로 취급해 콜백을 실행한다.
 * 반대로 닫히는 중인 위젯에 대한 중복 요청은 같은 애니메이션과 콜백이 다시 쌓이지 않도록 무시한다.
 */
void URDUserWidget::CloseUI(FOnEndUICloseAnimation Callback)
{
	if (mLifecycleState == ERDUserWidgetLifecycleState::Closed)
	{
		if (Callback.IsBound())
		{
			Callback.Execute(this);
		}
		return;
	}

	if (mLifecycleState == ERDUserWidgetLifecycleState::Closing)
	{
		return;
	}

	OnEndUICloseAnimation = MoveTemp(Callback);
	mLifecycleState = ERDUserWidgetLifecycleState::Closing;
	PlayCloseUIAnimation();
}

/**
 * @brief 내부 생명주기와 실제 UMG 표시 상태가 모두 열림 조건을 만족하는지 확인한다.
 */
bool URDUserWidget::IsOpened() const
{
	return (mLifecycleState == ERDUserWidgetLifecycleState::Opening
			|| mLifecycleState == ERDUserWidgetLifecycleState::Open)
		&& IsInViewport()
		&& IsVisible();
}

/**
 * @brief Blueprint 또는 기본 구현에서 열기 연출 완료 시점을 확정한다.
 */
void URDUserWidget::FinishOpenUI()
{
	if (mLifecycleState != ERDUserWidgetLifecycleState::Opening)
	{
		return;
	}

	mLifecycleState = ERDUserWidgetLifecycleState::Open;
	if (OnEndUIOpenAnimation.IsBound())
	{
		OnEndUIOpenAnimation.Execute(this);
		OnEndUIOpenAnimation.Unbind();
	}
}

/**
 * @brief Blueprint 또는 기본 구현에서 닫기 연출 완료 시점을 확정한다.
 */
void URDUserWidget::FinishCloseUI()
{
	if (mLifecycleState != ERDUserWidgetLifecycleState::Closing)
	{
		return;
	}

	ApplyCloseUI();
	mLifecycleState = ERDUserWidgetLifecycleState::Closed;
	if (OnEndUICloseAnimation.IsBound())
	{
		OnEndUICloseAnimation.Execute(this);
		OnEndUICloseAnimation.Unbind();
	}
}

/**
 * @brief 별도 Blueprint 연출이 없으면 즉시 열린 상태로 완료한다.
 */
void URDUserWidget::PlayOpenUIAnimation_Implementation()
{
	FinishOpenUI();
}

/**
 * @brief 별도 Blueprint 연출이 없으면 즉시 닫힌 상태로 완료한다.
 */
void URDUserWidget::PlayCloseUIAnimation_Implementation()
{
	FinishCloseUI();
}

/**
 * @brief 공통 OpenUI()가 선택한 ZOrder로 뷰포트에 올리고 Visible 상태를 보장한다.
 */
void URDUserWidget::ApplyOpenUI()
{
	if (IsInViewport() == false)
	{
		AddToViewport(GetViewportZOrder());
	}

	SetVisibility(ESlateVisibility::Visible);
}

/**
 * @brief 위젯별 닫힘 정책에 따라 제거하거나 숨김 상태로 보존한다.
 */
void URDUserWidget::ApplyCloseUI()
{
	if (ShouldRemoveFromParentOnClose())
	{
		RemoveFromParent();
		return;
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

int32 URDUserWidget::GetViewportZOrder() const
{
	return mViewportZOrder;
}

bool URDUserWidget::ShouldRemoveFromParentOnClose() const
{
	return mRemoveFromParentOnClose;
}
