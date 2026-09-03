// @file ShopUITypes.h
// @brief 상점(런 중 상점방) 화면 UI에 표시할 뷰 데이터입니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "DataAsset/UnitSpawnData/UnitJobType.h"
#include "Frontend/CharacterSelectTypes.h"
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
	Mercenary,
	// 온디맨드 상세(스킬/아티팩트) — Request*Detail 응답으로만 갱신
	Detail
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
	/** @brief 스킬 구매 대상 직업. Common은 모든 용병에게 표시한다. */
	UPROPERTY(BlueprintReadOnly) EUnitJobType mRequiredJobType = EUnitJobType::Common;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	UPROPERTY(BlueprintReadOnly) FText mDescription;
	UPROPERTY(BlueprintReadOnly) int32 mPrice = 0;
	UPROPERTY(BlueprintReadOnly) FLinearColor mRarityColor = FLinearColor::White;
	UPROPERTY(BlueprintReadOnly) bool mIsAffordable = true;
	UPROPERTY(BlueprintReadOnly) bool mIsSoldOut = false;
	/**
	 * @brief 이 상품을 이미 가진 파티 유닛 index 목록. 없으면 비어 있다.
	 *
	 * @details 같은 스킬을 두 번 사는 사고를 막는다. 판정(무엇이 "같은
	 * 스킬"인가)은 게임플레이 몫이라 화면은 이 목록만 읽는다. 스킬은 유닛
	 * 소유라 파티 전체가 아니라 **어느 유닛이** 가졌는지까지 담는다.
	 */
	UPROPERTY(BlueprintReadOnly) TArray<int32> mOwnedByUnitIndices;
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
 * @brief 소지 용병의 스킬 슬롯 한 칸의 표시값
 * @details
 * 빈 슬롯도 배열에 담아 배열 위치가 곧 스킬 슬롯 index —
 * 스킬 구매(빈 슬롯에 장착)와 교체(찬 슬롯 지정) 대상 선택에 그대로 씀
 */
USTRUCT(BlueprintType)
struct FShopOwnedSkillSlotUI
{
	GENERATED_BODY()

	// 빈 슬롯 여부 (빈 슬롯 = 구매 대상, 찬 슬롯 = 교체 대상)
	UPROPERTY(BlueprintReadOnly) bool mIsEmpty = true;
	UPROPERTY(BlueprintReadOnly) FText mName;
	/** 길게 누르기 상세 카드에 표시할 스킬 설명. */
	UPROPERTY(BlueprintReadOnly) FText mDescription;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
};

/**
 * @brief 소지 용병(파티 유닛) 한 명의 표시값
 * @details
 * 스킬 구매/교체는 유닛 index + 슬롯 index 지정이 필요해
 * 스킬 슬롯 전체(빈 칸 포함)를 유닛 단위로 담음.
 * 유닛 모델엔 표시용 이름이 없어 직업+레벨로 식별
 */
USTRUCT(BlueprintType)
struct FShopOwnedUnitUI
{
	GENERATED_BODY()

	// 파티 유닛 슬롯 index (스킬 구매/버리기 요청 payload 1)
	UPROPERTY(BlueprintReadOnly) int32 mUnitIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) EUnitJobType mJobType = EUnitJobType::None;
	UPROPERTY(BlueprintReadOnly) int32 mLevel = 1;
	/** 실제 파티 용병이면 참. 거짓인 직업 카드는 회색으로 보되 상품 상세 조회는 허용한다. */
	UPROPERTY(BlueprintReadOnly) bool mIsOwned = true;
	// 스킬 슬롯 전체. 배열 위치 = 스킬 슬롯 index
	UPROPERTY(BlueprintReadOnly) TArray<FShopOwnedSkillSlotUI> mSkillSlots;
};

/** @brief 휴식 화면의 파티원 한 줄을 그리기 위한 현재값/회복 후 값입니다. */
USTRUCT(BlueprintType)
struct FShopRestUnitUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mUnitIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) EUnitJobType mJobType = EUnitJobType::None;
	UPROPERTY(BlueprintReadOnly) float mHP = 0.f;
	UPROPERTY(BlueprintReadOnly) float mMaxHP = 0.f;
	UPROPERTY(BlueprintReadOnly) float mAP = 0.f;
	UPROPERTY(BlueprintReadOnly) float mMaxAP = 0.f;
};

/** @brief 상점방 휴식 서비스의 표시값입니다. 실제 가격/회복/1회 제한 판정은 ShopGameMode가 소유합니다. */
USTRUCT(BlueprintType)
struct FShopRestUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mPrice = 100;
	UPROPERTY(BlueprintReadOnly) bool mIsAffordable = false;
	UPROPERTY(BlueprintReadOnly) bool mIsUsed = false;
	UPROPERTY(BlueprintReadOnly) TArray<FShopRestUnitUI> mUnits;
};

/**
 * @brief 상점의 용병 후보 한 명.
 * @details
 * 후보의 전투 표시값은 기존 용병 선택 화면과 같은 FFrontendCharacterOption을
 * 재사용한다. 상점에서만 필요한 가격/품절/판매 슬롯만 바깥에 덧붙여 두 화면의
 * 이름·스탯·스킬 표현이 서로 갈라지지 않게 한다.
 */
USTRUCT(BlueprintType)
struct FShopMercenaryUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mSlotIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FFrontendCharacterOption mCharacter;
	UPROPERTY(BlueprintReadOnly) int32 mLevel = 1;
	UPROPERTY(BlueprintReadOnly) int32 mPrice = 0;
	UPROPERTY(BlueprintReadOnly) bool mIsAffordable = false;
	UPROPERTY(BlueprintReadOnly) bool mIsSoldOut = false;
};

/** @brief 고용 화면 오른쪽의 교체/배치 대상 파티 슬롯. 빈 슬롯도 보존한다. */
USTRUCT(BlueprintType)
struct FShopMercenaryPartySlotUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mUnitIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) bool mIsOccupied = false;
	UPROPERTY(BlueprintReadOnly) EUnitJobType mJobType = EUnitJobType::None;
	UPROPERTY(BlueprintReadOnly) int32 mLevel = 1;
};

/** @brief 상점 화면 전체 표시값입니다. */
// UI 필요값:
// - mGold: 상단 보유 골드(구매 가능 여부 갱신 기준).
// - mItems: 판매 슬롯 목록.
// - mOwnedArtifacts: 파티 소유 소지품. 거래마다 판매 목록과 함께 갱신되므로 같은 스냅샷에 담음.
// - mOwnedUnits: 파티 유닛. 스킬은 파티가 아닌 유닛+슬롯 소유라 유닛 카드에 슬롯째 담음.
USTRUCT(BlueprintType)
struct FShopUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mGold = 0;
	UPROPERTY(BlueprintReadOnly) TArray<FShopItemUI> mItems;
	UPROPERTY(BlueprintReadOnly) TArray<FShopOwnedArtifactUI> mOwnedArtifacts;
	UPROPERTY(BlueprintReadOnly) TArray<FShopOwnedUnitUI> mOwnedUnits;
	/** 여섯 직업 탐색 카드. 실제 구매 대상 유닛은 mOwnedUnits에서 별도로 선택한다. */
	UPROPERTY(BlueprintReadOnly) TArray<FShopOwnedUnitUI> mSkillTargetUnits;
	UPROPERTY(BlueprintReadOnly) FShopRestUI mRest;
	UPROPERTY(BlueprintReadOnly) TArray<FShopMercenaryUI> mMercenaries;
	UPROPERTY(BlueprintReadOnly) TArray<FShopMercenaryPartySlotUI> mMercenaryPartySlots;
};
