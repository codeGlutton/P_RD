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

/**
 * @brief  유닛 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS(abstract)
class P_RD_API UStaticUnitSpawnData : public UStaticObstacleSpawnData
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ShortCut", AssetBundles = "UI"))
	TSoftObjectPtr<UTexture2D> mShortCut;

public:
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillDatas", AllowedClasses = "/Script/P_RD.StaticUnitSkillData", AssetBundles = "PAD"))
	TArray<TSoftObjectPtr<UStaticSkillData>> mSkillDatas;
};
