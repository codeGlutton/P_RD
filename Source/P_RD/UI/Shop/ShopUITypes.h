// @file ShopUITypes.h
// @brief 상점(런 중 상점방) 화면 UI에 표시할 뷰 데이터입니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "ShopUITypes.generated.h"

class UTexture2D;

/** @brief 상점 판매 항목 종류. UI가 게임플레이 데이터 타입을 직접 모르게 어댑터가 변환해 넣는다. */
UENUM(BlueprintType)
enum class EShopItemKind : uint8
{
	Dice,        // 주사위 구매
	Skill,       // 스킬 구매
	Equipment,   // 장비 구매
	Heal,        // 회복 서비스
	Upgrade      // 강화 서비스
};

/** @brief 상점 판매 슬롯 한 칸을 그리기 위한 표시값입니다. */
// UI 필요값:
// - mSlotIndex: 구매/롱프레스 요청 payload.
// - mKind: 종류별 아이콘/배치.
// - mName/mIcon/mDescription: 슬롯 표시.
// - mPrice: 가격(골드).
// - mRarityColor: 희귀도 테두리(어댑터가 enum→색 변환).
// - mIsAffordable: 현재 골드로 살 수 있는지(비활성 표시).
// - mIsSoldOut: 이미 구매되어 품절인지.
// [합의필요] 최종 소스 = ShopGameMode + StaticShopRoomSpawnData. 현재 Mock.
USTRUCT(BlueprintType)
struct FShopItemUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mSlotIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) EShopItemKind mKind = EShopItemKind::Dice;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	UPROPERTY(BlueprintReadOnly) FText mDescription;
	UPROPERTY(BlueprintReadOnly) int32 mPrice = 0;
	UPROPERTY(BlueprintReadOnly) FLinearColor mRarityColor = FLinearColor::White;
	UPROPERTY(BlueprintReadOnly) bool mIsAffordable = true;
	UPROPERTY(BlueprintReadOnly) bool mIsSoldOut = false;
};

/** @brief 상점 화면 전체 표시값입니다. */
// UI 필요값:
// - mGold: 상단 보유 골드(구매 가능 여부 갱신 기준).
// - mItems: 판매 슬롯 목록.
USTRUCT(BlueprintType)
struct FShopUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mGold = 0;
	UPROPERTY(BlueprintReadOnly) TArray<FShopItemUI> mItems;
};
