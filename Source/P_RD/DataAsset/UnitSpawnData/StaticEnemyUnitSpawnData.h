/*****************************************************************//**
 * @file   StaticEnemyUnitSpawnData.h
 * @brief  적 유닛 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-20
 *********************************************************************/

#pragma once

#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"
#include "StateTree.h"
#include "StaticEnemyUnitSpawnData.generated.h"

/**
 * @brief  적 유닛 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticEnemyUnitSpawnData : public UStaticUnitSpawnData
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(UnitPrimaryAssetTypes::GetEnemyUnitType(), GetFName());
	}

public:
	UPROPERTY(Category = "AI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "StateTree", BUNDLE_ACTOR))
	TSoftObjectPtr<UStateTree> mStateTree;
};
