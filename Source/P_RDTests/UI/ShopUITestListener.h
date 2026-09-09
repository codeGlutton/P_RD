#pragma once

#include "CoreMinimal.h"
#include "UI/Shop/ShopUITypes.h"

#include "ShopUITestListener.generated.h"

/** @brief 상점 WBP가 UIModel에 넘긴 구매 payload를 검증하는 수신기. */
UCLASS()
class UShopUITestListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleBuySkillRequested(int32 SlotIndex, int32 UnitIndex,
		int32 SkillSlotIndex);

	UFUNCTION()
	void HandleRestRequested();

	UFUNCTION()
	void HandleUIChanged(EShopUIDomain Domain);

	UFUNCTION() void HandleRewardUnitRequested(int32 UnitIndex) { LastRewardUnitIndex = UnitIndex; }
	int32 LastRewardUnitIndex = INDEX_NONE;
	int32 CallCount = 0;
	int32 LastSlotIndex = INDEX_NONE;
	int32 LastUnitIndex = INDEX_NONE;
	int32 LastSkillSlotIndex = INDEX_NONE;
	int32 RestCallCount = 0;
	int32 TradeChangeCount = 0;
};
