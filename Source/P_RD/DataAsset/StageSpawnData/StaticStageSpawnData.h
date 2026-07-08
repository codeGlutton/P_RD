/*****************************************************************//**
 * @file   StaticStageSpawnData.h
 * @brief  스테이지 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/PrimaryAssetType.h"
#include "DataAsset/BundleType.h"
#include "DataAsset/StageSpawnData/StageLevelType.h"
#include "PCGStage/RoomType.h"
#include "StaticStageSpawnData.generated.h"

/**
 * @brief  스테이지 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticStageSpawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(StagePrimaryAssetTypes::GetStageType(), GetFName());
	}

public:
	UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "StageName"))
	FText mStageName;
	UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "BackgroundPanel", AssetBundles = "UI"))
	TSoftObjectPtr<UTexture2D> mBackgroundPanel;
	UPROPERTY(Category = "UI", EditAnywhere, meta = (DisplayName = "RoomIcons", ArraySizeEnum = "ERoomType", AssetBundles = "UI"))
	TSoftObjectPtr<UTexture2D> mRoomIcons[static_cast<uint8>(ERoomType::Count)];

public:
	UPROPERTY(Category = "Sound", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "BossMonsterRoomBGM", AssetBundles = "World"))
	TSoftObjectPtr<USoundBase> mBossMonsterRoomBGM;

	UPROPERTY(Category = "Sound", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MonsterRoomBGM", AssetBundles = "World"))
	TSoftObjectPtr<USoundBase> mMonsterRoomBGM;

	UPROPERTY(Category = "Sound", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ShopRoomBGM", AssetBundles = "World"))
	TSoftObjectPtr<USoundBase> mShopRoomBGM;

	UPROPERTY(Category = "Sound", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TreasureRoomBGM", AssetBundles = "World"))
	TSoftObjectPtr<USoundBase> mTreasureRoomBGM;
};
