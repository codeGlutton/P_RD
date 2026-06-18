// @file InventoryViewTypes.h
// @brief 인벤토리(런 상태 확인) 화면 UI에 표시할 뷰 데이터입니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "DataAsset/RarityType.h"
#include "InventoryViewTypes.generated.h"

class UTexture2D;

/** @brief 인벤토리 항목 종류. 같은 슬롯 위젯을 종류별로 구분해 그리기 위한 값입니다. */
// UI는 게임플레이 데이터 클래스를 직접 모르고, 어댑터가 종류를 이 enum으로 변환해 넣는다.
UENUM(BlueprintType)
enum class EInventoryItemKind : uint8
{
	Dice,
	Skill,
	Equipment
};

/** @brief 인벤토리 한 칸(보유 다이스/스킬/장비 공통)을 그리기 위한 표시값입니다. */
// 요약 화면이라 면 텍스처 같은 상세는 담지 않는다(상세는 롱프레스로 별도 요청).
// UI 필요값:
// - mKind: 다이스/스킬/장비 구분(슬롯 아이콘·배치 결정).
// - mItemIndex: 롱프레스 상세 요청 payload(같은 종류 배열 기준 index).
// - mName/mIcon: 슬롯 기본 표시.
// - mRarityColor: 희귀도 테두리/배경(어댑터가 enum→색 변환).
// - mDetailText: 다이스면 "d20", 장비면 슬롯명 등 보조 한 줄.
USTRUCT(BlueprintType)
struct FInventoryItemView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) EInventoryItemKind mKind = EInventoryItemKind::Dice;
	UPROPERTY(BlueprintReadOnly) int32 mItemIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	UPROPERTY(BlueprintReadOnly) FLinearColor mRarityColor = FLinearColor::White;
	UPROPERTY(BlueprintReadOnly) FText mDetailText;
};

/** @brief 인벤토리(현재 런 상태) 화면 전체 표시값입니다. */
// 읽기 전용 요약 화면이다 — 플레이어 메타(골드/레벨/체력) + 보유 다이스/스킬/장비 목록.
// UI 필요값:
// - mGold/mLevel/mHP/mMaxHP: 상단 메타 표시.
// - mDice/mSkills/mEquipment: 종류별 보유 목록 그리드.
// [합의필요] 최종 소스 = URunPersistData(보유 다이스 ids) + UDiceModel/USkillData/장비 + UUnitData(HP/Gold).
USTRUCT(BlueprintType)
struct FInventoryView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mGold = 0;
	UPROPERTY(BlueprintReadOnly) int32 mLevel = 0;
	UPROPERTY(BlueprintReadOnly) float mHP = 0.f;
	UPROPERTY(BlueprintReadOnly) float mMaxHP = 0.f;

	UPROPERTY(BlueprintReadOnly) TArray<FInventoryItemView> mDice;
	UPROPERTY(BlueprintReadOnly) TArray<FInventoryItemView> mSkills;
	UPROPERTY(BlueprintReadOnly) TArray<FInventoryItemView> mEquipment;
};
