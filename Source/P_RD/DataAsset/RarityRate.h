/*****************************************************************//**
 * @file   RarityRate.h
 * @brief  희귀도 비율 정의 헤더
 * @author 모호재
 * @date   2026-05-14
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/RarityType.h"
#include "RarityRate.generated.h"

/**
 * @brief  희귀도 비율
 */
USTRUCT(BlueprintType)
struct FRarityRate
{
	GENERATED_BODY()

public:
	float GetRate(ERarityType Type) const;
	float GetRate(ERarityType Type, const TArray<ERarityType>& IncludedTypes) const;
	ERarityType GetType(float Rate) const;
	ERarityType GetType(float Rate, const TArray<ERarityType>& IncludedTypes) const;

protected:
	float GetTotalWeight() const;
	float GetTotalWeight(const TArray<ERarityType>& IncludedTypes) const;

public:
	UPROPERTY(Category = "Weight", EditAnywhere, meta = (DisplayName = "Weights", ArraySizeEnum = "ERarityType"))
	float mWeights[static_cast<uint8>(ERarityType::Count)] = {};
};
