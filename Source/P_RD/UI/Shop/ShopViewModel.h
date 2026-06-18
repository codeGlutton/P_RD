// @file ShopViewModel.h
// @brief 상점(런 중 상점방) 화면 UI와 게임플레이를 잇는 경계(뷰모델)입니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "UI/Shop/ShopViewTypes.h"
#include "ShopViewModel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopViewChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopBuyRequested, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopLeaveRequested);

/** @brief 상점 화면 뷰모델. 상점방 진입 시 게임플레이가 하나 만들어 위젯에 물린다. */
// RewardViewModel과 같은 계약: 게임플레이가 SetShop()으로 밀어넣고, 위젯은 GetShop()으로 읽는다.
// 구매/나가기는 의도만 보낸다 — 골드 차감/지급/품절 처리는 게임플레이(ShopGameMode)가 하고 SetShop으로 갱신.
UCLASS(BlueprintType)
class P_RD_API UShopViewModel : public UObject
{
	GENERATED_BODY()

	/* ───────── 위젯이 구독하는 알림 ───────── */
public:
	/** @brief 상점 표시값이 설정/갱신됐음을 알림(구매 후 골드/품절 반영). 위젯은 목록을 다시 그린다. */
	UPROPERTY(BlueprintAssignable, Category = "Shop|View")
	FOnShopViewChanged OnViewChanged;

	/* ───────── 게임플레이가 구독하는 입력(의도) ───────── */
public:
	/** @brief 위젯이 구매를 확정했음(SlotIndex). 게임플레이가 골드 차감/지급한다. */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Input")
	FOnShopBuyRequested OnBuyRequested;

	/** @brief 위젯이 상점을 나가려 함(다음 화면으로). */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Input")
	FOnShopLeaveRequested OnLeaveRequested;

	/* ───────── UI → gameplay : 의도만 보낸다 ───────── */
public:
	/** @brief SlotIndex 구매 의도를 게임플레이 구독자에게 전달한다(구매 확인은 화면/팝업이 먼저 처리). */
	UFUNCTION(BlueprintCallable, Category = "Shop|Input") void RequestBuy(int32 SlotIndex);

	/** @brief 상점 나가기 의도를 전달한다. */
	UFUNCTION(BlueprintCallable, Category = "Shop|Input") void RequestLeave();

	/* ───────── gameplay → UI : 표시값을 밀어넣는다 ───────── */
public:
	/** @brief 게임플레이/어댑터가 확정한 상점 표시 스냅샷을 저장하고 변경 알림을 보낸다. */
	UFUNCTION(BlueprintCallable, Category = "Shop|Push") void SetShop(const FShopView& Shop);

	/* ───────── 위젯이 읽는다 ───────── */
public:
	/** @brief 위젯이 현재 상점 스냅샷을 읽는다; 반환 참조는 다음 SetShop()까지 유효하다. */
	UFUNCTION(BlueprintPure, Category = "Shop|Read") const FShopView& GetShop() const { return mShop; }

private:
	/** @brief 마지막으로 Push된 상점 표시값; Transient라 세이브/에셋 상태로 남기지 않는다. */
	UPROPERTY(Transient) FShopView mShop;
};
