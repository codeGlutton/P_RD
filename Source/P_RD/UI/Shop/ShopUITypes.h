// @file ShopUITypes.h
// @brief 상점(런 중 상점방) 화면 UI에 표시할 뷰 데이터입니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "ShopUITypes.generated.h"

class UTexture2D;

/**
 * @brief 상점 화면의 갱신 도메인
 * @details
 * 거래(판매/골드/소지품)와 프리뷰(시연)는 갱신 시점이 달라 도메인을 나눔.
 * 위젯은 자기 도메인 알림만 받아 다시 그림 (전투 UI의 ECombatUIDomain과 같은 계약)
 */
UENUM(BlueprintType)
enum class EShopUIDomain : uint8
{
	// 판매 목록 + 골드 + 소지품 (거래마다 함께 갱신)
	Trade,
	// 시연(용병 소환/이동/스킬 시전) 상태
	Preview,
	// 고용 게시판
	Mercenary
};

/**
 * @brief 상점 판매 항목 종류
 * @details
 * UI가 게임플레이 데이터 타입을 직접 모르게 어댑터가 변환해 넣음.
 * 장비는 아티펙트로 대체됨
 */
UENUM(BlueprintType)
enum class EShopItemKind : uint8
{
	Skill,       // 스킬 구매
	Artifact,    // 아티펙트 구매
	Mercenary,   // 용병 고용
	Heal         // 회복 서비스
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
	UPROPERTY(BlueprintReadOnly) EShopItemKind mKind = EShopItemKind::Skill;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	UPROPERTY(BlueprintReadOnly) FText mDescription;
	UPROPERTY(BlueprintReadOnly) int32 mPrice = 0;
	UPROPERTY(BlueprintReadOnly) FLinearColor mRarityColor = FLinearColor::White;
	UPROPERTY(BlueprintReadOnly) bool mIsAffordable = true;
	UPROPERTY(BlueprintReadOnly) bool mIsSoldOut = false;
};

/**
 * @brief 소지 중인 아티펙트 한 칸의 표시값
 * @details 아티펙트는 파티 공용 소유 — 버리기 요청 payload는 mArtifactIndex 하나
 */
USTRUCT(BlueprintType)
struct FShopOwnedArtifactUI
{
	GENERATED_BODY()

	// 파티 공용 아티펙트 배열 index (버리기 요청 payload)
	UPROPERTY(BlueprintReadOnly) int32 mArtifactIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	UPROPERTY(BlueprintReadOnly) FLinearColor mRarityColor = FLinearColor::White;
};

/**
 * @brief 소지 중인 스킬 한 칸의 표시값
 * @details 스킬은 유닛별 소유 — 버리기 요청 payload는 (mUnitIndex, mSlotIndex) 쌍
 */
USTRUCT(BlueprintType)
struct FShopOwnedSkillUI
{
	GENERATED_BODY()

	// 파티 유닛 슬롯 index (버리기 요청 payload 1)
	UPROPERTY(BlueprintReadOnly) int32 mUnitIndex = INDEX_NONE;
	// 유닛 내 스킬 슬롯 index (버리기 요청 payload 2)
	UPROPERTY(BlueprintReadOnly) int32 mSlotIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
};

/** @brief 상점 화면 전체 표시값입니다. */
// UI 필요값:
// - mGold: 상단 보유 골드(구매 가능 여부 갱신 기준).
// - mItems: 판매 슬롯 목록.
// - mOwnedArtifacts/mOwnedSkills: 소지품(버리기 대상). 거래마다 판매 목록과 함께 갱신되므로 같은 스냅샷에 담음.
USTRUCT(BlueprintType)
struct FShopUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mGold = 0;
	UPROPERTY(BlueprintReadOnly) TArray<FShopItemUI> mItems;
	UPROPERTY(BlueprintReadOnly) TArray<FShopOwnedArtifactUI> mOwnedArtifacts;
	UPROPERTY(BlueprintReadOnly) TArray<FShopOwnedSkillUI> mOwnedSkills;
};
