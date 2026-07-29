// @file InventoryUITypes.h
// @brief 인벤토리(런 상태 확인) 화면 UI에 표시할 뷰 데이터입니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "InventoryUITypes.generated.h"

class UTexture2D;

/** @brief 인벤토리 항목 종류. 같은 슬롯 위젯을 종류별로 구분해 그리기 위한 값입니다. */
// UI는 게임플레이 데이터 클래스를 직접 모르고, 어댑터가 종류를 이 enum으로 변환해 넣는다.
UENUM(BlueprintType)
enum class EInventoryItemKind : uint8
{
	Skill,
	Equipment,
	Artifact
};

/** @brief 인벤토리 한 칸(보유 스킬/장비 공통)을 그리기 위한 표시값입니다. */
// 요약 화면이라 면 텍스처 같은 상세는 담지 않는다(상세는 롱프레스로 별도 요청).
// UI 필요값:
// - mKind: 스킬/장비 구분(슬롯 아이콘·배치 결정).
// - mItemIndex: 롱프레스 상세 요청 payload(같은 종류 배열 기준 index).
// - mName/mIcon: 슬롯 기본 표시.
// - mRarityColor: 희귀도 테두리/배경(어댑터가 enum→색 변환).
// - mDetailText: 장비 슬롯명 등 보조 한 줄.
USTRUCT(BlueprintType)
struct FInventoryItemUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) EInventoryItemKind mKind = EInventoryItemKind::Skill;
	UPROPERTY(BlueprintReadOnly) int32 mItemIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	UPROPERTY(BlueprintReadOnly) FLinearColor mRarityColor = FLinearColor::White;
	UPROPERTY(BlueprintReadOnly) FText mDetailText;
};

/**
 * @brief 가방의 용병별 성장/생존 상태 한 줄.
 *
 * @details 경험치가 파티 공용 값에서 용병별 값으로 바뀌었으므로 레벨과 EXP를
 *          FInventoryUI 하나에 합치지 않는다. 각 용병의 현재 값을 독립적으로
 *          담아 한 화면에서 비교할 수 있게 한다.
 */
USTRUCT(BlueprintType)
struct FInventoryMercenaryUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mPartyIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mPortrait = nullptr;
	UPROPERTY(BlueprintReadOnly) int32 mLevel = 1;
	UPROPERTY(BlueprintReadOnly) float mExp = 0.f;
	UPROPERTY(BlueprintReadOnly) float mMaxExp = 0.f;
	UPROPERTY(BlueprintReadOnly) float mHP = 0.f;
	UPROPERTY(BlueprintReadOnly) float mMaxHP = 0.f;
};

/** @brief 인벤토리(현재 런 상태) 화면 전체 표시값입니다. */
// 읽기 전용 요약 화면이다 — 파티 골드 + 용병별 성장 상태 + 획득 보상 목록.
// UI 필요값:
// - mGold: 파티 공용 재화.
// - mMercenaries: 용병마다 별도로 저장되는 레벨/EXP/체력.
// - mSkills/mEquipment/mArtifacts: 이번 런에서 실제로 획득해 보관 중인 보상.
USTRUCT(BlueprintType)
struct FInventoryUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mGold = 0;
	UPROPERTY(BlueprintReadOnly) TArray<FInventoryMercenaryUI> mMercenaries;

	UPROPERTY(BlueprintReadOnly) TArray<FInventoryItemUI> mSkills;
	UPROPERTY(BlueprintReadOnly) TArray<FInventoryItemUI> mEquipment;
	UPROPERTY(BlueprintReadOnly) TArray<FInventoryItemUI> mArtifacts;
};
