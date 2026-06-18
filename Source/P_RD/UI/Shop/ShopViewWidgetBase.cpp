#include "UI/Shop/ShopViewWidgetBase.h"
#include "UI/Shop/ShopViewModel.h"

/** @brief 새 ViewModel을 구독하고 이미 들어온 상점 스냅샷도 즉시 한 번 그린다. */
void UShopViewWidgetBase::BindViewModel(UShopViewModel* InViewModel)
{
	if (mViewModel == InViewModel)
	{
		return;
	}
	UnbindViewModel();
	mViewModel = InViewModel;
	if (mViewModel != nullptr)
	{
		mViewModel->OnViewChanged.AddDynamic(this, &UShopViewWidgetBase::HandleViewChanged);
		// 상점 데이터가 BindViewModel보다 먼저 들어온 경우도 있으므로 연결 직후 한 번 그린다.
		OnShopRefreshed();
	}
}

/** @brief WBP의 슬롯 구매 입력을 ViewModel의 구매 의도 이벤트로 전달한다. */
void UShopViewWidgetBase::BuyItem(int32 SlotIndex)
{
	if (mViewModel != nullptr)
	{
		mViewModel->RequestBuy(SlotIndex);
	}
}

/** @brief WBP의 나가기 입력을 ViewModel의 나가기 의도 이벤트로 전달한다. */
void UShopViewWidgetBase::Leave()
{
	if (mViewModel != nullptr)
	{
		mViewModel->RequestLeave();
	}
}

/** @brief 현재 ViewModel 구독을 해제해 화면 파괴 후 OnViewChanged가 들어오지 않게 한다. */
void UShopViewWidgetBase::UnbindViewModel()
{
	if (mViewModel != nullptr)
	{
		mViewModel->OnViewChanged.RemoveDynamic(this, &UShopViewWidgetBase::HandleViewChanged);
	}
	mViewModel = nullptr;
}

/** @brief ViewModel 변경 알림을 WBP 구현 이벤트로 변환한다. */
void UShopViewWidgetBase::HandleViewChanged()
{
	OnShopRefreshed();
}

/** @brief 위젯 생명주기 종료 시 ViewModel 델리게이트를 먼저 끊고 부모 정리를 따른다. */
void UShopViewWidgetBase::NativeDestruct()
{
	UnbindViewModel();
	Super::NativeDestruct();
}
