#include "UI/Shop/MockShopDriver.h"

#include "UI/Shop/ShopUIModel.h"

namespace
{
	struct FMockShopEntry
	{
		EShopItemKind Kind;
		const TCHAR* Name;
		int32 Price;
	};

	// mock 판매 목록 fixture. 밸런스 데이터 아님 — 구매→골드차감→품절 흐름 검증용.
	const FMockShopEntry GMockEntries[] = {
		{ EShopItemKind::Dice,      TEXT("d8 Dice"),     40 },
		{ EShopItemKind::Skill,     TEXT("Cleave"),      60 },
		{ EShopItemKind::Equipment, TEXT("Leather"),     50 },
		{ EShopItemKind::Heal,      TEXT("Heal +10"),    30 },
		{ EShopItemKind::Upgrade,   TEXT("Dice Upgrade"), 80 },
	};
	constexpr int32 GMockEntryCount = sizeof(GMockEntries) / sizeof(GMockEntries[0]);
}

/**
 * @details 가짜 판매 목록 + 보유 골드를 만들어 push하고, 구매/나가기 입력을 구독한다.
 * 실제 ShopGameMode 가 붙기 전 "슬롯 표시 → 구매 → 골드 차감 → 품절 갱신" UI 흐름을 검증한다.
 */
void UMockShopDriver::Start(UShopUIModel* UIModel)
{
	if (UIModel == nullptr)
	{
		return;
	}
	mUIModel = UIModel;
	mGold = 120;
	mSoldOut.Init(false, GMockEntryCount);

	mUIModel->OnBuyRequested.AddDynamic(this, &UMockShopDriver::HandleBuy);
	mUIModel->OnLeaveRequested.AddDynamic(this, &UMockShopDriver::HandleLeave);

	PushShop();
}

/** @details 현재 골드/품절 상태로 상점 스냅샷을 만들어 push한다. 가격>골드면 mIsAffordable=false. */
void UMockShopDriver::PushShop()
{
	if (mUIModel == nullptr)
	{
		return;
	}

	FShopUI Shop;
	Shop.mGold = mGold;
	for (int32 i = 0; i < GMockEntryCount; ++i)
	{
		FShopItemUI Item;
		Item.mSlotIndex = i;
		Item.mKind = GMockEntries[i].Kind;
		Item.mName = FText::FromString(GMockEntries[i].Name);
		Item.mPrice = GMockEntries[i].Price;
		Item.mIsSoldOut = mSoldOut.IsValidIndex(i) && mSoldOut[i];
		Item.mIsAffordable = !Item.mIsSoldOut && mGold >= Item.mPrice;
		Shop.mItems.Add(Item);
	}
	mUIModel->SetShop(Shop);
}

/** @details mock 구매: 살 수 있으면 골드 차감 + 품절 처리 후 다시 push해 화면을 갱신한다. */
void UMockShopDriver::HandleBuy(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= GMockEntryCount)
	{
		return;
	}
	if (mSoldOut.IsValidIndex(SlotIndex) && mSoldOut[SlotIndex])
	{
		return;
	}
	const int32 Price = GMockEntries[SlotIndex].Price;
	if (mGold < Price)
	{
		UE_LOG(LogRD, Display, TEXT("MockShopDriver: not enough gold for slot %d"), SlotIndex);
		return;
	}
	mGold -= Price;
	mSoldOut[SlotIndex] = true;
	UE_LOG(LogRD, Display, TEXT("MockShopDriver: bought slot %d, gold=%d"), SlotIndex, mGold);
	PushShop();
}

/** @details mock 나가기: 로그만 남긴다(실제 전환은 게임플레이). */
void UMockShopDriver::HandleLeave()
{
	UE_LOG(LogRD, Display, TEXT("MockShopDriver: leave shop"));
}
