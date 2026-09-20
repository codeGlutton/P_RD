/*****************************************************************//**
 * @file   StaticPlayerUnitSpawnData.h
 * @brief  플레이어 유닛 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-20
 *********************************************************************/

#pragma once

#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"
#include "DataAsset/UnitSpawnData/UnitJobType.h"
#include "StaticPlayerUnitSpawnData.generated.h"

/**
 * @brief  플레이어 유닛 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticPlayerUnitSpawnData : public UStaticUnitSpawnData
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(UnitPrimaryAssetTypes::GetPlayerUnitType(), GetFName());
	}

#if WITH_EDITOR
public:
	EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

public:
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable, meta = (DisplayName = "JobType"))
	EUnitJobType mJobType = EUnitJobType::None;
};
