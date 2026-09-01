/*****************************************************************//**
 * @file   LevelAttributeCache.h
 * @brief  레벨별 희귀도 비율 및 가격 데이터 캐싱 구조체 헤더
 * @author 모호재
 * @date   2026-08-05
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/RarityRate.h"
#include "LevelAttributeCache.generated.h"

/**
 * @brief 레벨별 희귀도 비율 및 가격 캐싱 데이터
 */
USTRUCT(BlueprintType)
struct FLevelAttributeCache
{
	GENERATED_BODY()

public:
	/*
	 * @brief 특정 레벨(1-indexed)의 최대 경험치를 반환합니다.
	 * @param Level 대상 레벨
	 * @return 해당 레벨의 Price
	 */
	float GetMaxExp(int32 Level) const;

	/*
	 * @brief 특정 레벨(1-indexed)의 희귀도 비율을 반환합니다.
	 * @param Level 대상 레벨
	 * @return 해당 레벨의 FRarityRate
	 */
	FRarityRate GetRarityRate(int32 Level) const;

	/*
	 * @brief 특정 레벨(1-indexed)의 가격을 반환합니다.
	 * @param Level 대상 레벨
	 * @return 해당 레벨의 Price
	 */
	float GetPrice(int32 Level) const;

public:
	UPROPERTY(Category = LevelCache, VisibleAnywhere, BlueprintReadWrite, meta = (DisplayName = "MaxExps"))
	TArray<float> mMaxExps;

	UPROPERTY(Category = LevelCache, VisibleAnywhere, BlueprintReadWrite, meta = (DisplayName = "RarityRates"))
	TArray<FRarityRate> mRarityRates;

	UPROPERTY(Category = LevelCache, VisibleAnywhere, BlueprintReadWrite, meta = (DisplayName = "Prices"))
	TArray<float> mPrices;
};
