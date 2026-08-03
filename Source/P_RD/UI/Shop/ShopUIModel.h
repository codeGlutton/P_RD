// @file ShopUIModel.h
// @brief 상점(런 중 상점방) 화면 UI와 게임플레이를 잇는 경계(뷰모델)입니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "UI/Shop/ShopUITypes.h"
#include "ShopUIModel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopUIChanged, EShopUIDomain, Domain);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopBuyRequested, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShopBuySkillRequested, int32, SlotIndex, int32, UnitIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopDiscardArtifactRequested, int32, ArtifactIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShopDiscardSkillRequested, int32, UnitIndex, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopLeaveRequested);

/** @brief 상점 화면 뷰모델. 상점방 진입 시 게임플레이가 하나 만들어 위젯에 물린다. */
// RewardUIModel과 같은 계약: 게임플레이가 SetShop()으로 밀어넣고, 위젯은 GetShop()으로 읽는다.
// 구매/나가기는 의도만 보낸다 — 골드 차감/지급/품절 처리는 게임플레이(ShopGameMode)가 하고 SetShop으로 갱신.
UCLASS(BlueprintType)
class P_RD_API UShopUIModel : public UObject
{
	GENERATED_BODY()

	/* ───────── 위젯이 구독하는 알림 ───────── */
public:
	/**
	 * @brief 상점 표시값이 설정/갱신됐음을 알림 (어느 도메인이 갱신됐는지 함께 전달)
	 * @details 위젯은 자기 도메인 알림만 받아 해당 부분을 다시 그림
	 */
	UPROPERTY(BlueprintAssignable, Category = "Shop|UI")
	FOnShopUIChanged OnUIChanged;

	/* ───────── 게임플레이가 구독하는 입력(의도) ───────── */
public:
	/** @brief 위젯이 구매를 확정했음(SlotIndex). 게임플레이가 골드 차감/지급한다. */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Input")
	FOnShopBuyRequested OnBuyRequested;

	/**
	 * @brief 위젯이 스킬 구매를 확정했음 (슬롯 index + 지급 대상 유닛 index)
	 * @details 스킬은 유닛별 소유라 버리기와 마찬가지로 대상 유닛 지정이 필요
	 */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Input")
	FOnShopBuySkillRequested OnBuySkillRequested;

	/**
	 * @brief 위젯이 소지 아티펙트 하나를 버리려 함 (파티 공용 배열 index)
	 * @details 실제 제거는 게임플레이가 처리 — 버리기는 0골드 거래로 구매와 같은 파이프를 탐
	 */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Input")
	FOnShopDiscardArtifactRequested OnDiscardArtifactRequested;

	/**
	 * @brief 위젯이 유닛의 스킬 슬롯 하나를 버리려 함 (유닛 index + 슬롯 index)
	 * @details 스킬은 유닛별 소유라 대상 지정에 두 index가 필요
	 */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Input")
	FOnShopDiscardSkillRequested OnDiscardSkillRequested;

	/** @brief 위젯이 상점을 나가려 함(다음 화면으로). */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Input")
	FOnShopLeaveRequested OnLeaveRequested;

	/* ───────── UI → gameplay : 의도만 보낸다 ───────── */
public:
	/** @brief SlotIndex 구매 의도를 게임플레이 구독자에게 전달한다(구매 확인은 화면/팝업이 먼저 처리). */
	UFUNCTION(BlueprintCallable, Category = "Shop|Input") void RequestBuy(int32 SlotIndex);

	/** @brief 스킬 구매 의도를 전달 (지급 대상 유닛 선택은 화면이 먼저 처리) */
	UFUNCTION(BlueprintCallable, Category = "Shop|Input") void RequestBuySkill(int32 SlotIndex, int32 UnitIndex);

	/** @brief 소지 아티펙트 버리기 의도를 전달 (버리기 확인은 화면/팝업이 먼저 처리) */
	UFUNCTION(BlueprintCallable, Category = "Shop|Input") void RequestDiscardArtifact(int32 ArtifactIndex);

	/** @brief 유닛 스킬 슬롯 버리기 의도를 전달 (버리기 확인은 화면/팝업이 먼저 처리) */
	UFUNCTION(BlueprintCallable, Category = "Shop|Input") void RequestDiscardSkill(int32 UnitIndex, int32 SlotIndex);

	/** @brief 상점 나가기 의도를 전달한다. */
	UFUNCTION(BlueprintCallable, Category = "Shop|Input") void RequestLeave();

	/* ───────── gameplay → UI : 표시값을 밀어넣는다 ───────── */
public:
	/** @brief 게임플레이/어댑터가 확정한 상점 표시 스냅샷을 저장하고 변경 알림을 보낸다. */
	UFUNCTION(BlueprintCallable, Category = "Shop|Push") void SetShop(const FShopUI& Shop);

	/* ───────── 위젯이 읽는다 ───────── */
public:
	/** @brief 위젯이 현재 상점 스냅샷을 읽는다; 반환 참조는 다음 SetShop()까지 유효하다. */
	UFUNCTION(BlueprintPure, Category = "Shop|Read") const FShopUI& GetShop() const { return mShop; }

private:
	/** @brief 마지막으로 Push된 상점 표시값; Transient라 세이브/에셋 상태로 남기지 않는다. */
	UPROPERTY(Transient) FShopUI mShop;
};
