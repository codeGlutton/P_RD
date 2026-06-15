/*****************************************************************//**
 * @file   StaticObstacleSpawnData.h
 * @brief  장애물 생성 시 사용되는 정적 Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-20
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/PrimaryAssetType.h"
#include "DataAsset/BundleType.h"
#include "StaticObstacleSpawnData.generated.h"

/**
 * @brief  장애물 생성 시 사용되는 정적 Data Asset
 */
UCLASS()
class P_RD_API UStaticObstacleSpawnData : public UDataAsset
{
	GENERATED_BODY()

public:
    UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Class", MustImplement = "/Script/P_RD.TileActor", AssetBundles = "Actor"))
    TSoftClassPtr<AActor> mClass;

public:
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Portrait", AssetBundles = "UI"))
    TSoftObjectPtr<UTexture2D> mPortrait;
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Name"))
    FText mName;
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Description", MultiLine = true))
    FText mDescription;
};
