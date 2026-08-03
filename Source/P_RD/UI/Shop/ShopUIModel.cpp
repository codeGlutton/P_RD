#include "UI/Shop/ShopUIModel.h"

/** @details 구매 의도만 구독자(게임플레이)에게 중계한다. 골드 차감/지급/품절은 게임플레이가 처리한다. */
void UShopUIModel::RequestBuy(int32 SlotIndex)
{
	OnBuyRequested.Broadcast(SlotIndex);
}

/** @details 스킬 구매 의도만 중계한다. 직업 검증과 지급은 게임플레이(ShopGameMode)가 처리한다. */
void UShopUIModel::RequestBuySkill(int32 SlotIndex, int32 UnitIndex)
{
	OnBuySkillRequested.Broadcast(SlotIndex, UnitIndex);
}

/** @details 아티펙트 버리기 의도만 중계한다. 실제 제거는 게임플레이(ShopGameMode)가 처리한다. */
void UShopUIModel::RequestDiscardArtifact(int32 ArtifactIndex)
{
	OnDiscardArtifactRequested.Broadcast(ArtifactIndex);
}

/** @details 스킬 슬롯 버리기 의도만 중계한다. 실제 슬롯 비우기는 게임플레이(ShopGameMode)가 처리한다. */
void UShopUIModel::RequestDiscardSkill(int32 UnitIndex, int32 SlotIndex)
{
	OnDiscardSkillRequested.Broadcast(UnitIndex, SlotIndex);
}

/** @details 상점 나가기 의도만 전달한다. 실제 화면 전환은 게임플레이가 결정한다. */
void UShopUIModel::RequestLeave()
{
	OnLeaveRequested.Broadcast();
}

/** @details 상점 스냅샷을 저장하고 거래 도메인 변경 알림을 쏜다. 거래 후 게임플레이가 다시 호출해 골드/품절/소지품을 갱신한다. */
void UShopUIModel::SetShop(const FShopUI& Shop)
{
	mShop = Shop;
	OnUIChanged.Broadcast(EShopUIDomain::Trade);
}
