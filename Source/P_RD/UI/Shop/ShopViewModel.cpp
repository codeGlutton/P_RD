#include "UI/Shop/ShopViewModel.h"

/** @details 구매 의도만 구독자(게임플레이)에게 중계한다. 골드 차감/지급/품절은 게임플레이가 처리한다. */
void UShopViewModel::RequestBuy(int32 SlotIndex)
{
	OnBuyRequested.Broadcast(SlotIndex);
}

/** @details 상점 나가기 의도만 전달한다. 실제 화면 전환은 게임플레이가 결정한다. */
void UShopViewModel::RequestLeave()
{
	OnLeaveRequested.Broadcast();
}

/** @details 상점 스냅샷을 저장하고 변경 알림을 쏜다. 구매 후 게임플레이가 다시 호출해 골드/품절을 갱신한다. */
void UShopViewModel::SetShop(const FShopView& Shop)
{
	mShop = Shop;
	OnViewChanged.Broadcast();
}
