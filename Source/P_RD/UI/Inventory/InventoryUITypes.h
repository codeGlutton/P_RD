// @file InventoryUITypes.h
// @brief 파티 공용 인벤토리 화면 UI에 표시할 뷰 데이터입니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "InventoryUITypes.generated.h"

class UTexture2D;

/** @brief 파티가 공용으로 소유하는 아티팩트 한 칸의 표시값입니다. */
USTRUCT(BlueprintType)
struct FInventoryArtifactUI
{
	GENERATED_BODY()

	/** @brief 상세 보기 요청에 돌려보낼 공용 아티팩트 배열 index. */
	UPROPERTY(BlueprintReadOnly) int32 mArtifactIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	UPROPERTY(BlueprintReadOnly) FLinearColor mRarityColor = FLinearColor::White;
	UPROPERTY(BlueprintReadOnly) FText mDetailText;
};

/** @brief 파티 공용 인벤토리 전체 표시값입니다. */
// 용병 성장/스킬/장비는 각 용병 화면의 책임이다. 이 화면에는 공용 골드와
// 파티 전체에 적용되는 아티팩트만 들어온다.
USTRUCT(BlueprintType)
struct FInventoryUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mGold = 0;
	UPROPERTY(BlueprintReadOnly) TArray<FInventoryArtifactUI> mArtifacts;
};
