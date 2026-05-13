/*****************************************************************//**
 * @file   StaticTreasureRoomSpawnData.h
 * @brief  보상 방 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-12
 *********************************************************************/

#pragma once

#include "DataAsset/RoomSpawnData/StaticRoomSpawnData.h"
#include "StaticTreasureRoomSpawnData.generated.h"

/**
 * @brief  보상 방 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticTreasureRoomSpawnData : public UStaticRoomSpawnData
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(RoomPrimaryAssetTypes::GetTreasureRoomType(mStageLevel), GetFName());
	}

public:
	UPROPERTY(Category = "Actor", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TreasureBoxClass", AssetBundles = "Actor"))
	TSoftClassPtr<AActor> mTreasureBoxClass;
};
