/*****************************************************************//**
 * @file   TreasureUITypes.h
 * @brief  보물방 화면 UI에 표시할 뷰 데이터 정의 헤더
 * @author 이문환
 * @date   2026-08-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "TreasureUITypes.generated.h"

class UTexture2D;

/**
 * @brief 보물방 화면의 갱신 도메인
 * @details
 * 위젯은 자기 도메인 알림만 받아 다시 그림
 */
UENUM(BlueprintType)
enum class ETreasureUIDomain : uint8
{
	// 상자 상태 + 보상 목록 (개봉/지급마다 함께 갱신)
	Reward
};

/**
 * @brief 보물방 보상 항목 종류
 * @details UI가 게임플레이 데이터 타입을 직접 모르게 어댑터가 변환해 넣음
 */
UENUM(BlueprintType)
enum class ETreasureItemKind : uint8
{
	Gold,        // 골드 지급
	Artifact     // 아티팩트 지급
};

/** @brief 보상 카드 한 칸을 그리기 위한 표시값 */
// UI 필요값:
// - mSlotIndex: 보상 목록 내 자리 번호 (향후 택1 기획 시 요청 payload)
// - mKind: 종류별 아이콘/배치
// - mName/mIcon/mDescription: 카드 표시
// - mAmount: 수량 (골드 전용, 아티팩트는 0)
// - mRarityColor: 희귀도 테두리 (어댑터가 enum→색 변환)
USTRUCT(BlueprintType)
struct FTreasureItemUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mSlotIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) ETreasureItemKind mKind = ETreasureItemKind::Gold;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	UPROPERTY(BlueprintReadOnly) FText mDescription;
	UPROPERTY(BlueprintReadOnly) int32 mAmount = 0;
	UPROPERTY(BlueprintReadOnly) FLinearColor mRarityColor = FLinearColor::White;
};

/** @brief 보물방 화면 전체 표시값 */
// UI 필요값:
// - mIsOpened: 상자 개봉 여부 (개봉 전엔 상자만, 개봉 후엔 보상 목록 표시)
// - mItems: 보상 카드 목록 (개봉 시 전부 지급된 내역)
USTRUCT(BlueprintType)
struct FTreasureUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) bool mIsOpened = false;
	UPROPERTY(BlueprintReadOnly) TArray<FTreasureItemUI> mItems;
};
