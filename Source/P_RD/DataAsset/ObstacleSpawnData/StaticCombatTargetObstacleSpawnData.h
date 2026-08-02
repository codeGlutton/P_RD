/*****************************************************************//**
 * @file   StaticCombatTargetObstacleSpawnData.h
 * @brief  전투가능한 장애물 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-08-01
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/ObstacleSpawnData/StaticObstacleSpawnData.h"
#include "StaticCombatTargetObstacleSpawnData.generated.h"

class UStaticSkillData;

/**
 * @brief  전투가능한 장애물 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS(abstract)
class P_RD_API UStaticCombatTargetObstacleSpawnData : public UStaticObstacleSpawnData
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(ObstaclePrimaryAssetTypes::GetCombatTargetObstacleType(), GetFName());
	}

public:
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillDatas", AssetBundles = "PAD"))
	TArray<TSoftObjectPtr<UStaticSkillData>> mSkillDatas;
};
