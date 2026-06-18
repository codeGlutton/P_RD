// @file MockShopDriver.h
// @brief 게임플레이 없이 상점 화면을 선개발/검증하기 위한 가짜 데이터 공급기입니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "MockShopDriver.generated.h"

class UShopViewModel;

/** @brief ViewModel에 mock 상점 목록을 채우고, 구매 시 골드 차감/품절을 흉내내 갱신한다. */
// 실제 어댑터(ShopGameMode/StaticShopRoomSpawnData 연결) 전, UI 흐름(구매→갱신)을 검증하기 위한 임시 드라이버.
UCLASS()
class P_RD_API UMockShopDriver : public UObject
{
	GENERATED_BODY()

public:
	/** @brief 가짜 판매 목록 + 보유 골드를 만들어 ViewModel에 push하고 구매/나가기 입력을 구독한다. */
	UFUNCTION(BlueprintCallable, Category = "Shop|Mock")
	void Start(UShopViewModel* ViewModel);

private:
	/** @brief mock 구매: 골드가 충분하면 차감하고 해당 슬롯을 품절 처리 후 다시 push한다. */
	UFUNCTION() void HandleBuy(int32 SlotIndex);

	/** @brief mock 나가기: 로그만 남긴다. */
	UFUNCTION() void HandleLeave();

	/** @brief 현재 갱신된 상점 스냅샷을 다시 만들어 push한다. */
	void PushShop();

	UPROPERTY(Transient) TObjectPtr<UShopViewModel> mViewModel;
	UPROPERTY(Transient) int32 mGold = 0;
	UPROPERTY(Transient) TArray<bool> mSoldOut;
};
