/*****************************************************************//**
 * @file   StaticUnitSpawnData.h
 * @brief  유닛 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-20
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/ObstacleSpawnData/StaticObstacleSpawnData.h"
#include "StaticUnitSpawnData.generated.h"

class UStaticSkillData;
class UStaticEquipmentData;

/**
 * @brief  유닛 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS(abstract)
class P_RD_API UStaticUnitSpawnData : public UStaticObstacleSpawnData
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillDatas", AssetBundles = "PAD"))
	TArray<TSoftObjectPtr<UStaticSkillData>> mSkillDatas;
    UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EquipmentDatas", AssetBundles = "PAD"))
	TArray<TSoftObjectPtr<UStaticEquipmentData>> mEquipmentDatas;
};
